// ---------------------------------------------------------------------
//
// Copyright (C) 2010 - 2018 by the deal.II authors
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



// check adding elements into a matrix using
// SparseMatrix::add(row, n_cols, col_indices, values, elide_zero_values,
//                   col_indices_are_sorted)
// need to filter out zeros, indices are sorted and zero values should
// not be elided

#include <deal.II/lac/sparse_matrix.h>

#include "../tests.h"


void
test()
{
  // set up sparse matrix
  SparsityPattern sp(5, 5, 3);
  for (unsigned int i = 0; i < sp.n_rows(); ++i)
    for (unsigned int j = 0; j < sp.n_cols(); ++j)
      if ((i + 2 * j + 1) % 3 == 0)
        sp.add(i, j);
  sp.compress();

  SparseMatrix<double> m(sp);

  // prepare structure with indices and values
  std::vector<types::global_dof_index> indices(m.n());
  for (unsigned int j = 0; j < m.n(); ++j)
    indices[j] = j;
  std::vector<double> values(m.n());

  // try to add entries from the list. Zeros
  // should be filtered out. list is sorted
  for (unsigned int i = 0; i < m.m(); ++i)
    {
      for (unsigned int j = 0; j < m.n(); ++j)
        if ((i + 2 * j + 1) % 3 == 0)
          values[j] = i * j * .5 + .5;
        else
          values[j] = 0;
      m.add(i, m.m(), &indices[0], &values[0], false, true);
    }

  // then make sure we retrieve the same ones
  for (unsigned int i = 0; i < m.m(); ++i)
    for (unsigned int j = 0; j < m.n(); ++j)
      if ((i + 2 * j + 1) % 3 == 0)
        {
          AssertThrow(m(i, j) == i * j * .5 + .5, ExcInternalError());
        }
      else
        {
          AssertThrow(m.el(i, j) == 0, ExcInternalError());
        }

  // try to add an invalid list of indices to
  // first and last row, should throw an
  // exception
  for (unsigned int i = 0; i < m.m(); ++i)
    values[i] = 0.5 * i - 1.5;
  try
    {
      m.add(0, m.m(), &indices[0], &values[0], false, true);
    }
  catch (ExceptionBase &e)
    {
      deallog << e.get_exc_name() << std::endl;
    }

  try
    {
      m.add(m.m() - 1, m.m(), &indices[0], &values[0], false, true);
    }
  catch (ExceptionBase &e)
    {
      deallog << e.get_exc_name() << std::endl;
    }

  deallog << "OK" << std::endl;
}



int
main()
{
  deal_II_exceptions::disable_abort_on_exception();

  initlog();

  try
    {
      test();
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
}
