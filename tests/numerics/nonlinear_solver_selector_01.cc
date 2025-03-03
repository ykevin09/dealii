// ---------------------------------------------------------------------
//
// Copyright (C) 2007 - 2023 by the deal.II authors
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



// Tests the class NonlinearSolverSelector using an example based on
// the step-77 tutorial. The output will vary depending on what
// packages are configured with deal.II.

#include <deal.II/base/function.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/timer.h>
#include <deal.II/base/utilities.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_refinement.h>
#include <deal.II/grid/tria.h>

#include <deal.II/lac/affine_constraints.h>
#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/sparse_direct.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/error_estimator.h>
#include <deal.II/numerics/matrix_tools.h>
#include <deal.II/numerics/nonlinear.h>
#include <deal.II/numerics/solution_transfer.h>
#include <deal.II/numerics/vector_tools.h>
#ifdef DEAL_II_WITH_SUNDIALS
#  include <deal.II/sundials/kinsol.h>
#endif
#ifdef DEAL_II_TRILINOS_WITH_NOX
#  include <deal.II/trilinos/nox.h>
#endif

#include <fstream>
#include <iostream>

#include "../tests.h"


namespace nonlinear_solver_selector_test
{
  using namespace dealii;

  using NLSolve = NonlinearSolverSelector<Vector<double>>;

#ifndef SOLVER
#  define SOLVER NLSolve::AdditionalData::kinsol
#endif

  template <int dim>
  class MinimalSurfaceProblem
  {
  public:
    MinimalSurfaceProblem();
    void
    run();

  private:
    void
    setup_system(const bool initial_step);
    void
    solve(const Vector<double> &rhs,
          Vector<double>       &solution,
          const double          tolerance);
    void
    refine_mesh();
    void
    output_results(const unsigned int refinement_cycle);
    void
    set_boundary_values();
    void
    compute_and_factorize_jacobian(const Vector<double> &evaluation_point);
    void
    compute_residual(const Vector<double> &evaluation_point,
                     Vector<double>       &residual);

    Triangulation<dim> triangulation;

    DoFHandler<dim> dof_handler;
    FE_Q<dim>       fe;

    AffineConstraints<double> hanging_node_constraints;

    SparsityPattern                      sparsity_pattern;
    SparseMatrix<double>                 jacobian_matrix;
    std::unique_ptr<SparseDirectUMFPACK> jacobian_matrix_factorization;

    Vector<double> current_solution;
  };


  template <int dim>
  class BoundaryValues : public Function<dim>
  {
  public:
    virtual double
    value(const Point<dim> &p, const unsigned int component = 0) const override;
  };


  template <int dim>
  double
  BoundaryValues<dim>::value(const Point<dim> &p,
                             const unsigned int /*component*/) const
  {
    return std::sin(2 * numbers::PI * (p[0] + p[1]));
  }


  template <int dim>
  MinimalSurfaceProblem<dim>::MinimalSurfaceProblem()
    : dof_handler(triangulation)
    , fe(1)
  {}


  template <int dim>
  void
  MinimalSurfaceProblem<dim>::setup_system(const bool initial_step)
  {
    if (initial_step)
      {
        dof_handler.distribute_dofs(fe);
        current_solution.reinit(dof_handler.n_dofs());

        hanging_node_constraints.clear();
        DoFTools::make_hanging_node_constraints(dof_handler,
                                                hanging_node_constraints);
        hanging_node_constraints.close();
      }

    DynamicSparsityPattern dsp(dof_handler.n_dofs());
    DoFTools::make_sparsity_pattern(dof_handler, dsp);

    hanging_node_constraints.condense(dsp);

    sparsity_pattern.copy_from(dsp);
    jacobian_matrix.reinit(sparsity_pattern);
    jacobian_matrix_factorization.reset();
  }


  template <int dim>
  void
  MinimalSurfaceProblem<dim>::compute_and_factorize_jacobian(
    const Vector<double> &evaluation_point)
  {
    {
      deallog << "  Computing Jacobian matrix" << std::endl;

      const QGauss<dim> quadrature_formula(fe.degree + 1);

      jacobian_matrix = 0;

      FEValues<dim> fe_values(fe,
                              quadrature_formula,
                              update_gradients | update_quadrature_points |
                                update_JxW_values);

      const unsigned int dofs_per_cell = fe.n_dofs_per_cell();
      const unsigned int n_q_points    = quadrature_formula.size();

      FullMatrix<double> cell_matrix(dofs_per_cell, dofs_per_cell);

      std::vector<Tensor<1, dim>> evaluation_point_gradients(n_q_points);

      std::vector<types::global_dof_index> local_dof_indices(dofs_per_cell);

      for (const auto &cell : dof_handler.active_cell_iterators())
        {
          cell_matrix = 0;

          fe_values.reinit(cell);

          fe_values.get_function_gradients(evaluation_point,
                                           evaluation_point_gradients);

          for (unsigned int q = 0; q < n_q_points; ++q)
            {
              const double coeff =
                1.0 / std::sqrt(1 + evaluation_point_gradients[q] *
                                      evaluation_point_gradients[q]);

              for (unsigned int i = 0; i < dofs_per_cell; ++i)
                {
                  for (unsigned int j = 0; j < dofs_per_cell; ++j)
                    cell_matrix(i, j) +=
                      (((fe_values.shape_grad(i, q)    // ((\nabla \phi_i
                         * coeff                       //   * a_n
                         * fe_values.shape_grad(j, q)) //   * \nabla \phi_j)
                        -                              //  -
                        (fe_values.shape_grad(i, q)    //  (\nabla \phi_i
                         * coeff * coeff * coeff       //   * a_n^3
                         *
                         (fe_values.shape_grad(j, q)       //   * (\nabla \phi_j
                          * evaluation_point_gradients[q]) //      * \nabla u_n)
                         * evaluation_point_gradients[q])) //   * \nabla u_n)))
                       * fe_values.JxW(q));                // * dx
                }
            }

          cell->get_dof_indices(local_dof_indices);
          hanging_node_constraints.distribute_local_to_global(cell_matrix,
                                                              local_dof_indices,
                                                              jacobian_matrix);
        }

      std::map<types::global_dof_index, double> boundary_values;
      VectorTools::interpolate_boundary_values(dof_handler,
                                               0,
                                               Functions::ZeroFunction<dim>(),
                                               boundary_values);
      Vector<double> dummy_solution(dof_handler.n_dofs());
      Vector<double> dummy_rhs(dof_handler.n_dofs());
      MatrixTools::apply_boundary_values(boundary_values,
                                         jacobian_matrix,
                                         dummy_solution,
                                         dummy_rhs);
    }

    {
      deallog << "  Factorizing Jacobian matrix" << std::endl;

      jacobian_matrix_factorization = std::make_unique<SparseDirectUMFPACK>();
      jacobian_matrix_factorization->factorize(jacobian_matrix);
    }
  }


