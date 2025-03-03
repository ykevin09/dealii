// ---------------------------------------------------------------------
//
// Copyright (C) 2013 - 2018 by the deal.II authors
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



// check BlockSparseMatrix::vmult, Tvmult with deal.II vector/block vector
// combination

#include <deal.II/base/utilities.h>

#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/trilinos_block_sparse_matrix.h>

#include <iostream>
#include <vector>

#include "../tests.h"


void
test(const unsigned int size_1, const unsigned int size_2)
{
  TrilinosWrappers::SparseMatrix m(size_1, size_2, size_2);
  for (unsigned int i = 0; i < m.m(); ++i)
    for (unsigned int j = 0; j < m.n(); ++j)
      m.set(i, j, i + 2 * j);
  m.compress(VectorOperation::insert);

  TrilinosWrappers::BlockSparseMatrix m_block;
  m_block.reinit(1, 2);
  m_block.block(0, 0).copy_from(m);
  m_block.block(0, 1).copy_from(m);
  m_block.collect_sizes();

  BlockVector<double> v(2);
  v.block(0).reinit(size_2);
  v.block(1).reinit(size_2);
  v.collect_sizes();
  Vector<double> w(size_1);
  for (unsigned int i = 0; i < v.size(); ++i)
    v(i) = i;

  // w:=Mv
  m_block.vmult(w, v);

  // make sure we get the expected result
  for (unsigned int i = 0; i < m.m(); ++i)
    {
      double result = 0;
      for (unsigned int j = 0; j < m.n(); ++j)
        result += (i + 2 * j) * j;
      for (unsigned int j = 0; j < m.n(); ++j)
        result += (i + 2 * j) * (j + m.n());
      AssertThrow(w(i) == result, ExcInternalError());
    }

  for (unsigned int i = 0; i < w.size(); ++i)
    w(i) = i;

  m_block.Tvmult(v, w);
  // make sure we get the expected result
  for (unsigned int i = 0; i < m.n(); ++i)
    {
      double result = 0;
      for (unsigned int j = 0; j < m.m(); ++j)
        result += (j + 2 * i) * j;
      AssertThrow(v(i) == result, ExcInternalError());
      AssertThrow(v(i + m.n()) == result, ExcInternalError());
    }

  deallog << "OK" << std::endl;
}



int
main(int argc, char **argv)
{
  initlog();

  Utilities::MPI::MPI_InitFinalize mpi_initialization(
    argc, argv, testing_max_num_threads());


  try
    {
      test(50, 47);
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
