/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2015 the developers

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <new>
#include <memory>
#include <t8_refcount.h>
#include <t8_eclass.h>
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx>
// #include <t8_schemes/t8_2_5dimension/t8_2_5dimension_element.hxx>
#include <t8_schemes/t8_2_5dimension/t8_mixed_scheme.hxx>

#include <t8_schemes/t8_default/t8_default.hxx>
#include <t8_schemes/t8_default/t8_default_vertex/t8_default_vertex.hxx>
#include <t8_schemes/t8_default/t8_default_line/t8_default_line.hxx>
#include <t8_schemes/t8_default/t8_default_quad/t8_default_quad.hxx>
#include <t8_schemes/t8_default/t8_default_tri/t8_default_tri.hxx>
#include <t8_schemes/t8_default/t8_default_tet/t8_default_tet.hxx>
#include <t8_schemes/t8_default/t8_default_pyramid/t8_default_pyramid.hxx>
// #include <t8_schemes/t8_scheme_builder.hxx>

template <class TUnderlyingEclassScheme1, typename TUnderlyingElementType1, class TUnderlyingEclassScheme2,
          typename TUnderlyingElementType2>
class t8_2_5dimension_scheme;

// const t8_mixed_scheme *
const t8_scheme *
t8_scheme_new_2_5dimension ()
{

  t8_mixed_scheme_builder builder;
  // t8_scheme_builder builder;

  builder.add_eclass_scheme_mixed<invalid_scheme> ();
  builder.add_eclass_scheme_mixed<invalid_scheme> ();
  /* 2.5D for QUAD */
  builder.add_eclass_scheme_mixed<t8_2_5dimension_scheme<line_class1, t8_dline_t, line_class2, t8_dline_t>> ();
  builder.add_eclass_scheme_mixed<invalid_scheme> ();
  /* 2.5D for HEX */
  builder.add_eclass_scheme_mixed<
    t8_2_5dimension_scheme<t8_default_scheme_quad, t8_pquad_t, t8_default_scheme_line, t8_dline_t>> ();
  builder.add_eclass_scheme_mixed<invalid_scheme> ();
  /* 2.5D for PRISM */
  builder.add_eclass_scheme_mixed<
    t8_2_5dimension_scheme<t8_default_scheme_tri, t8_dtri_t, t8_default_scheme_line, t8_dline_t>> ();
  builder.add_eclass_scheme_mixed<invalid_scheme> ();

  return builder.build_mixed_scheme ();

  // return (t8_scheme *) builder.build_mixed_scheme ();
  // return builder.build_scheme ();
}
