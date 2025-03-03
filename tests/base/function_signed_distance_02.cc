// ---------------------------------------------------------------------
//
// Copyright (C) 2007 - 2022 by the deal.II authors
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

/**
 * Test the class Functions::SignedDistance::Ellipsoid by creating an object of
 * the class and calling the value function at points where we know what the
 * function values should be.
 */

#include <deal.II/base/function_signed_distance.h>

#include "../tests.h"

namespace
{
  template <int dim>
  void
  print_value_and_gradient_at_point(const Function<dim> &function,
                                    const Point<dim>    &point)
  {
    deallog << "point = " << point << std::endl;
    deallog << "value = " << function.value(point) << std::endl;
    deallog << "gradient = " << function.gradient(point) << std::endl;
  }


  void
  test_ellipsoid_signed_distance_1d()
  {
    static constexpr int dim = 1;

    deallog << "test_ellipsoid_signed_distance" << std::endl;
    deallog << "dim = " << dim << std::endl;
    deallog << std::endl;

    const Point<dim>              center(1.);
    const std::array<double, dim> radii{{2.}};

    const Functions::SignedDistance::Ellipsoid<dim> ellipsoid(center, radii);

    deallog << "center" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, center);
    deallog << "inside" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, Point<dim>(0.));
    print_value_and_gradient_at_point(ellipsoid, Point<dim>(1.5));
    deallog << "on ellipse" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, Point<dim>(-1.));
    print_value_and_gradient_at_point(ellipsoid, Point<dim>(3.));
    deallog << "outside" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, Point<dim>(-2.));
    print_value_and_gradient_at_point(ellipsoid, Point<dim>(6.));

    deallog << std::endl;
  }



  void
  test_ellipsoid_signed_distance_2d()
  {
    static constexpr int dim = 2;

    deallog << "test_ellipsoid_signed_distance" << std::endl;
    deallog << "dim = " << dim << std::endl;
    deallog << std::endl;

    const Point<dim>              center(3., 2.);
    const std::array<double, dim> radii{{1., 2.}};

    const Functions::SignedDistance::Ellipsoid<dim> ellipsoid(center, radii);

    deallog << "center" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, center);
    deallog << "inside" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, {3., 1.});
    print_value_and_gradient_at_point(ellipsoid, {3.5, 2.});
    print_value_and_gradient_at_point(ellipsoid, {3.1, 1.9});
    deallog << "on ellipse" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, {2., 2.});
    print_value_and_gradient_at_point(ellipsoid, {3., 4.});
    print_value_and_gradient_at_point(ellipsoid, {2.5, 2. + std::sqrt(3.)});
    deallog << "outside" << std::endl;
    print_value_and_gradient_at_point(ellipsoid, {3., -1.});
    print_value_and_gradient_at_point(ellipsoid, {-1., 2.});
    print_value_and_gradient_at_point(ellipsoid, {0., 0.});
    print_value_and_gradient_at_point(ellipsoid, {4., 5.});

    deallog << std::endl;
  }
} // namespace



int
main()
{
  initlog();
  test_ellipsoid_signed_distance_1d();
  test_ellipsoid_signed_distance_2d();
}
