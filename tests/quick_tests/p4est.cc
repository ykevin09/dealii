// ---------------------------------------------------------------------
//
// Copyright (C) 2008 - 2022 by the deal.II authors
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



// Test DoFTools::count_dofs_per_fe_component

// Test p4est. This test exposes a bug in OpenMPI 1.3 and 1.4 Update to
// OpenMPI 1.5 or newer.


#include <deal.II/base/tensor.h>
#include <deal.II/base/utilities.h>

#include <deal.II/distributed/tria.h>

#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_dgq.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/intergrid_map.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <numeric>

using namespace dealii;

template <int dim>
void
test()
{
  parallel::distributed::Triangulation<dim> triangulation(
    MPI_COMM_WORLD, Triangulation<dim>::limit_level_difference_at_vertices);

  FESystem<dim> fe(FE_Q<dim>(3), 2, FE_DGQ<dim>(1), 1);

  DoFHandler<dim> dof_handler(triangulation);

  GridGenerator::hyper_cube(triangulation);
  triangulation.refine_global(2);
  dof_handler.distribute_dofs(fe);

  const std::vector<types::global_dof_index> dofs_per_component =
    DoFTools::count_dofs_per_fe_component(dof_handler);

  Assert(std::accumulate(dofs_per_component.begin(),
                         dofs_per_component.end(),
                         0U) == dof_handler.n_dofs(),
         ExcInternalError());

  const unsigned int myid = Utilities::MPI::this_mpi_process(MPI_COMM_WORLD);
  if (myid == 0)
    {
      deallog << "Total number of dofs: " << dof_handler.n_dofs() << std::endl;
      for (unsigned int i = 0; i < dofs_per_component.size(); ++i)
        deallog << "Component " << i << " has " << dofs_per_component[i]
                << " global dofs" << std::endl;
    }
}


int
main(int argc, char *argv[])
{
  Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, 1);

  test<2>();
  test<3>();

  return 0;
}
