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

/*MESSy Application als t8code-example*/

#include <t8.h>
#include <t8_cmesh.h>
#include <t8_cmesh/t8_cmesh_examples.h>
#include <t8_forest/t8_forest_general.h>
#include <t8_forest/t8_forest_io.h>
#include <t8_forest/t8_forest_geometrical.h> 
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx>
#include <t8_schemes/t8_default/t8_default.hxx>
#include <t8_types/t8_vec.h>                    /* Basic operations on 3D vectors. */
#include <netcdf.h>
#include <algorithm>
#include <cmath>
#include "t8_forest/t8_forest_types.h"

T8_EXTERN_C_BEGIN ();

/*
int main()
{
    //t8code intialisiert

    0) Dimensionlängen aus netCDF File auslesen musst: Lon: 128; Lat: 64, Lev:90 (netCDF seitig) (nc_open(); nc_get_dimension();)

    1) Baue mir ein 2.5D Gitter mit den Dimensionen 64 x 128 und Säulenhöhe 90 (t8code seitig)

    2a.) Definiere Datenstruktur, die netCDF Daten beinhaltet 

    2) Iteriere über 2D 64 x 128 Gitter -> für jedes Element Säulendaten aus netCDF auslesen (nc_get_var() für lon , lat Koordinaten )

    3) AdaptData-Struct definieren (= 2a.), das Zugriff auf die Daten hat und an adapt callback übergeben (t8code-setig)


}
*/

/* Print the local and global number of elements of a forest. */
void
t8_2_5D_print_forest_information (t8_forest_t forest)
{
  t8_locidx_t local_num_elements;
  t8_gloidx_t global_num_elements;

  /* Check that forest is a committed, that is valid and usable, forest. */
  T8_ASSERT (t8_forest_is_committed (forest));

  /* Get the local number of elements. */
  local_num_elements = t8_forest_get_local_num_elements (forest);
  /* Get the global number of elements. */
  global_num_elements = t8_forest_get_global_num_elements (forest);
  t8_global_productionf (" [2_5D] Local number of elements:\t\t%i\n", local_num_elements);
  t8_global_productionf (" [2_5D] Global number of elements:\t%li\n", global_num_elements);
}

void
t8_2_5D_elems_per_column(int *elems_per_column, int level1, int num_cmesh)
{
  for (int i=0; i<pow(2,2*level1)*num_cmesh; i++) {
    t8_global_productionf("elems_per_column[%i]: %i\n", i, elems_per_column[i]);
  }
}

float*
get_values_for_column_k(int k, int levlength, int latlength, int lonlength, float inputvalues[], float outputvalues[])
{
  int num_datapoints;
  num_datapoints = latlength * lonlength;
  outputvalues[0] = inputvalues[k];
  for (int i = 1; i < levlength; i++) {
    k += num_datapoints;
    outputvalues[i] = inputvalues[k];
  }
  return outputvalues;
}

//double*
float*
get_values_for_column_xy(int x, int y, int level, int corr_levlength, int levlength, int latlength, int lonlength, float inputvalues[], float outputvalues[])
{
  int num_datapoints;
  num_datapoints = latlength * lonlength;
  //Temperature values for column one at x = 0, y = 0
  //int k = x*y; //# of column
  int k = y*lonlength + x; //#of column  //= morton_index!!
  t8_global_productionf("column: %i\n", k);
  outputvalues[0] = inputvalues[k];
  for (int i = 1; i < levlength; i++) {
    k += num_datapoints;
    outputvalues[i] = inputvalues[k];
  }
  return outputvalues;
}

int
get_morton_index(int x, int y, int level)
{
  int x_vec[level];
  int y_vec[level];
  int morton[2*level];
  int morton_index = 0;
  for (int i = level-1; i >= 0; i--){
    if (pow(2,i) <= x){
      x_vec[i] = 1;
      x -= pow(2,i);
    }
    else {
      x_vec[i] = 0;
    }
    if (pow(2,i) <= y){
      y_vec[i] = 1;
      y -= pow(2,i);
    }
    else {
      y_vec[i] = 0;
    }
  }
  for (int i = 0; i < 2*level - 1; i+=2){
    morton[i] = x_vec[i/2];
    morton[i+1] = y_vec[i/2];
  }

  for (int i = 2*level-1; i >= 0 ; i--){
    morton_index += pow(2,i)*morton[i];
  }
  return morton_index;
}

void
get_coords_of_morton (int k, int level)
{ 
  int x = 0;
  int y = 0;
  int k_vec[2*level];
  int x_vec[level];
  int y_vec[level];
  for (int i = 2*level-1; i >= 0; i--){
    if (pow(2,i) <= k){
      k_vec[i] = 1;
      k -= pow(2,i);
    }
    else {
      k_vec[i] = 0;
    }
  }
  for (int i = 0; i < 2*level - 1; i+=2) {
    y_vec[i/2] = k_vec[i+1];
    x_vec[i/2] = k_vec[i];
  }
  for (int i = 0; i < level ; i++){
    x += pow(2,i)*x_vec[i];
    y += pow(2,i)*y_vec[i];
  }
  t8_global_productionf("(x,y) = (%i, %i) \n", x, y);
}