  template <int dim>
  void
  MinimalSurfaceProblem<dim>::compute_residual(
    const Vector<double> &evaluation_point,
    Vector<double>       &residual)
  {
    deallog << "  Computing residual vector..." << std::flush;
    residual = 0.;

    const QGauss<dim> quadrature_formula(fe.degree + 1);
    FEValues<dim>     fe_values(fe,
                            quadrature_formula,
                            update_gradients | update_quadrature_points |
                              update_JxW_values);

    const unsigned int dofs_per_cell = fe.n_dofs_per_cell();
    const unsigned int n_q_points    = quadrature_formula.size();

    Vector<double>              cell_residual(dofs_per_cell);
    std::vector<Tensor<1, dim>> evaluation_point_gradients(n_q_points);

    std::vector<types::global_dof_index> local_dof_indices(dofs_per_cell);

    for (const auto &cell : dof_handler.active_cell_iterators())
      {
        cell_residual = 0;
        fe_values.reinit(cell);

        fe_values.get_function_gradients(evaluation_point,
                                         evaluation_point_gradients);


        for (unsigned int q = 0; q < n_q_points; ++q)
          {
            const double coeff =
              1.0 / std::sqrt(1 + evaluation_point_gradients[q] *
                                    evaluation_point_gradients[q]);

            for (unsigned int i = 0; i < dofs_per_cell; ++i)
              cell_residual(i) +=
                (fe_values.shape_grad(i, q)      // \nabla \phi_i
                 * coeff                         // * a_n
                 * evaluation_point_gradients[q] // * \nabla u_n
                 * fe_values.JxW(q));            // * dx
          }

        cell->get_dof_indices(local_dof_indices);
        for (unsigned int i = 0; i < dofs_per_cell; ++i)
          residual(local_dof_indices[i]) += cell_residual(i);
      }

    hanging_node_constraints.condense(residual);

    for (const types::global_dof_index i :
         DoFTools::extract_boundary_dofs(dof_handler))
      residual(i) = 0;

    for (const types::global_dof_index i :
         DoFTools::extract_hanging_node_dofs(dof_handler))
      residual(i) = 0;

    deallog << " norm=" << residual.l2_norm() << std::endl;
  }


  template <int dim>
  void
  MinimalSurfaceProblem<dim>::solve(const Vector<double> &rhs,
                                    Vector<double>       &solution,
                                    const double /*tolerance*/)
  {
    deallog << "  Solving linear system" << std::endl;

    jacobian_matrix_factorization->vmult(solution, rhs);

    hanging_node_constraints.distribute(solution);
  }


  template <int dim>
  void
  MinimalSurfaceProblem<dim>::set_boundary_values()
  {
    std::map<types::global_dof_index, double> boundary_values;
    VectorTools::interpolate_boundary_values(dof_handler,
                                             0,
                                             BoundaryValues<dim>(),
                                             boundary_values);
    for (const auto &boundary_value : boundary_values)
      current_solution(boundary_value.first) = boundary_value.second;

    hanging_node_constraints.distribute(current_solution);
  }


  template <int dim>
  void
  MinimalSurfaceProblem<dim>::run()
  {
    GridGenerator::hyper_ball(triangulation);
    triangulation.refine_global(2);

    setup_system(/*initial_step=*/true);
    set_boundary_values();

    {
      typename NLSolve::AdditionalData additional_data;
      additional_data.solver_type = SOLVER;

      NLSolve nonlinear_solver(additional_data);

      nonlinear_solver.reinit_vector = [&](Vector<double> &x) {
        x.reinit(dof_handler.n_dofs());
      };

      nonlinear_solver.residual = [&](const Vector<double> &evaluation_point,
                                      Vector<double>       &residual) {
        compute_residual(evaluation_point, residual);
      };

      nonlinear_solver.setup_jacobian = [&](const Vector<double> &current_u) {
        compute_and_factorize_jacobian(current_u);
      };

      nonlinear_solver.solve_with_jacobian = [&](const Vector<double> &rhs,
                                                 Vector<double>       &dst,
                                                 const double tolerance) {
        solve(rhs, dst, tolerance);
      };

      nonlinear_solver.solve(current_solution);
    }
  }
} // namespace nonlinear_solver_selector_test


int
main()
{
  initlog();

  using namespace nonlinear_solver_selector_test;

  MinimalSurfaceProblem<2> laplace_problem_2d;
  laplace_problem_2d.run();
}
