/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element types in parallel.

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

#include <t8.h>                                           /* General t8code header, always include this. */
#include <t8_forest/t8_forest_general.h>                  /* forest definition and basic interface. */
#include <t8_forest/t8_forest_io.h>                       /* save forest */
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx> /* 2_5D refinement scheme. */
#include <t8_schemes/t8_2_5dimension/t8_mixed_scheme.hxx>
#include <t8_forest/t8_forest_types.h>
#include <tutorials/2_5D/t8_2_5D_vtu.hxx>

/* Write the forest as vtu and also write the highlighted element in the file.
 * 
 * t8code supports writing element based data to vtu as long as its stored
 * as doubles. Each of the data fields to write has to be provided in its own
 * array of length num_local_elements.
 * We support two types: T8_VTK_SCALAR - One double per element
 *                  and  T8_VTK_VECTOR - 3 doubles per element
 */

void
t8_2_5D_output_data_to_vtu (t8_forest_t forest, int level1, int level2, double *array, const char *prefix)
{
  t8_locidx_t num_elements = t8_forest_get_global_num_leaf_elements (forest);
  t8_locidx_t ielem;
  /* We need to allocate a new array to store the data on their own.
   * The arrays have one entry per local element. */
  double *highlight = T8_ALLOC (double, num_elements);
  double *sfc_index = T8_ALLOC (double, num_elements);
  /* The number of user defined data fields to write. */
  int num_data = 2;
  /* For each user defined data field we need one t8_vtk_data_field_t variable */
  t8_vtk_data_field_t vtk_data[num_data];
  /* Set the type of this variable. Since we have for each array one value per element, we pick T8_VTK_SCALAR */
  vtk_data[0].type = T8_VTK_SCALAR;
  /* The name of the field as should be written to the file. */
  strcpy (vtk_data[0].description, "highlight");
  vtk_data[0].data = highlight;
  /* Copy the element's height from our data array to the output array. */
  for (ielem = 0; ielem < num_elements; ++ielem) {
    highlight[ielem] = array[ielem];
  }

  vtk_data[1].type = T8_VTK_SCALAR;
  /* The name of the field as should be written to the file. */
  strcpy (vtk_data[1].description, "SFC index");
  vtk_data[1].data = sfc_index;
  /* Copy the element's height from our data array to the output array. */
  t8_tree_t tree;
  t8_locidx_t itree;
  t8_locidx_t element_index, elems_in_tree;
  t8_locidx_t element_index_in_tree;
  t8_locidx_t elems_considered;
  t8_locidx_t num_global_trees;
  t8_locidx_t num_local_trees;
  t8_element_t *element;
  const t8_mixed_scheme *scheme;
  num_global_trees = t8_forest_get_num_global_trees (forest);
  element_index = 0;
  element_index_in_tree = 0;
  elems_considered = 0;

  for (itree = 0; itree < num_global_trees; itree++) {
    /* Get the tree that stores the elements */
    num_local_trees = t8_forest_get_num_local_trees (forest);
    if (itree < num_local_trees) {
      tree = t8_forest_get_tree (forest, itree);
      /* Get the eclass scheme of the tree */
      scheme = t8_forest_get_scheme_2_5D (forest);
      const t8_eclass_t tree_class = t8_forest_get_tree_class (forest, itree);
      elems_in_tree = (t8_locidx_t) t8_element_array_get_count (&tree->leaf_elements);
      element_index_in_tree = elems_in_tree;
      for (element_index = 0; element_index < element_index_in_tree; element_index++) {
        /* Get a pointer to the element */
        element = t8_forest_get_leaf_element (forest, tree->elements_offset + element_index, &itree);

        std::vector<int> levels = { level1, level2 };

        sfc_index[element_index + elems_considered] = (scheme->element_get_linear_id (tree_class, element, levels));
      }
      elems_considered += elems_in_tree;
    }
  }

  {
    /* To write user defined data, we need to extended output function t8_forest_vtk_write_file
     * from t8_forest_vtk.h. Despite writing user data, it also offers more control over which 
     * properties of the forest to write. */
    int write_treeid = 1;
    int write_mpirank = 1;
    int write_level = 1;
    int write_element_id = 1;
    int write_ghosts = 0;
    t8_forest_write_vtk_ext (forest, prefix, write_treeid, write_mpirank, write_level, write_element_id, write_ghosts,
                             0, 0, num_data, vtk_data);
  }
  /* clean-up */
  T8_FREE (highlight);
  T8_FREE (sfc_index);
}
