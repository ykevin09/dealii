// ---------------------------------------------------------------------
//
// Copyright (C) 2003 - 2021 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------


// test output of curved cells at the boundary and in the inner of the domain,
// where the last one is only relevant for mappings of type MappingQEulerian

#include <deal.II/base/multithread_info.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/thread_management.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/mapping_q1.h>
#include <deal.II/fe/mapping_q_eulerian.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/lac/affine_constraints.h>
#include <deal.II/lac/constrained_linear_operator.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/identity_matrix.h>
#include <deal.II/lac/linear_operator.h>
#include <deal.II/lac/packaged_operation.h>
#include <deal.II/lac/precondition.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/vector_memory.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/matrix_tools.h>

#include <iostream>
#include <vector>

#include "../tests.h"


void
laplace_solve(const SparseMatrix<double>      &S,
              const AffineConstraints<double> &constraints,
              Vector<double>                  &u)
{
  const unsigned int n_dofs = S.n();
  const auto         op     = linear_operator(S);
  const auto constrained_op = constrained_linear_operator(constraints, op);

  SolverControl                       control(10000, 1.e-10, false, false);
  GrowingVectorMemory<Vector<double>> mem;
  SolverCG<Vector<double>>            solver(control, mem);

  Vector<double> f(n_dofs);

  const auto constrained_rhs = constrained_right_hand_side(constraints, op, f);
  solver.solve(constrained_op, u, constrained_rhs, PreconditionIdentity());
  constraints.distribute(u);
}