/*Testing if data sorting is done properly
*/
void
test_data (int pos, int levlength, int latlength, int lonlength, int level) 
{
  t8_global_productionf("levlength in test_data: %i", levlength);
  int column = pos / levlength;
  int level_pos = pos % levlength;
  get_coords_of_morton (column, level);
  t8_global_productionf("@ level %i \n", level_pos);
}

/* This is the user data that we will pass on to the
 * adaptation callback. */

struct MESSy_data
{
  float *temperature;
  float *height;
  float refine_if_more_than_90perc;
  float coarsen_if_less_than_10perc;
  int level_horizontal;
  int *elems_per_column;
};

float
t8_2_5D_calculate_gradient (double *gradient, t8_locidx_t length, float percentage, int comp_average)
{
  float value = 0;
  for (int i = 0; i < length; i++){
    value += gradient[i];
  }
  value /= length;
  t8_global_productionf("value: %f\n", value);
  if (comp_average == 1){
    value = value + percentage*value;
  }
  else if (comp_average == 0){
    value = value - (percentage*value);
  }
  else if (comp_average == 2){
    value *= percentage;
  }
  t8_global_productionf("value: %f\n", value);
  return value;
  
}

/* The adaptation callback function. This function will be called once for each element
 * and the return value decides whether this element should be refined or not.
 *   return > 0 -> This element should get refined.
 *   return = 0 -> This element should not get refined.
 * If the current element is the first element of a family (= all level l elements that arise from refining
 * the same level l-1 element) then this function is called with the whole family of elements
 * as input and the return value additionally decides whether the whole family should get coarsened.
 *   return > 0 -> The first element should get refined.
 *   return = 0 -> The first element should not get refined.
 *   return < 0 -> The whole family should get coarsened.
 *  
 * \param [in] forest       The current forest that is in construction.
 * \param [in] forest_from  The forest from which we adapt the current forest (in our case, the uniform forest)
 * \param [in] which_tree   The process local id of the current tree.
 * \param [in] lelement_id  The tree local index of the current element (or the first of the family).
 * \param [in] is_family    if 1, the first entries in \a elements form a family. If 0, they do not.
 * \param [in] num_elements The number of entries in \a elements elements that are defined.
 * \param [in] elements     The element or family of elements to consider for refinement/coarsening.
 */
