// ---------------------------------------------------------------------
//
// Copyright (C) 2000 - 2023 by the deal.II authors
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


// Until version 1.50 of mg_dof_handler.cc, the
// DoFHandler::distribute_dofs function in 1d and 3d could not
// handle coarsened grids (unused vertices). Also, the
// DoFHandler::renumbering function could not handle coarsened grids
// (unused vertices, unused faces). Check that all this works now.

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_renumbering.h>

#include <deal.II/fe/fe_dgp.h>
#include <deal.II/fe/fe_dgq.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>

#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/vector.h>

#include <algorithm>

#include "../tests.h"



template <int dim>
void
check()
{
  FE_Q<dim> fe(3);
  deallog << fe.get_name() << std::endl;

  Triangulation<dim> tria(
    Triangulation<dim>::limit_level_difference_at_vertices);
  GridGenerator::hyper_cube(tria);
  tria.refine_global(2);
  typename Triangulation<dim>::active_cell_iterator cell = tria.begin_active();
  for (unsigned int i = 0; i < GeometryInfo<dim>::max_children_per_cell;
       ++i, ++cell)
    cell->set_coarsen_flag();
  tria.execute_coarsening_and_refinement();

  DoFHandler<dim> mg_dof_handler(tria);
  mg_dof_handler.distribute_dofs(fe);
  mg_dof_handler.distribute_mg_dofs();
  for (unsigned int level = 0; level < tria.n_levels(); ++level)
    {
      const types::global_dof_index n_dofs = mg_dof_handler.n_dofs(level);
      std::vector<types::global_dof_index> new_numbers(n_dofs);
      for (unsigned int i = 0; i < n_dofs; ++i)
        new_numbers[i] = n_dofs - 1 - i;

      mg_dof_handler.renumber_dofs(level, new_numbers);
    }
}


int
main()
{
  initlog(__FILE__);
  check<1>();
  check<2>();
  check<3>();

  deallog << "OK" << std::endl;
}
