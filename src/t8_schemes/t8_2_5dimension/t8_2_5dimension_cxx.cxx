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
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension_element_cxx.hxx>
#include <t8_schemes/t8_default/t8_default_line/t8_default_line_cxx.hxx>
#include <t8_schemes/t8_default/t8_default_quad/t8_default_quad_cxx.hxx>
#include <t8_schemes/t8_default/t8_default_tri/t8_default_tri_cxx.hxx>
#include <t8_schemes/t8_default/t8_default_pyramid/t8_default_pyramid_cxx.hxx>

/** Return the 2_5dimension element implementation of t8code. */
t8_scheme_comb_cxx_t *
/**TODO:
*doppelter Pointer (array ist bereits Pointer) auf array von schemes -> alles außer Kombinationen
*von Line, Quad und Tet auf "nicht implementiert" setzen
*/
t8_scheme_new_2_5dimension_cxx (t8_eclass_scheme_c *scheme1, t8_eclass_scheme_c *scheme2)
{
  t8_scheme_comb_cxx_t *s;

  s = T8_ALLOC_ZERO (t8_scheme_comb_cxx_t , 1);
  //s = T8_ALLOC_ZERO ( T8_ALLOC ( T8_ALLOC (t8_scheme_comb_cxx_t[1] , 1), 1), 1);
  t8_refcount_init (&s->rc);
  s->eclass_schemes_comb[scheme1->eclass][scheme2->eclass] = 
    new t8_2_5dimension_scheme_c(scheme1, scheme2);
  // s->eclass_schemes_comb[T8_ECLASS_LINE][T8_ECLASS_QUAD] = 
  //   new t8_2_5dimension_scheme_c(T8_ECLASS_LINE, T8_ECLASS_QUAD) ();
  //Abort bei constructor
  // s->eclass_schemes_comb[T8_ECLASS_PYRAMID][T8_ECLASS_QUAD] = 
  //   new t8_2_5dimension_scheme_c(T8_ECLASS_PYRAMID, T8_ECLASS_QUAD) ();
  // s->eclass_schemes_comb[T8_ECLASS_LINE][T8_ECLASS_TRIANGLE] = 
  //   new t8_2_5dimension_scheme_c(T8_ECLASS_LINE, T8_ECLASS_TRIANGLE) ();
  // s->eclass_schemes_comb[T8_ECLASS_QUAD][T8_ECLASS_LINE] = 
  //   new t8_2_5dimension_scheme_c(T8_ECLASS_QUAD, T8_ECLASS_LINE) (); 
  // s->eclass_schemes_comb[T8_ECLASS_TRIANGLE][T8_ECLASS_LINE] = 
  //   new t8_2_5dimension_scheme_c(T8_ECLASS_TRIANGLE, T8_ECLASS_LINE) (); 

  return s;
}