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

/** file t8_2_5D_adapt.hxx
 * This is the header file to the step3 example of t8code. It collects
 * functions of t8_step3 that we reuse in other examples.
 * In this example we discuss how to adapt a forest.
 * The main program is t8_2_5D_adapt_main.
 * See \ref t8_2_5D_adapt_forest.cxx for more details.
 */

#ifndef T8_2_5D_VTU_HXX
#define T8_2_5D_VTU_HXX

#include <t8.h>                          /* General t8code header, always include this. */
#include <t8_forest/t8_forest_general.h> /* forest definition and basic interface. */

/* Write the biforest as vtu and also write the highlighted element in the file.
 * 
 * t8code supports writing element based data to vtu as long as its stored
 * as doubles. Each of the data fields to write has to be provided in its own
 * array of length num_local_elements.
 * We support two types: T8_VTK_SCALAR - One double per element
 *                  and  T8_VTK_VECTOR - 3 doubles per element
 * 
 * \param [in] forest     The biforest to write to a vtu/pvtu file.
 * \param [in] level1     The horizonzal refinement level.
 * \param [in] level2     The vertical refinement level.
 * \param [in] array      The array of the considered data.
 * \param [in] prefix     The name of the returned vtu/pvtu file.
 * \return  A vtu/pvtu file including the data contained in array and the SFC indices with the given name of the considered biforest.
 */
void
t8_2_5D_output_data_to_vtu (t8_forest_t forest, int level1, int level2, double *array, const char *prefix);

#endif /* !T8_2_5D_VTU_HXX */