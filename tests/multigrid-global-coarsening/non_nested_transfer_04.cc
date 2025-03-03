// ---------------------------------------------------------------------
//
// Copyright (C) 2020 - 2023 by the deal.II authors
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

// Test the non nested transfer for multiple components

#include <deal.II/base/conditional_ostream.h>
#include <deal.II/base/function.h>
#include <deal.II/base/logstream.h>
#include <deal.II/base/mpi.h>
#include <deal.II/base/quadrature_lib.h>

#include <deal.II/distributed/tria.h>

#include <deal.II/dofs/dof_handler.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_tools.h>
#include <deal.II/fe/mapping_q.h>

#include <deal.II/grid/grid_generator.h>

#include <deal.II/matrix_free/matrix_free.h>

#include <deal.II/multigrid/mg_transfer_global_coarsening.h>

#include "mg_transfer_util.h"

using namespace dealii;

template <int dim, typename Number>
void
do_test(const FiniteElement<dim>    &fe_fine,
        const FiniteElement<dim>    &fe_coarse,
        const Function<dim, Number> &function)
{
  // create coarse grid
  parallel::distributed::Triangulation<dim> tria_coarse(MPI_COMM_WORLD);
  GridGenerator::hyper_cube(tria_coarse);
  tria_coarse.refine_global();

  // create fine grid
  parallel::distributed::Triangulation<dim> tria_fine(MPI_COMM_WORLD);
  GridGenerator::hyper_cube(tria_fine);
  tria_fine.refine_global();
  tria_fine.refine_global();

  // setup dof-handlers
  DoFHandler<dim> dof_handler_fine(tria_fine);
  dof_handler_fine.distribute_dofs(fe_fine);

  DoFHandler<dim> dof_handler_coarse(tria_coarse);
  dof_handler_coarse.distribute_dofs(fe_coarse);

  // setup constraint matrix
  AffineConstraints<Number> constraint_coarse;
  constraint_coarse.close();

  AffineConstraints<Number> constraint_fine;
  constraint_fine.close();

  // setup transfer operator
  MGTwoLevelTransferNonNested<dim, LinearAlgebra::distributed::Vector<Number>>
                 transfer;
  MappingQ1<dim> mapping_fine, mapping_coarse;
  transfer.reinit(dof_handler_fine,
                  dof_handler_coarse,
                  mapping_fine,
                  mapping_coarse,
                  constraint_fine,
                  constraint_coarse);

  test_non_nested_transfer(transfer,
                           dof_handler_fine,
                           dof_handler_coarse,
                           function);
}

template <int dim, typename Number>
void
test(int fe_degree, const Function<dim, Number> &function)
{
  const auto str_fine   = std::to_string(fe_degree);
  const auto str_coarse = std::to_string(fe_degree);

  if (fe_degree > 0)
    {
      deallog.push("CG<2>(" + str_fine + ")<->CG<2>(" + str_coarse + ")");
      do_test<dim, double>(FESystem<dim>(FE_Q<dim>(fe_degree), dim),
                           FESystem<dim>(FE_Q<dim>(fe_degree), dim),
                           function);
      deallog.pop();
    }


  if (fe_degree > 0)
    {
      deallog.push("DG<2>(" + str_fine + ")<->CG<2>(" + str_coarse + ")");
      do_test<dim, double>(FESystem<dim>(FE_DGQ<dim>(fe_degree) ^ dim),
                           FESystem<dim>(FE_Q<dim>(fe_degree) ^ dim),
                           function);
      deallog.pop();
    }


  {
    deallog.push("DG<2>(" + str_fine + ")<->DG<2>(" + str_coarse + ")");
    do_test<dim, double>(FESystem<dim>(FE_DGQ<dim>(fe_degree) ^ dim),
                         FESystem<dim>(FE_DGQ<dim>(fe_degree) ^ dim),
                         function);
    deallog.pop();
  }
}

int
main(int argc, char **argv)
{
  Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, 1);
  MPILogInitAll                    all;

  deallog.precision(8);

  static constexpr unsigned int          dim = 2;
  Functions::ConstantFunction<2, double> constant(1., dim);

  for (unsigned int i = 0; i < 3; ++i)
    test<2, double>(i, constant);
}
