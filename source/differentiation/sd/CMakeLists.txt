## ---------------------------------------------------------------------
##
## Copyright (C) 2017 - 2023 by the deal.II authors
##
## This file is part of the deal.II library.
##
## The deal.II library is free software; you can use it, redistribute
## it, and/or modify it under the terms of the GNU Lesser General
## Public License as published by the Free Software Foundation; either
## version 2.1 of the License, or (at your option) any later version.
## The full text of the license can be found in the file LICENSE at
## the top level of the deal.II distribution.
##
## ---------------------------------------------------------------------

set(_src
  symengine_math.cc
  symengine_number_types.cc
  symengine_number_visitor_internal.cc
  symengine_optimizer.cc
  symengine_scalar_operations.cc
  symengine_tensor_operations.cc
  symengine_types.cc
  symengine_utilities.cc
  )

set(_inst
  symengine_number_visitor_internal.inst.in
  symengine_optimizer.inst.in
  symengine_tensor_operations.inst.in
  )

file(GLOB _header
  ${CMAKE_SOURCE_DIR}/include/deal.II/differentiation/sd/*.h
  )

define_object_library(object_differentiation_sd OBJECT ${_src} ${_header} ${_inst})
expand_instantiations(object_differentiation_sd "${_inst}")
