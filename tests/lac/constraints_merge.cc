// ---------------------------------------------------------------------
//
// Copyright (C) 1998 - 2023 by the deal.II authors
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



// merge and print a bunch of ConstrainMatrices

#include <deal.II/lac/affine_constraints.h>

#include "../tests.h"



void
merge_check()
{
  deallog << "Checking AffineConstraints<double>::merge" << std::endl;

  // check twice, once with closed
  // objects, once with open ones
  for (unsigned int run = 0; run < 2; ++run)
    {
      deallog << "Checking with " << (run == 0 ? "open" : "closed")
              << " objects" << std::endl;

      // check that the `merge' function
      // works correctly
      AffineConstraints<double> c1, c2;

      // enter simple line
      c1.add_line(0);
      c1.add_entry(0, 11, 1.);
      // add more complex line
      c1.add_line(1);
      c1.add_entry(1, 3, 0.5);
      c1.add_entry(1, 4, 0.5);

      // fill second constraints
      // object with one trivial line
      // and one which further
      // constrains one of the
      // entries in the first object
      c2.add_line(10);
      c2.add_entry(10, 11, 1.);
      c2.add_line(3);
      c2.add_entry(3, 12, 0.25);
      c2.add_entry(3, 13, 0.75);

      // in one of the two runs,
      // close the objects
      if (run == 1)
        {
          c1.close();
          c2.close();
        };

      // now merge the two and print the
      // results
      c1.merge(c2);
      c1.print(deallog.get_file_stream());
    };
}


int
main()
{
  initlog();
  deallog << std::setprecision(2);
  deallog.get_file_stream() << std::setprecision(2);

  merge_check();
}
