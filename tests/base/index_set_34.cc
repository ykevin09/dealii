// ---------------------------------------------------------------------
//
// Copyright (C) 2009 - 2018 by the deal.II authors
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


// test IndexSet::is_subset_of()

#include <deal.II/base/index_set.h>

#include "../tests.h"


void
test()
{
  // Create an index set that has about 30% of its possible indices:
  const unsigned int N = 1000;
  IndexSet           index_set(N);
  for (unsigned int i = 0; i < N / 3; ++i)
    index_set.add_index(random_value<unsigned int>(0, N - 1));
  index_set.compress();

  // Test that the set is a subset of itself:
  Assert(index_set.is_subset_of(index_set), ExcInternalError());

  // Now take out one value after the other in some random order, and
  // make sure that the result is a subset of the original set (and
  // that the original one is *not* a subset (because it must be a
  // superset):
  const IndexSet original = index_set;
  while (index_set.is_empty() == false)
    {
      IndexSet to_remove(N);
      to_remove.add_index(index_set.nth_index_in_set(
        random_value<unsigned int>(0, index_set.n_elements() - 1)));
      to_remove.compress();

      index_set.subtract_set(to_remove);

      Assert(index_set.is_subset_of(original) == true, ExcInternalError());
      Assert(original.is_subset_of(index_set) == false, ExcInternalError());
    }

  deallog << "OK" << std::endl;
}



int
main()
{
  initlog();

  test();
}
