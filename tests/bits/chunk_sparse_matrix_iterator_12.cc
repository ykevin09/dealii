// ---------------------------------------------------------------------
//
// Copyright (C) 2004 - 2021 by the deal.II authors
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



// like chunk_sparse_matrix_iterator_11 but for const_iterator

#include <deal.II/lac/chunk_sparse_matrix.h>

#include <iterator>

#include "../tests.h"


void
test(const unsigned int chunk_size)
{
  deallog << "Chunk size: " << chunk_size << std::endl;

  // create a sparsity pattern with totally
  // empty lines (not even diagonals, since
  // not quadratic)
  ChunkSparsityPattern sparsity(4, 5, 1, chunk_size);
  sparsity.add(1, 1);
  sparsity.add(3, 1);
  sparsity.compress();

  // attach a sparse matrix to it
  ChunkSparseMatrix<double> A(sparsity);

  ChunkSparseMatrix<double>::const_iterator k = A.begin(),
                                            j = std::next(A.begin());

  AssertThrow(k < j, ExcInternalError());
  AssertThrow(j > k, ExcInternalError());

  AssertThrow(!(j < k), ExcInternalError());
  AssertThrow(!(k > j), ExcInternalError());

  AssertThrow(k != j, ExcInternalError());
  AssertThrow(!(k == j), ExcInternalError());

  AssertThrow(k == k, ExcInternalError());
  AssertThrow(!(k != k), ExcInternalError());

  deallog << "OK" << std::endl;
}



int
main()
{
  initlog();

  try
    {
      test(1);
      test(3);
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