// create a ring grid and compute a MappingQEuler to represent the inner
// boundary
void
curved_grid(std::ostream &out)
{
  // number of cells in radial and
  // circumferential direction
  const unsigned int n_phi = 4, n_r = 8;
  // inner and outer radius
  const double r_i = 0.5, r_a = 2.0;
  // we want to increase the radial extent of
  // each cell by 'factor'. compute, how large
  // the first extent 'dr' has to be
  double factor = 2., sum = 0;
  for (unsigned int j = 0; j < n_r; ++j)
    sum += std::pow(factor, 1. * j);
  double dr = (r_a - r_i) / sum;
  // radii of the cells
  std::vector<double> r(n_r + 1, r_i);
  for (unsigned int j = 1; j < n_r + 1; ++j)
    {
      r[j] = r[j - 1] + dr;
      dr *= factor;
    }
  // create vertices
  std::vector<Point<2>> vertices(n_phi * (n_r + 1));
  for (unsigned int j = 0; j < n_r + 1; ++j)
    for (unsigned int i = 0; i < n_phi; ++i)
      {
        const unsigned int p     = i + j * n_phi;
        const double       alpha = i * 2 * numbers::PI / n_phi;
        vertices[p] = Point<2>(r[j] * cos(alpha), r[j] * sin(alpha));
      }
  // create connectivity
  std::vector<CellData<2>> cells(n_phi * n_r);
  for (unsigned int j = 0; j < n_r; ++j)
    for (unsigned int i = 0; i < n_phi; ++i)
      {
        const unsigned int index         = i + j * n_phi;
        const unsigned int p             = i + j * n_phi;
        const unsigned int p_iplus       = (i + 1) % n_phi + j * n_phi;
        const unsigned int p_iplus_jplus = (i + 1) % n_phi + (j + 1) * n_phi;
        const unsigned int p_jplus       = i + (j + 1) * n_phi;

        cells[index].vertices[0] = p_iplus;
        cells[index].vertices[1] = p;
        cells[index].vertices[2] = p_iplus_jplus;
        cells[index].vertices[3] = p_jplus;
      }
  // create triangulation
  Triangulation<2> triangulation;
  triangulation.create_triangulation(vertices, cells, SubCellData());
  // now provide everything that is
  // needed for solving a Laplace
  // equation.
  MappingQ<2>   mapping_q1(1);
  FE_Q<2>       fe(2);
  DoFHandler<2> dof_handler(triangulation);
  dof_handler.distribute_dofs(fe);
  SparsityPattern sparsity_pattern(dof_handler.n_dofs(),
                                   dof_handler.n_dofs(),
                                   dof_handler.max_couplings_between_dofs());
  DoFTools::make_sparsity_pattern(dof_handler, sparsity_pattern);
  sparsity_pattern.compress();
  SparseMatrix<double> S(sparsity_pattern);
  QGauss<2>            quadrature(4);
  MatrixCreator::create_laplace_matrix(mapping_q1, dof_handler, quadrature, S);
  // set up the boundary values for
  // the laplace problem
  std::vector<AffineConstraints<double>> m(2);
  // fill these constraints: on the inner boundary try
  // to approximate a circle, on the outer
  // boundary use straight lines
  DoFHandler<2>::face_iterator face;
  for (const auto &cell : dof_handler.active_cell_iterators())
    {
      // fix all vertices
      for (const unsigned int vertex_no : GeometryInfo<2>::vertex_indices())
        {
          for (unsigned int i = 0; i < 2; ++i)
            if (m[i].is_constrained(cell->vertex_dof_index(vertex_no, 0)) ==
                false)
              m[i].constrain_dof_to_zero(cell->vertex_dof_index(vertex_no, 0));
        }

      if (cell->at_boundary())
        for (const unsigned int face_no : GeometryInfo<2>::face_indices())
          {
            face = cell->face(face_no);
            // insert a modified value for
            // the middle point of boundary
            // faces
            if (face->at_boundary())
              {
                const double eps = 1e-4;
                if (std::fabs(face->vertex(1).norm() - r_i) < eps)
                  for (unsigned int i = 0; i < 2; ++i)
                    {
                      m[i].constrain_dof_to_zero(face->dof_index(0));
                      m[i].set_inhomogeneity(face->dof_index(0),
                                             (face->center() *
                                              (r_i / face->center().norm() -
                                               1))(i));
                    }
                else if (std::fabs(face->vertex(1).norm() - r_a) < eps)
                  for (unsigned int i = 0; i < 2; ++i)
                    m[i].constrain_dof_to_zero(face->dof_index(0));
                else
                  Assert(false, ExcInternalError());
              }
          }
    }
  m[0].close();
  m[1].close();

  // solve the 2 problems with
  // different right hand sides.
  Vector<double> us[2];
  for (unsigned int i = 0; i < 2; ++i)
    us[i].reinit(dof_handler.n_dofs());
  // solve linear systems in parallel
  std::vector<std::thread> threads;
  for (unsigned int i = 0; i < 2; ++i)
    threads.emplace_back(&laplace_solve,
                         std::ref(S),
                         std::ref(m[i]),
                         std::ref(us[i]));
  threads[0].join();
  threads[1].join();
  // create a new DoFHandler for the combined
  // system
  FESystem<2>   cfe(FE_Q<2>(2), 2);
  DoFHandler<2> cdh(triangulation);
  cdh.distribute_dofs(cfe);
  Vector<double> displacements(cdh.n_dofs()), dx(fe.dofs_per_cell),
    dy(fe.dofs_per_cell), dxy(cfe.dofs_per_cell);
  DoFHandler<2>::active_cell_iterator component_cell =
                                        dof_handler.begin_active(),
                                      end_c         = dof_handler.end(),
                                      combined_cell = cdh.begin_active();
  for (; component_cell != end_c; ++component_cell, ++combined_cell)
    {
      component_cell->get_dof_values(us[0], dx);
      component_cell->get_dof_values(us[1], dy);
      for (unsigned int i = 0; i < fe.dofs_per_cell; ++i)
        {
          dxy(cfe.component_to_system_index(0, i)) = dx(i);
          dxy(cfe.component_to_system_index(1, i)) = dy(i);
        }
      combined_cell->set_dof_values(dxy, displacements);
    }
  // and create the MappingQEulerian
  MappingQEulerian<2> euler(2, cdh, displacements);
  // now the actual test
  DataOut<2> data_out;
  data_out.attach_dof_handler(cdh);
  std::vector<std::string> names;
  names.push_back("x_displacement");
  names.push_back("y_displacement");
  data_out.add_data_vector(displacements, names);
  // output with no mapping
  data_out.build_patches(5);
  data_out.write_gnuplot(out);
  // output with no curved cells
  data_out.build_patches(euler, 5, DataOut<2>::no_curved_cells);
  data_out.write_gnuplot(out);
  // output with curved cells at the boundary
  // (default)
  data_out.build_patches(euler, 5);
  data_out.write_gnuplot(out);
  // output with all cells curved
  data_out.build_patches(euler, 5, DataOut<2>::curved_inner_cells);
  data_out.write_gnuplot(out);
}



int
main()
{
  initlog();
  deallog.get_file_stream() << std::setprecision(4);

  curved_grid(deallog.get_file_stream());
}