int
t8_2_5D_adapt_callback (t8_forest_t forest, t8_forest_t forest_from, t8_locidx_t which_tree,
                        [[maybe_unused]] t8_eclass_t tree_class, [[maybe_unused]] t8_locidx_t lelement_id,
                        [[maybe_unused]] const t8_scheme *scheme, const int is_family,
                        [[maybe_unused]] const int num_elements, t8_element_t *elements[])
{
  /* The adaptation criterion for the MESSy application is to look at temperature data given for several positions and
  * differnt levels throughout the world. 
  * The temperature change inside the columns shall be considered. To do so, the temperature gradients are analysed.
  * gradients larger than average are refined and gradients smaller are coarsened. */

  t8_locidx_t elems = t8_forest_get_local_num_elements (forest_from);
  t8_locidx_t num_trees = t8_forest_get_num_local_trees (forest_from);
  t8_locidx_t elems_per_tree = elems/num_trees;
  t8_locidx_t elems_tree1 = t8_forest_get_tree_num_elements (forest_from, 0);
  
  /* In MESSy_2_5D_adapt_forest we pass a MESSy_data pointer as user data to the
   * t8_forest_new_adapt function. This pointer is stored as the used data of the new forest
   * and we can now access it with t8_forest_get_user_data (forest). */
  const struct MESSy_data *adapt_data = (const struct MESSy_data *) t8_forest_get_user_data (forest);
  const int level2_fixed = 21; 
  int level1 = adapt_data->level_horizontal;
  t8_global_productionf("adapt_data->coarsen: %f", adapt_data->coarsen_if_less_than_10perc);
  t8_global_productionf("adapt_data->refine: %f", adapt_data->refine_if_more_than_90perc);
  std::vector<int> levels = {adapt_data->level_horizontal, level2_fixed};
  t8_linearidx_t lin_id = scheme->element_get_linear_id (tree_class, elements[0],levels);
  t8_global_productionf("which_tree: %i", which_tree);
  int column = lin_id / pow(2,level2_fixed) + which_tree * pow(2,2*level1);
  t8_global_productionf ("column: %i \n", column);
  double gradient;
  double gradient1;
  double gradient2;

  t8_linearidx_t lin_id2;
  int column_succ1;
  t8_linearidx_t lin_id3;
  int column_succ2;

  t8_element_t **successor1 = T8_ALLOC(t8_element_t *, 1);
  t8_element_t **successor2 = T8_ALLOC(t8_element_t *, 1);
  int elems_help;
  if (which_tree == 0){
    elems_help = elems_tree1;
  }
  else {
    elems_help = elems;
  }
  if (lelement_id + (which_tree * elems_tree1) + 1 < elems_help){
    t8_locidx_t tree_succ1;
    successor1[0] = t8_forest_get_element(forest_from, lelement_id + (which_tree * elems_tree1) + 1, &tree_succ1);
    t8_global_productionf("tree_succ: %li", &tree_succ1);
    lin_id2 = scheme->element_get_linear_id (tree_class, successor1[0],levels);
    column_succ1 = lin_id2 / pow(2,level2_fixed) + which_tree * pow(2,2*level1);
    t8_global_productionf ("column_succ1: %i \n", column_succ1);
  }
  else {
    lin_id2 =  pow(2,level2_fixed) * (column-1) + adapt_data->elems_per_column[column];
  }

  if (is_family){
    if (lelement_id + (which_tree * elems_tree1) + 2 < elems_help){
      t8_locidx_t tree_succ2;
      successor2[0] = t8_forest_get_element(forest_from, lelement_id + (which_tree * elems_tree1) + 2, &tree_succ2);
      t8_global_productionf("tree_succ: %li", &tree_succ2);
      lin_id3 = scheme->element_get_linear_id (tree_class, successor2[0],levels);
      column_succ2 = lin_id3 / pow(2,level2_fixed) + which_tree * pow(2,2*level1);
      t8_global_productionf ("column_succ2: %i \n", column_succ2);
    }
    else{
      lin_id3 =  pow(2,level2_fixed) * (column-1) + adapt_data->elems_per_column[column];
    }
  }
  

  /* You can use T8_ASSERT for assertions that are active in debug mode (when configured with --enable-debug).
   * If the condition is not true, then the code will abort.
   * In this case, we want to make sure that we actually did set a user pointer to forest and thus
   * did not get the NULL pointer from t8_forest_get_user_data.
   */

  T8_FREE (successor1);
  T8_FREE (successor2);

  T8_ASSERT (adapt_data != NULL);
  float corr;
  float corr2;
  float corr3;
  unsigned id;
  unsigned id2;
  unsigned id3;
    corr = lin_id / pow(2,level2_fixed);
    id = 90 * corr + (which_tree * pow(2,2*level1) * 90);
    corr2 = lin_id2 / pow(2,level2_fixed);
    id2 = 90 * corr2 + (which_tree * pow(2,2*level1) * 90);

    t8_global_productionf ("lin_id: %u \n", lin_id);
    t8_global_productionf ("lin_id2: %u \n", lin_id2);
    
    t8_global_productionf ("id: %i \n", id);
    t8_global_productionf ("id2: %i \n", id2);
    
    t8_global_productionf ("corr: %f \n", corr);
    t8_global_productionf ("corr2: %f \n", corr2);

    if (is_family){
      corr3 = lin_id3 / pow(2,level2_fixed);
      id3 = 90 * corr3 + (which_tree * pow(2,2*level1) * 90);
      t8_global_productionf ("lin_id3: %u \n", lin_id3);
      t8_global_productionf ("id3: %i \n", id3);
      t8_global_productionf ("corr3: %f \n", corr3);

        t8_global_productionf ("adapt_data->height[id]: %f \n", adapt_data->height[id]);
        t8_global_productionf ("adapt_data->height[id2]: %f \n", adapt_data->height[id2]);
        t8_global_productionf ("adapt_data->height[id3]: %f \n", adapt_data->height[id3]);
        if (id3 < (column_succ2+1)*90){
          float diff_hei1 = abs(adapt_data->height[id] - adapt_data->height[id2]);
          if (diff_hei1 != 0){
            gradient1 = abs(adapt_data->temperature[id] - adapt_data->temperature[id2])/diff_hei1;
          }
          else{
            gradient1 = 0;
          }
          float diff_hei2 = abs(adapt_data->height[id2] - adapt_data->height[id3]);
          if (diff_hei2 != 0){
            gradient2 = abs(adapt_data->temperature[id2] - adapt_data->temperature[id3])/diff_hei2;
          }
          else{
            gradient2 = 0;
          }
          gradient = (gradient1 + gradient2)/2;
        }
        else {
          gradient = -0.00000001;
        }
    }
    else {
        t8_global_productionf ("adapt_data->height[id]: %f \n", adapt_data->height[id]);
        t8_global_productionf ("adapt_data->height[id2]: %f \n", adapt_data->height[id2]);
        if (id2 < (column_succ1+1)*90){
          float diff_hei = abs(adapt_data->height[id] - adapt_data->height[id2]);
          if (diff_hei != 0 ){
            gradient = abs(adapt_data->temperature[id] - adapt_data->temperature[id2])/diff_hei;
          }
          else {
            gradient = 0;
          }
        }
        else {
          gradient = -0.00000001;
        }
    }

  t8_global_productionf("gradient: %f", gradient);


  if (gradient > adapt_data->refine_if_more_than_90perc) {
    /* Refine this element. */
    adapt_data->elems_per_column[column] += 1;
    return 1;
  }
  else if (is_family && gradient1 < adapt_data->coarsen_if_less_than_10perc && gradient2 < adapt_data->coarsen_if_less_than_10perc) {
    /* Coarsen this family. Note that we check for is_family before, since returning < 0
     * if we do not have a family as input is illegal. */
    adapt_data->elems_per_column[column] -= 1;
    return -1;
  }
  else{
    t8_global_productionf("adapt_data->height[id]: %f", adapt_data->height[id]);
    t8_global_productionf("adapt_data->height[id2]: %f", adapt_data->height[id2]);
    t8_global_productionf("id: %u", id);
    t8_global_productionf("id2: %u", id2);
  }
  /* Do not change this element. */
  return 0;
}

/* Adapt a forest according to our t8_2_5D_adapt_callback function.
 * This will create a new forest and return it. */
