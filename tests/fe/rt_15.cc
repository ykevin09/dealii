// ---------------------------------------------------------------------
//
// Copyright (C) 2003 - 2023 by the deal.II authors
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


// Like rt_13, but use FESubfaceValues
//
// the test used to fail because of the issue with computing the
// normals using FEFaceValue, where FEFaceValue by accident uses the
// *face* mapping, not the *cell* mapping to compute the Piola
// transform (leading to a missing power of h in the determinant)

#include <deal.II/base/quadrature_lib.h>

#include <deal.II/dofs/dof_accessor.h>

#include <deal.II/fe/fe_raviart_thomas.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/lac/precondition.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/sparsity_pattern.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/vector_memory.h>

#include <string>
#include <vector>

#include "../tests.h"

#define PRECISION 8


template <int dim>
void
test(const unsigned int degree)
{
  FE_RaviartThomas<dim> fe_rt(degree);

  deallog << "Degree=" << degree << std::endl;

  for (double h = 1; h > 1. / 128; h /= 2)
    {
      deallog << "  h=" << h << std::endl;

      Triangulation<dim> tr;
      GridGenerator::hyper_cube(tr, 0., h);

      DoFHandler<dim> dof(tr);
      dof.distribute_dofs(fe_rt);

      QTrapezoid<dim - 1> quadrature;

      FESubfaceValues<dim> fe_values(fe_rt, quadrature, update_gradients);
      fe_values.reinit(dof.begin_active(), 0, 0);
      for (unsigned int q = 0; q < quadrature.size(); ++q)
        {
          deallog << "    Quadrature point " << q << ": ";
          for (unsigned int i = 0; i < fe_rt.dofs_per_cell; ++i)
            {
              deallog << "[ ";
              for (unsigned int c = 0; c < fe_rt.n_components(); ++c)
                deallog << fe_values.shape_grad_component(i, q, c) << ' ';
              deallog << ']';
            }
          deallog << std::endl;
        }
    }
}


int
main()
{
  initlog();
  deallog << std::setprecision(PRECISION);
  deallog << std::fixed;

  for (unsigned int i = 0; i < 4; ++i)
    test<2>(i);

  return 0;
}
