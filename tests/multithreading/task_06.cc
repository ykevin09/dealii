// ---------------------------------------------------------------------
//
// Copyright (C) 2009 - 2020 by the deal.II authors
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


// make sure we can start tasks from individual threads. this requires that a
// task scheduler object is running on each thread we create

#include <deal.II/base/thread_management.h>

#include "../tests.h"


void
test(int i)
{
  deallog << "Task " << i << " starting..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));
  if (i < 10)
    {
      Threads::new_task(test, 10 + i).join();
    }
  deallog << "Task " << i << " finished!" << std::endl;
}



int
main()
{
  initlog();

  {
    std::thread t1(test, 1);
    std::thread t2(test, 2);

    t1.join();
    t2.join();

    deallog << "OK" << std::endl;
  }

  std::ofstream *out_stream =
    dynamic_cast<std::ofstream *>(&deallog.get_file_stream());
  Assert(out_stream != nullptr, ExcInternalError());
  deallog.detach();
  out_stream->close();
  sort_file_contents("output");
}