t8_forest_t
t8_2_5D_adapt_forest (t8_forest_t forest, MESSy_data adapt_data)
{
  t8_forest_t forest_adapt;

  /* Check that forest is a committed, that is valid and usable, forest. */
  T8_ASSERT (t8_forest_is_committed (forest));

  t8_forest_set_user_data (forest, &adapt_data);

  /* Create a new forest that is adapted from \a forest with our adaptation callback.
   * We provide the adapt_data as user data that is stored as the used_data pointer of the
   * new forest (see also t8_forest_set_user_data).
   * The 0, 0 arguments are flags that control
   *   recursive  -    If non-zero adaptation is recursive, thus if an element is adapted the children
   *                   or parents are plugged into the callback again recursively until the forest does not
   *                   change any more. If you use this you should ensure that refinement will stop eventually.
   *                   One way is to check the element's level against a given maximum level.
   *   do_face_ghost - If non-zero additionally a layer of ghost elements is created for the forest.
   *                   We will discuss ghost in later steps of the tutorial.
   */
  forest_adapt = t8_forest_new_adapt (forest, t8_2_5D_adapt_callback, 0, 0, 2, &adapt_data);

  return forest_adapt;
}

/* The data that we want to store for each element.
 * In this example we want to store the element's level and volume. */
struct MESSy_data_per_element
{
  float *height;
  float *temperature;
  float *gradient;
  int *elems_per_column;
};

/* Write the forest as vtu and also write the element's volumes in the file.
 * 
 * t8code supports writing element based data to vtu as long as its stored
 * as doubles. Each of the data fields to write has to be provided in its own
 * array of length num_local_elements.
 * We support two types: T8_VTK_SCALAR - One double per element
 *                  and  T8_VTK_VECTOR - 3 doubles per element
 */
