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



// check ChunkSparseMatrix::matrix_scalar_product

#include <deal.II/lac/chunk_sparse_matrix.h>
#include <deal.II/lac/vector.h>

#include <vector>

#include "../tests.h"


void
test(const unsigned int chunk_size, Vector<double> &v, Vector<double> &w)
{
  // set some entries in the
  // matrix. actually, set them all
  ChunkSparsityPattern sp(v.size(), v.size(), v.size(), chunk_size);
  for (unsigned int i = 0; i < v.size(); ++i)
    for (unsigned int j = 0; j < v.size(); ++j)
      sp.add(i, j);
  sp.compress();

  // then create a matrix from that
  ChunkSparseMatrix<double> m(sp);
  for (unsigned int i = 0; i < m.m(); ++i)
    for (unsigned int j = 0; j < m.n(); ++j)
      m.set(i, j, i + 2 * j);

  for (unsigned int i = 0; i < v.size(); ++i)
    {
      v(i) = i;
      w(i) = i + 1;
    }

  v.compress();
  w.compress();

  // <w,Mv>
  const double s = m.matrix_scalar_product(w, v);

  // make sure we get the expected result
  for (unsigned int i = 0; i < v.size(); ++i)
    {
      AssertThrow(v(i) == i, ExcInternalError());
      AssertThrow(w(i) == i + 1, ExcInternalError());
    }

  double result = 0;
  for (unsigned int i = 0; i < m.m(); ++i)
    for (unsigned int j = 0; j < m.n(); ++j)
      result += (i + 2 * j) * j * (i + 1);

  AssertThrow(s == result, ExcInternalError());

  deallog << "OK" << std::endl;
}



int
main()
{
  initlog();

  try
    {
      const unsigned int chunk_sizes[] = {1, 2, 4, 7, 11};
      for (unsigned int i = 0; i < sizeof(chunk_sizes) / sizeof(chunk_sizes[0]);
           ++i)
        {
          Vector<double> v(100);
          Vector<double> w(100);
          test(chunk_sizes[i], v, w);
        }
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
