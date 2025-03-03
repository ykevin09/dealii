//-----------------------------------------------------------
//
//    Copyright (C) 2017 - 2023 by the deal.II authors
//
//    This file is part of the deal.II library.
//
//    The deal.II library is free software; you can use it, redistribute
//    it, and/or modify it under the terms of the GNU Lesser General
//    Public License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//    The full text of the license can be found in the file LICENSE.md at
//    the top level directory of deal.II.
//
//-----------------------------------------------------------

#include <deal.II/base/parameter_handler.h>

#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/vector.h>

#include <deal.II/sundials/kinsol.h>

#include "../tests.h"

// Solve a nonlinear system.
//
// Similar to the _02 test, but we're now actually providing a solver with
// the Jacobian matrix. For the current case,
//
//   F(u) = [  cos(u1 + u2) - 1 + 2*u1  ]
//          [  sin(u1 - u2)     + 2*u2   ]
//
// the Jacobian is the 2x2 matrix
//
//   J(u) = [ -sin(u1 + u2) + 2      -sin(u1 + u2)]
//          [  cos(u1 - u2)          -cos(u1 - u2) + 2]
//
// The addition of the +2u_i to the function F does not move the solution
// (it is still u=0) but it makes sure that the Jacobian at the solution
// remains non-singular
//
// The _03 test case has a flaw in that starting at SUNDIALS 4.x, the solve
// function no longer receives the current 'u' vector. This means that
// one can't compute a proper Jacobian in that function because we don't
// know what to linearize around. This test fixes this by computing
// the Jacobian in the setup function.
//
// It turns out that doing this leads to a *larger* number of
// iterations than the kludge in _03, but that is because _03
// linearizes around the zero vector which just so also happens to be
// solution of the problem. In other words, it accidentally has the
// perfect Jacobian matrix to use, and consequently converges rapidly,
// whereas we here have to deal with a poor Jacobian for the first few
// iterations. Furthermore, because we don't update the Jacobian very
// frequently, we are stuck with the poor Jacobian for numerous
// iterations. The _05 test therefore forces updates in every
// iteration and, unsurprisingly, converges much quicker.

int
main()
{
  initlog();

  using VectorType = Vector<double>;

  SUNDIALS::KINSOL<VectorType>::AdditionalData data;
  ParameterHandler                             prm;
  data.add_parameters(prm);

  std::ifstream ifile(SOURCE_DIR "/kinsol_linesearch.prm");
  prm.parse_input(ifile);

  // Size of the problem
  const unsigned int N = 2;

  SUNDIALS::KINSOL<VectorType> kinsol(data);

  kinsol.reinit_vector = [N](VectorType &v) { v.reinit(N); };

  kinsol.residual = [](const VectorType &u, VectorType &F) {
    deallog << "Evaluating the solution at u=(" << u[0] << ',' << u[1] << ')'
            << std::endl;

    F(0) = std::cos(u[0] + u[1]) - 1 + 2 * u[0];
    F(1) = std::sin(u[0] - u[1]) + 2 * u[1];
  };


  kinsol.iteration_function = [](const VectorType &u, VectorType &F) {
    // We want a Newton-type scheme, not a fixed point iteration. So we
    // shouldn't get into this function.
    std::abort();

    // But if anyone wanted to see how it would look like:
    F(0) = std::cos(u[0] + u[1]) - 1 + 2 * u[0] - u[0];
    F(1) = std::sin(u[0] - u[1]) + 2 * u[1] - u[1];
  };

  FullMatrix<double> J_inverse(2, 2);

  kinsol.setup_jacobian = [&J_inverse](const VectorType &u,
                                       const VectorType &F) {
    deallog << "Setting up Jacobian system at u=(" << u[0] << ',' << u[1] << ')'
            << std::endl;

    FullMatrix<double> J(2, 2);
    J(0, 0) = -std::sin(u[0] + u[1]) + 2;
    J(0, 1) = -std::sin(u[0] + u[1]);
    J(1, 0) = std::cos(u[0] - u[1]);
    J(1, 1) = -std::cos(u[0] - u[1]) + 2;

    J_inverse.invert(J);
  };


  kinsol.solve_with_jacobian =
    [&J_inverse](const VectorType &rhs, VectorType &dst, double) {
      deallog << "Solving Jacobian system with rhs=(" << rhs[0] << ',' << rhs[1]
              << ')' << std::endl;

      J_inverse.vmult(dst, rhs);
    };

  VectorType v(N);
  v(0) = 0.5;
  v(1) = 1.234;

  auto niter = kinsol.solve(v);
  v.print(deallog.get_file_stream());
  deallog << "Converged in " << niter << " iterations." << std::endl;
}
