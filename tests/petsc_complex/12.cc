// ---------------------------------------------------------------------
//
// Copyright (C) 2004 - 2018 by the deal.II authors
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



// check PETScWrappers::MPI::Vector::operator() in set-mode

#include <deal.II/lac/petsc_vector.h>

#include <iostream>
#include <vector>

#include "../tests.h"



void
test(PETScWrappers::MPI::Vector &v)
{
  deallog << "Complex test" << std::endl;

  // set only certain elements of the vector. have a bit pattern of
  // where we actually wrote elements to
  std::vector<bool> pattern(v.size(), false);
  for (unsigned int k = 0; k < v.size(); k += 1 + k)
    {
      v(k)       = std::complex<double>(0., k);
      pattern[k] = true;
    }

  v.compress(VectorOperation::insert);

  // check that they are ok, and this time all of them
  for (unsigned int k = 0; k < v.size(); ++k)
    {
      const PetscScalar el = v(k);
      AssertThrow(((pattern[k] == true) && (PetscRealPart(el) == 0.) &&
                   (PetscImaginaryPart(el) == k)) ||
                    ((pattern[k] == false) && (PetscRealPart(el) == 0.) &&
                     (PetscImaginaryPart(el) == 0.)),
                  ExcInternalError());
    }

  deallog << "OK" << std::endl;
}


int
main(int argc, char **argv)
{
  initlog();
  deallog.depth_console(0);

  try
    {
      Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, 1);
      {
        PETScWrappers::MPI::Vector v(MPI_COMM_WORLD, 100, 100);
        test(v);
      }
    }
  catch (const std::exception &exc)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Exception on processing: " << std::endl
                << exc.what() << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;

      return 1;
    }
  catch (...)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Unknown exception!" << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    };
}
