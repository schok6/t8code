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

/** \file t8_2_5D.hxx
 * Definitions of 2.5D-specific functions. 
 */

#ifndef T8_2_5D_HXX
#define T8_2_5D_HXX

#include <t8_element.h>
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension_element_cxx.hxx>

// /** Compute the linear id of a given element in a hypothetical uniform
//  * refinement of given level for eclass1 and eclass2.
//  * \param [in] elem     The element whose id we compute.
//  * \param [in] level1   The level of eclass1 of the uniform refinement to consider.
//  * \param [in] level2   The level of eclass2 of the uniform refinement to consider.
//  * \return              The linear id of the element.
//  * 
//  */
// int
// t8_2_5D_get_linear_id (const t8_element_t *elem, int level1, int level2)

// typedef struct t8_2_5D
// {
//   int8_t level;
//   t8_dline_coord_t x;
//   t8_dline_coord_t y;
//   t8_dline_coord_t z;
// } t8_2_5D_t;

#endif /* T8_2_5D_HXX */