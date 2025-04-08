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
#include <t8_refcount.h>
#include <t8_eclass.h>
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension_element.hxx>
#include <t8_schemes/t8_default/t8_default.hxx>
#include <t8_schemes/t8_default/t8_default_vertex/t8_default_vertex.hxx>
#include <t8_schemes/t8_default/t8_default_line/t8_default_line.hxx>
#include <t8_schemes/t8_default/t8_default_quad/t8_default_quad.hxx>
#include <t8_schemes/t8_default/t8_default_tri/t8_default_tri.hxx>
#include <t8_schemes/t8_default/t8_default_tet/t8_default_tet.hxx>
#include <t8_schemes/t8_scheme_builder.hxx>

/*t8_scheme_new_2_5dimension gets ownership of scheme*/
const t8_scheme *
t8_scheme_new_2_5dimension (const t8_scheme *scheme)
{
  t8_scheme_builder builder;

  /* refcount of scheme is already one and needs to get increased to two*/
  scheme->ref ();

  builder.add_eclass_scheme<t8_default_scheme_vertex> ();
  builder.add_eclass_scheme<t8_default_scheme_line> ();
  builder.add_eclass_scheme<t8_default_scheme_quad> ();
  builder.add_eclass_scheme<t8_default_scheme_tri> ();
  builder.add_eclass_scheme<t8_2_5dimension_scheme> (scheme, T8_ECLASS_QUAD, T8_ECLASS_LINE);
  builder.add_eclass_scheme<t8_default_scheme_tet> (); //NULL
  builder.add_eclass_scheme<t8_2_5dimension_scheme> (scheme, T8_ECLASS_TRIANGLE, T8_ECLASS_LINE);
  builder.add_eclass_scheme<t8_default_scheme_pyramid> (); //NULL
  
  return builder.build_scheme ();
}