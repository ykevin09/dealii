// ---------------------------------------------------------------------
//
// Copyright (C) 2016 - 2020 by the deal.II authors
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


// Check MGTransferMatrixFree by comparison with MGTransferPrebuilt on a
// series of meshes with adaptive meshes for FE_DGQ (except for the different
// element and no constraints the same test as transfer_matrix_free_01)

#include <deal.II/distributed/tria.h>

#include <deal.II/fe/fe_dgq.h>

#include <deal.II/grid/grid_generator.h>

#include <deal.II/lac/la_parallel_vector.h>

#include <deal.II/multigrid/mg_transfer.h>
#include <deal.II/multigrid/mg_transfer_global_coarsening.h>

#include "../tests.h"


template <int dim, typename Number>
void
check(const unsigned int fe_degree)
{
  FE_DGQ<dim> fe(fe_degree);
  deallog << "FE: " << fe.get_name() << std::endl;

  // run a few different sizes...
  unsigned int sizes[] = {1, 2, 3};
  for (unsigned int cycle = 0; cycle < sizeof(sizes) / sizeof(unsigned int);
       ++cycle)
    {
      unsigned int n_refinements = 0;
      unsigned int n_subdiv      = sizes[cycle];
      if (n_subdiv > 1)
        while (n_subdiv % 2 == 0)
          {
            n_refinements += 1;
            n_subdiv /= 2;
          }
      n_refinements += 3 - dim;
      if (fe_degree < 3)
        n_refinements += 1;

      parallel::distributed::Triangulation<dim> tr(
        MPI_COMM_WORLD,
        Triangulation<dim>::limit_level_difference_at_vertices,
        parallel::distributed::Triangulation<
          dim>::construct_multigrid_hierarchy);
      GridGenerator::subdivided_hyper_cube(tr, n_subdiv);
      tr.refine_global(n_refinements);

      // adaptive refinement into a circle
      for (typename Triangulation<dim>::active_cell_iterator cell =
             tr.begin_active();
           cell != tr.end();
           ++cell)
        if (cell->is_locally_owned() && cell->center().norm() < 0.5)
          cell->set_refine_flag();
      tr.execute_coarsening_and_refinement();
      for (typename Triangulation<dim>::active_cell_iterator cell =
             tr.begin_active();
           cell != tr.end();
           ++cell)
        if (cell->is_locally_owned() && cell->center().norm() > 0.3 &&
            cell->center().norm() < 0.4)
          cell->set_refine_flag();
      tr.execute_coarsening_and_refinement();
      for (typename Triangulation<dim>::active_cell_iterator cell =
             tr.begin_active();
           cell != tr.end();
           ++cell)
        if (cell->is_locally_owned() && cell->center().norm() > 0.33 &&
            cell->center().norm() < 0.37)
          cell->set_refine_flag();
      tr.execute_coarsening_and_refinement();

      deallog << "no. cells: " << tr.n_global_active_cells() << std::endl;

      DoFHandler<dim> mgdof(tr);
      mgdof.distribute_dofs(fe);
      mgdof.distribute_mg_dofs();

      // build reference
      MGTransferPrebuilt<LinearAlgebra::distributed::Vector<double>>
        transfer_ref;
      transfer_ref.build(mgdof);

      // build matrix-free transfer
      MGTransferMF<dim, Number> transfer;
      transfer.build(mgdof);

      const Number tolerance = 1000. * std::numeric_limits<Number>::epsilon();

      // check prolongation for all levels using random vector
      for (unsigned int level = 1;
           level < mgdof.get_triangulation().n_global_levels();
           ++level)
        {
          LinearAlgebra::distributed::Vector<Number> v1, v2;
          LinearAlgebra::distributed::Vector<double> v1_cpy, v2_cpy, v3;
          v1.reinit(mgdof.locally_owned_mg_dofs(level - 1), MPI_COMM_WORLD);
          v2.reinit(mgdof.locally_owned_mg_dofs(level), MPI_COMM_WORLD);
          v3.reinit(mgdof.locally_owned_mg_dofs(level), MPI_COMM_WORLD);
          for (unsigned int i = 0; i < v1.locally_owned_size(); ++i)
            v1.local_element(i) = random_value<double>();
          v1_cpy = v1;
          transfer.prolongate(level, v2, v1);
          transfer_ref.prolongate(level, v3, v1_cpy);
          v2_cpy = v2;
          v3 -= v2_cpy;
          deallog << "Diff prolongate   l" << level << ": "
                  << filter_out_small_numbers(v3.l2_norm(), tolerance)
                  << std::endl;
        }

      // check restriction for all levels using random vector
      for (unsigned int level = 1;
           level < mgdof.get_triangulation().n_global_levels();
           ++level)
        {
          LinearAlgebra::distributed::Vector<Number> v1, v2;
          LinearAlgebra::distributed::Vector<double> v1_cpy, v2_cpy, v3;
          v1.reinit(mgdof.locally_owned_mg_dofs(level), MPI_COMM_WORLD);
          v2.reinit(mgdof.locally_owned_mg_dofs(level - 1), MPI_COMM_WORLD);
          v3.reinit(mgdof.locally_owned_mg_dofs(level - 1), MPI_COMM_WORLD);
          for (unsigned int i = 0; i < v1.locally_owned_size(); ++i)
            v1.local_element(i) = random_value<double>();
          v1_cpy = v1;
          transfer.restrict_and_add(level, v2, v1);
          transfer_ref.restrict_and_add(level, v3, v1_cpy);
          v2_cpy = v2;
          v3 -= v2_cpy;
          deallog << "Diff restrict     l" << level << ": "
                  << filter_out_small_numbers(v3.l2_norm(), tolerance)
                  << std::endl;

          v2 = 1.;
          v3 = 1.;
          transfer.restrict_and_add(level, v2, v1);
          transfer_ref.restrict_and_add(level, v3, v1_cpy);
          v2_cpy = v2;
          v3 -= v2_cpy;
          deallog << "Diff restrict add l" << level << ": "
                  << filter_out_small_numbers(v3.l2_norm(), tolerance)
                  << std::endl;
        }
      deallog << std::endl;
    }
}


int
main(int argc, char **argv)
{
  // no threading in this test...
  Utilities::MPI::MPI_InitFinalize mpi(argc, argv, 1);
  mpi_initlog();

  check<2, double>(1);
  check<2, double>(3);
  check<3, double>(1);
  check<3, double>(3);
  check<2, float>(2);
  check<3, float>(2);
}