static void
t8_2_5D_output_data_to_vtu (t8_forest_t forest, struct MESSy_data_per_element *data, const char *prefix, double *gradient, int level1, int level2)
{
  t8_tree_t tree;
  t8_locidx_t itree;
  t8_locidx_t element_index, elems_in_tree;
  t8_locidx_t element_index_in_tree;
  t8_locidx_t elems_considered;
  t8_locidx_t num_global_trees;
  t8_locidx_t num_local_trees;
  t8_element_t *element;
  t8_locidx_t num_elements = t8_forest_get_global_num_elements (forest);
  t8_locidx_t correct;
  // t8_eclass_scheme_c *scheme;  //all two trees have the same scheme
  t8_linearidx_t lin_id;
  t8_locidx_t column;
  t8_locidx_t column_check;
  num_global_trees = t8_forest_get_num_global_trees (forest);
  element_index = 0;
  element_index_in_tree = 0;
  elems_considered = 0;
  column = 0;
  column_check = 0;
  /* We need to allocate a new array to store the volumes on their own.
   * The arrays have one entry per local element. */
  double *height = T8_ALLOC (double, num_elements);
  double *temperature = T8_ALLOC (double, num_elements);

  /* The number of user defined data fields to write. */
  int num_data = 3;
  /* For each user defined data field we need one t8_vtk_data_field_t variable */
  t8_vtk_data_field_t vtk_data[3];
  /* Set the type of this variable. Since we have for each array one value per element, we pick T8_VTK_SCALAR */
  vtk_data[0].type = T8_VTK_SCALAR;
  vtk_data[1].type = T8_VTK_SCALAR;
  vtk_data[2].type = T8_VTK_SCALAR;
  /* The name of the field as should be written to the file. */
  strcpy (vtk_data[0].description, "Height in m");
  strcpy (vtk_data[1].description, "Temperature in K");
  strcpy (vtk_data[2].description, "Gradient in K/m");
  vtk_data[0].data = height;
  vtk_data[1].data = temperature;
  vtk_data[2].data = gradient;
  for (itree = 0; itree < num_global_trees; itree++) {
    /* Get the tree that stores the elements */
    num_local_trees = t8_forest_get_num_local_trees (forest);
    if (itree < num_local_trees){
      tree = t8_forest_get_tree (forest, itree);
      /* Get the eclass scheme of the tree */
      const t8_scheme *scheme = t8_forest_get_scheme(forest);
      const t8_eclass_t tree_class = t8_forest_get_tree_class (forest, itree);
      elems_in_tree = (t8_locidx_t) t8_element_array_get_count (&tree->elements);
      t8_global_productionf ("elems_in_tree: %li \n", elems_in_tree);
      element_index_in_tree = elems_in_tree;
      int level2_fixed = 21; 
      int shift = 1;
      int shift_check = 0;
      int id_prev = -1;
      for (element_index = 0; element_index < element_index_in_tree; element_index++, column_check++) {
        t8_global_productionf ("element_index: %li \n", element_index);
        /* Get a pointer to the element */
        element = t8_forest_get_element (forest, tree->elements_offset + element_index, &itree);
        std::vector<int> levels = {level1, level2_fixed};
        lin_id = scheme->element_get_linear_id (tree_class, element, levels);

        int column = lin_id / pow(2, level2_fixed);
        t8_global_productionf ("column: %li \n", column);

        int pos = (itree * pow(2,2*level1)) + column;
        t8_global_productionf ("data->elems_per_column[column]: %li \n", data->elems_per_column[pos]);

          float corr = lin_id / pow(2,level2_fixed);
          int id = 90 * corr + (itree * pow(2,2*level1) * 90);

          t8_global_productionf ("corr: %f \n", corr);
          t8_global_productionf ("id: %li \n", id);          

          if (id % 90 == 0){
            height[element_index + elems_considered] = data->height[id];
            temperature[element_index + elems_considered] = data->temperature[id];
          }
          else if (data->height[id] != height[element_index + elems_considered - 1] && id_prev < id){
            height[element_index + elems_considered] = data->height[id];
            temperature[element_index + elems_considered] = data->temperature[id];
          }
          else {
            id = id_prev + 1;
            if (id < (pos+1)*90){
              height[element_index + elems_considered] = data->height[id];
              temperature[element_index + elems_considered] = data->temperature[id];
            }
            else{
              height[element_index + elems_considered] = 0;
              temperature[element_index + elems_considered] = 100;
            } 
          }
          t8_global_productionf("height[element_index + elems_considered]: %f", height[element_index + elems_considered]);

          id_prev = id;
      }
      t8_global_productionf ("element_index: %li \n", element_index);
      elems_considered += elems_in_tree;
      t8_global_productionf ("element_index: %li \n", element_index);
      t8_global_productionf ("num_elements: %li \n", num_elements);
      t8_global_productionf ("element_index_in_tree: %li \n", element_index_in_tree);
    }
  }
  
    column = 0;
    double *height_help = T8_ALLOC (double, pow(2,2*level1)*num_local_trees);
    double *temperature_help = T8_ALLOC (double, pow(2,2*level1)*num_local_trees);
    float diff_hei;
    for (int i = 0; i < pow(2,2*level1)*num_local_trees; i++){
      height_help[i] = data->height[(i+1)*90-1];
      temperature_help[i] = data->temperature[(i+1)*90-1];
      t8_global_productionf("height_help[%i]: %f \n", i, height_help[i]);
      t8_global_productionf("temperature_help[%i]: %f \n", i, temperature_help[i]);
    }
    int considered_elems_column = 0;
    for (int ielem = 0; ielem < num_elements; ++ielem) {
      int x = data->elems_per_column[column];
      if (((considered_elems_column + 1) % x) == 0){
        diff_hei = abs(height[ielem] - height_help[column]);
        if (diff_hei != 0){
          if (temperature[ielem] != 100){
            gradient[ielem] = abs(temperature[ielem] - temperature_help[column])/diff_hei;
          }
          else {
            gradient[ielem] = -0.00000001;
          }
        }
        else {
          gradient[ielem] = 0;
        }
        considered_elems_column = 0;
        column++;
      }
      else{
        if ((double)height[ielem+1] != (double)0){
          diff_hei = abs(height[ielem] - height[ielem+1]);
        }
        else{
          diff_hei = 0;
        }
        if (diff_hei != 0){
          if (temperature[ielem] != 100){
            gradient[ielem] = abs(temperature[ielem] - temperature[ielem+1])/diff_hei;
          }
          else {
            gradient[ielem] = -0.00000001;
          }
        }
        else {
          if (considered_elems_column > 90){
            gradient[ielem] = -0.00000001;
          }
          else{
            gradient[ielem] = 0;
          }
        }
        considered_elems_column++;
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
  T8_FREE (height);
  T8_FREE (temperature);
  T8_FREE (height_help);
  T8_FREE (temperature_help);
}

int
main (int argc, char **argv)
{
  int mpiret;
  sc_MPI_Comm comm;
  t8_cmesh_t cmesh;
  t8_forest_t forest;
  /* The prefix for our output files. */
  const char *prefix_uniform = "t8_2_5D_uniform_forest";
  const char *prefix_forest_with_data = "t8_2_5D_forest_with_height_temperature_gradient";
  const char *prefix_adapt = "t8_2_5D_adapted_forest";
  const char *prefix_forest_with_data_adapt = "t8_2_5D_forest_with_height_temperature_gradient_adapt";
  const char *prefix_adapt1 = "t8_2_5D_adapted_forest_iteration1";
  const char *prefix_forest_with_data_adapt1 = "t8_2_5D_forest_with_height_temperature_gradient_adapt_iteration1";
  const char *prefix_adapt2 = "t8_2_5D_adapted_forest_iteration2";
  const char *prefix_forest_with_data_adapt2 = "t8_2_5D_forest_with_height_temperature_gradient_adapt_iteration2";
  const char *prefix_adapt3 = "t8_2_5D_adapted_forest_iteration3";
  const char *prefix_forest_with_data_adapt3 = "t8_2_5D_forest_with_height_temperature_gradient_adapt_iteration3";
  const char *prefix_adapt4 = "t8_2_5D_adapted_forest_iteration4";
  const char *prefix_forest_with_data_adapt4 = "t8_2_5D_forest_with_height_temperature_gradient_adapt_iteration4";
  const char *prefix_adapt5 = "t8_2_5D_adapted_forest_iteration5";
  const char *prefix_forest_with_data_adapt5 = "t8_2_5D_forest_with_height_temperature_gradient_adapt_iteration5";
  const char *prefix_adapt6 = "t8_2_5D_adapted_forest_iteration6";
  const char *prefix_forest_with_data_adapt6 = "t8_2_5D_forest_with_height_temperature_gradient_adapt_iteration6";

  
  /* The uniform refinement level of the forest. */
  int level1;
  int level2;

  /* Initialize MPI. This has to happen before we initialize sc or t8code. */
  mpiret = sc_MPI_Init (&argc, &argv);
  /* Error check the MPI return value. */
  SC_CHECK_MPI (mpiret);

  /* Initialize the sc library, has to happen before we initialize t8code. */
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  /* Initialize t8code with log level SC_LP_PRODUCTION. See sc.h for more info on the log levels. */
  t8_init (SC_LP_PRODUCTION);

  /* Print a message on the root process. */
  t8_global_productionf (" [2_5D] \n");
  t8_global_productionf (" [2_5D] This the 2_5D MESSy application example in t8code.\n");
  t8_global_productionf (" [2_5D] In this example ....\n");
  t8_global_productionf (" [2_5D] \n");

  /* We will use MPI_COMM_WORLD as a communicator. */
  comm = sc_MPI_COMM_WORLD;

  int status, ncid, lonid, latid, levid;
  size_t lonlength, latlength, levlength;

  nc_open("RD1-base-01____20190801-03_ECHAM5_tm1.nc", NC_NOWRITE, &ncid);

  nc_inq_dimid(ncid, "lon", &lonid);

  nc_inq_dimlen(ncid, lonid, &lonlength);
  
  nc_inq_dimid(ncid, "lat", &latid);
  
  nc_inq_dimlen(ncid, latid, &latlength);

  nc_inq_dimid(ncid, "lev", &levid);
  
  nc_inq_dimlen(ncid, levid, &levlength);

  lonlength = 16;
  latlength = 8;
  levlength = 90;
  t8_global_productionf("lonlength: %i & latlength: %i & levlength: %i \n", lonlength, latlength, levlength);

  int x,y,z;
  int x_lev, y_lev, z_lev;
  int x_check, y_check, z_check;

  for (int i = 0; i < 8; i++) { 
  int p = pow(2,i);
    if (lonlength <= pow(2,i)){
      x = pow(2,i);
      x_lev = i;
      x_check = x - lonlength;
      if (x_check >= 0){
        break;
      }
    }
  }
  for (int i = 0; i < 8; i++) { 
    if (latlength <= pow(2,i)){
      y = pow(2,i);
      y_lev = i;
      y_check = y - latlength;
      if (y_check >= 0){
        break;
      }
    }
  }
  for (int i = 0; i < 8; i++) { 
    if (levlength <= pow(2,i)){
      z = pow(2,i);
      z_lev = i;
      z_check = z - levlength;
      if (z_check >= 0){
        break;
      }
    }
  }

  t8_global_productionf("(x,y,z) = (%i,%i,%i) & (x_check, y_check, z_check) = (%i,%i,%i) & (x_lev, y_lev, z_lev) = (%i,%i,%i) \n", x, y, z, x_check, y_check, z_check, x_lev, y_lev, z_lev);


  int tm1_id;
  static size_t start_tm1[] = {0, 0, 0, 0};
  static size_t count_tm1[] = {1, levlength, latlength, lonlength};
  float tm_1[levlength*latlength*lonlength];

  nc_inq_varid (ncid, "tm1", &tm1_id);
  nc_get_vara_float(ncid, tm1_id, start_tm1, count_tm1, tm_1);

  int geopot_id;
  static size_t start_geopot[] = {0, 0, 0, 0};
  static size_t count_geopot[] = {1, levlength, latlength, lonlength};
  float geopot[levlength*latlength*lonlength];

  nc_inq_varid (ncid, "geopot", &geopot_id);
  nc_get_vara_float(ncid, geopot_id, start_geopot, count_geopot, geopot);

  float* elems_number = new float[levlength*latlength*lonlength];
  for (int i=0; i<levlength*latlength*lonlength; i++){
    elems_number[i] = i;
  }

  float tm_1_col_k[levlength];
  float geopot_col_0[levlength];
  float geopot_col_1[levlength];
  get_values_for_column_k(0, levlength, latlength, lonlength, tm_1, tm_1_col_k);
  get_values_for_column_k(0, levlength, latlength, lonlength, geopot, geopot_col_0);
  get_values_for_column_k(1, levlength, latlength, lonlength, geopot, geopot_col_1);


  /*As we will use a hypercube as cmesh and only the z-direction is dependent, the ratio of lonlength and latlength decides about the number of initial cmeshes
  /*several cmeshes can be used as for now adaptation only takes place in z-direction and the corresponding levels needed for adaptation will always be in the same cmesh 
  *as they are dependent on the x- and -y-direction they belong to.*/

  int num_cmesh;
  int quadlength;
  if (x >= y){
    num_cmesh = x/y;
    level1 = y_lev;
    quadlength = y;
  }
  else {
    num_cmesh = y/x;
    level1 = x_lev;
    quadlength = x;
  }

  level2 = z_lev;

  t8_global_productionf("num_cmesh: %i & level1: %i & level2: %i\n", num_cmesh, level1, level2);

  /*Sorting the Data: the underlying uniform forest will contain the quad scheme and therefore the iterative Z-order in xy-direction
  */

  t8_global_productionf("quadlength: %i", quadlength);

  float radius_earth = 6371000;

  bool exit_loop;
  float* tm_1_sort_90 = new float[levlength*latlength*lonlength];
  float* geopot_sort_90 = new float[levlength*latlength*lonlength];
  float* height_sort_90 = new float[levlength*latlength*lonlength];
  float* temp_grad_sort_90 = new float[levlength*latlength*lonlength];
  int col_per_cmesh = (latlength * lonlength)/num_cmesh;
  int elems_per_cmesh = (levlength * latlength * lonlength)/num_cmesh;
  for (int cmesh = 0; cmesh < num_cmesh; cmesh++){
    for (int check = 0; check < col_per_cmesh; check++){
      exit_loop = false;
      for (int x_val = 0; x_val < quadlength && !exit_loop; x_val++){
        for (int y_val = 0; y_val < quadlength; y_val++){
          if (get_morton_index(x_val, y_val, level1) == check) {
            int pos = elems_per_cmesh * cmesh + check * levlength;
            if (latlength > lonlength){
              get_values_for_column_xy(x_val, y_val + pow(2, level1) * cmesh, level1, levlength, levlength, latlength, lonlength, tm_1, tm_1_sort_90 + pos);
              get_values_for_column_xy(x_val, y_val + pow(2, level1) * cmesh, level1, levlength, levlength, latlength, lonlength, geopot, geopot_sort_90 + pos);
            }
            else if (latlength < lonlength){
              get_values_for_column_xy(x_val + pow(2, level1) * cmesh, y_val, level1, levlength, levlength, latlength, lonlength, tm_1, tm_1_sort_90 + pos);
              get_values_for_column_xy(x_val + pow(2, level1) * cmesh, y_val, level1, levlength, levlength, latlength, lonlength, geopot, geopot_sort_90 + pos);
            }
            else if (latlength == lonlength){
              get_values_for_column_xy(x_val, y_val, level1, levlength, levlength, latlength, lonlength, tm_1, tm_1_sort_90 + pos);
              get_values_for_column_xy(x_val, y_val, level1, levlength, levlength, latlength, lonlength, geopot, geopot_sort_90 + pos);
            }
            for (int j = pos; j < pos + levlength; j++){
              // height_sort_90[j] = (radius_earth * geopot_sort_90[j]/9.81)/(radius_earth - geopot_sort_90[j]/9.81);
              height_sort_90[j] = geopot_sort_90[j]/9.81;
            }
            exit_loop = true;
            break;
          }
        }
      }
    }
  }

  float* elems_sort_90 = new float[levlength*latlength*lonlength];
  for (int i=0; i<levlength*latlength*lonlength; i++){
    elems_sort_90[i] = i;
  }

  t8_global_productionf("level for quad scheme is %i \n", level1);
  get_morton_index(2, 3, level1);

  /*
  * Setup.
  * Build cmesh and uniform forest.
  */

  cmesh = t8_cmesh_new_row_of_cubes (num_cmesh, 1, 0, comm);
  t8_global_productionf (" [2_5D] Created coarse mesh.\n");
  // const t8_scheme *scheme_base = t8_scheme_new_default ();
  level2 = 4;
  forest = t8_forest_new_uniform_2_5D (cmesh, t8_scheme_new_2_5dimension (t8_scheme_new_default ()), t8_scheme_new_default(), level1, level2, 0, comm);

  /* Print information of the forest. */
  t8_global_productionf (" [2_5D] Created uniform forest.\n");
  t8_global_productionf (" [2_5D] Refinement level1:\t%i\n", level1);
  t8_global_productionf (" [2_5D] Refinement level2:\t%i\n", level2);
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_uniform);
  t8_global_productionf (" [2_5D] Wrote uniform forest to vtu files: %s*\n", prefix_uniform);

  int *elems_per_column =new int[num_cmesh*latlength*lonlength];
  int num_elems = pow(2,level2);
  for (int i=0; i < num_cmesh*latlength*lonlength; i++){
    elems_per_column[i] = num_elems;
  }

  struct MESSy_data_per_element data = {
    height_sort_90,
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column
  };

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  t8_locidx_t num_elements = t8_forest_get_global_num_elements (forest);
  double *gradient = T8_ALLOC (double, num_elements);

  /*
   * Output the volume data to vtu.
   */
  t8_2_5D_output_data_to_vtu (forest, &data, prefix_forest_with_data, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data);

  /*---------------------------ADAPT 1--------------------------------*/

  //num_elements = t8_forest_get_global_num_elements (forest);  
  
  float coarsen;
  float refine;
  float percentage1 = 0; //0.05;
  float percentage2 = 0; //0.05;
  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("coarsen: %f", coarsen);
  t8_global_productionf("refine: %f", refine);

  struct MESSy_data adapt_data = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted forest.\n");
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt);
  t8_global_productionf (" [2_5D] Wrote adapted forest to vtu files: %s*\n", prefix_adapt);


  /* Get the global number of elements of adapted forest. */
  num_elements = t8_forest_get_global_num_elements (forest);
  gradient = T8_REALLOC (gradient, double, num_elements);

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  struct MESSy_data_per_element data_adapt_element = {
    height_sort_90,
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column
  };

  
  t8_2_5D_output_data_to_vtu (forest, &data_adapt_element, prefix_forest_with_data_adapt, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data_adapt);

  /*---------------------------ADAPT 2--------------------------------*/

  num_elements = t8_forest_get_global_num_elements (forest); 

  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("coarsen2: %f", refine);
  t8_global_productionf("refine2: %f", refine);


  struct MESSy_data adapt_data1 = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data1);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted forest.\n");
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt1);
  t8_global_productionf (" [2_5D] Wrote adapted forest to vtu files: %s*\n", prefix_adapt1);


    /* Get the global number of elements of adapted forest. */
  num_elements = t8_forest_get_global_num_elements (forest);
  gradient = T8_REALLOC (gradient, double, num_elements);

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  struct MESSy_data_per_element data_adapt_element1 = {
    height_sort_90, 
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column
  };

  
  t8_2_5D_output_data_to_vtu (forest, &data_adapt_element1, prefix_forest_with_data_adapt1, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data_adapt1);

  /*---------------------------ADAPT 3--------------------------------*/

  // num_elements = t8_forest_get_global_num_elements (forest); 
 
  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("coarsen3: %f", refine);
  t8_global_productionf("refine3: %f", refine);


  struct MESSy_data adapt_data2 = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data2);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted forest.\n");
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt2);
  t8_global_productionf (" [2_5D] Wrote adapted forest to vtu files: %s*\n", prefix_adapt2);


    /* Get the global number of elements of adapted forest. */
  num_elements = t8_forest_get_global_num_elements (forest);
  gradient = T8_REALLOC (gradient, double, num_elements);

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  struct MESSy_data_per_element data_adapt_element2 = {
    height_sort_90, 
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column 
  };

  
  t8_2_5D_output_data_to_vtu (forest, &data_adapt_element2, prefix_forest_with_data_adapt2, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data_adapt2);

  /*---------------------------ADAPT 4--------------------------------*/

  num_elements = t8_forest_get_global_num_elements (forest); 

  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("refine: %f", refine);


  struct MESSy_data adapt_data3 = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data3);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted forest.\n");
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt3);
  t8_global_productionf (" [2_5D] Wrote adapted forest to vtu files: %s*\n", prefix_adapt3);


    /* Get the global number of elements of adapted forest. */
  num_elements = t8_forest_get_global_num_elements (forest);
  gradient = T8_REALLOC (gradient, double, num_elements);

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  struct MESSy_data_per_element data_adapt_element3 = {
    height_sort_90, 
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column
  };

  
  t8_2_5D_output_data_to_vtu (forest, &data_adapt_element3, prefix_forest_with_data_adapt3, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data_adapt3);

  /*---------------------------ADAPT 5--------------------------------*/

  num_elements = t8_forest_get_global_num_elements (forest); 

  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("refine: %f", refine);


  struct MESSy_data adapt_data4 = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data4);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted forest.\n");
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt4);
  t8_global_productionf (" [2_5D] Wrote adapted forest to vtu files: %s*\n", prefix_adapt4);


    /* Get the global number of elements of adapted forest. */
  num_elements = t8_forest_get_global_num_elements (forest);
  gradient = T8_REALLOC (gradient, double, num_elements);

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  struct MESSy_data_per_element data_adapt_element4 = {
    height_sort_90, 
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column
  };

  
  t8_2_5D_output_data_to_vtu (forest, &data_adapt_element4, prefix_forest_with_data_adapt4, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data_adapt4);

  /*---------------------------ADAPT 6--------------------------------*/

  num_elements = t8_forest_get_global_num_elements (forest); 

  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("refine: %f", refine);


  struct MESSy_data adapt_data5 = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data5);

  num_elements = t8_forest_get_global_num_elements (forest); 

  coarsen = t8_2_5D_calculate_gradient (gradient, num_elements, percentage1, 0);
  refine = t8_2_5D_calculate_gradient (gradient, num_elements, percentage2, 1);
  t8_global_productionf("refine: %f", refine);


  struct MESSy_data adapt_data6 = {
    tm_1_sort_90,
    height_sort_90,
    refine,
    coarsen,
    level1,
    elems_per_column
  };

   /*
   *  Adapt the forest according to the temperature gradients in vertical direction.
   */
  
  /* Adapt the forest. We can reuse the forest variable, since the new adapted
   * forest will take ownership of the old forest and destroy it.
   * Note that the adapted forest is a new forest, though. */
  forest = t8_2_5D_adapt_forest (forest, adapt_data6);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted forest.\n");
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt5);
  t8_global_productionf (" [2_5D] Wrote adapted forest to vtu files: %s*\n", prefix_adapt5);


    /* Get the global number of elements of adapted forest. */
  num_elements = t8_forest_get_global_num_elements (forest);
  gradient = T8_REALLOC (gradient, double, num_elements);

  t8_2_5D_elems_per_column(elems_per_column, level1, num_cmesh);

  struct MESSy_data_per_element data_adapt_element5 = {
    height_sort_90, 
    tm_1_sort_90,
    temp_grad_sort_90,
    elems_per_column
  };

  
  t8_2_5D_output_data_to_vtu (forest, &data_adapt_element4, prefix_forest_with_data_adapt5, gradient, level1, level2);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data_adapt5);


  nc_close (ncid);






  /*
   * clean-up
   */

  /* Destroy the forest. */
  t8_forest_unref (&forest);
  t8_global_productionf (" [2_5D] Destroyed forest.\n");

  T8_FREE (gradient);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

T8_EXTERN_C_END ();