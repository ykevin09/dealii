// ---------------------------------------------------------------------
//
// Copyright (C) 2002 - 2018 by the deal.II authors
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



// apply SparsityTools::reorder_Cuthill_McKee to the cell connectivity
// graph for the mesh used in gerold_2. apparently the mesh consists
// of two or more non-connected parts, and the reordering algorithm
// trips over that

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_in.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/manifold_lib.h>
#include <deal.II/grid/tria.h>

#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/sparsity_tools.h>

#include "../tests.h"


template <int dim>
class LaplaceProblem
{
public:
  void
  run();

private:
  Triangulation<dim> triangulation;
};


template <int dim>
void
LaplaceProblem<dim>::run()
{
  GridIn<dim> grid_in;
  grid_in.attach_triangulation(triangulation);

  std::ifstream input_file(SOURCE_DIR "/gerold_1.inp");
  grid_in.read_ucd(input_file);

  DynamicSparsityPattern cell_connectivity;
  GridTools::get_face_connectivity_of_cells(triangulation, cell_connectivity);
  std::vector<types::global_dof_index> permutation(
    triangulation.n_active_cells());
  SparsityTools::reorder_Cuthill_McKee(cell_connectivity, permutation);

  for (unsigned int i = 0; i < permutation.size(); ++i)
    deallog << permutation[i] << std::endl;
}


int
main()
{
  initlog();

  try
    {
      LaplaceProblem<3> laplace_problem_3d;
      laplace_problem_3d.run();
    }
  catch (const std::exception &exc)
    {
      deallog << std::endl
              << std::endl
              << "----------------------------------------------------"
              << std::endl;
      deallog << "Exception on processing: " << std::endl
              << exc.what() << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------"
              << std::endl;
      return 1;
    }
  catch (...)
    {
      deallog << std::endl
              << std::endl
              << "----------------------------------------------------"
              << std::endl;
      deallog << "Unknown exception!" << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------"
              << std::endl;
      return 1;
    };

  return 0;
}
