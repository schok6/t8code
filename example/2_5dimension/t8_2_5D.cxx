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
//#include <t8_cmesh_vtk_writer.h>        /* cmesh-writer interface. */
#include <t8_forest/t8_forest_general.h>
#include <t8_forest/t8_forest_io.h>
#include <t8_forest/t8_forest_geometrical.h> 
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx>
#include <t8_vec.h>                      /* Basic operations on 3D vectors. */
#include <netcdf.h>
#include <algorithm>
#include <cmath>
// #include <span> //c++20
// #include <example/2_5dimension/netCDF_2_5D.hxx>
// #include <example/2_5dimension/netCDF.hxx>

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

float*
get_values_for_column_k(int k, int levlength, int latlength, int lonlength, float inputvalues[], float outputvalues[])
{
  int num_datapoints;
  num_datapoints = latlength * lonlength;
  // t8_global_productionf(" data points per level: %i \n", num_datapoints);
  //Temperature values for column one at x = 0, y = 0
  //int k = x*y; //# of column
  //k = 0;
  outputvalues[0] = inputvalues[k];
  for (int i = 1; i < levlength; i++) {
  //for (k; k < levlength*latlength*lonlength; k += num_datapoints) {
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
  // t8_global_productionf(" data points per level: %i \n", num_datapoints);
  //Temperature values for column one at x = 0, y = 0
  //int k = x*y; //# of column
  int k = y*lonlength + x; //#of column  //= morton_index!!
  //int k = get_morton_index(x, y, level);
  // t8_global_productionf("k = %i \n", k);
  outputvalues[0] = inputvalues[k];
  for (int i = 1; i < levlength; i++) {
  //for (k; k < levlength*latlength*lonlength; k += num_datapoints) {
    k += num_datapoints;
    outputvalues[i] = inputvalues[k];
  }
  // for (int i = levlength; i < corr_levlength; i++){
  //   outputvalues[i] = 0;
  // }
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
  for (int i = 0; i < level; i++){
    // t8_global_productionf("x_vec[%i] = %i & y_vec[%i] = %i \n", i, x_vec[i], i, y_vec[i]);
  }
  //zu einem Schritt machen? interleving und morton index bestimmen
  for (int i = 0; i < 2*level - 1; i+=2){
    //t8_global_productionf("morton[%i] = %i & morton[%i] = %i \n", i, x_vec[i/2], i+1, y_vec[i/2]);
    morton[i] = x_vec[i/2];
    morton[i+1] = y_vec[i/2];
  }
  // for (int i = 0; i < 2*level; i++){
  //   t8_global_productionf("morton[%i] = %i \n", i, morton[i]);
  // }
  for (int i = 2*level-1; i >= 0 ; i--){
    morton_index += pow(2,i)*morton[i];
    // t8_global_productionf("@ %i morton_index = %i \n", i, morton_index);
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
  for (int i = 0; i < 2*level - 1; i++) {
    // t8_global_productionf("k_vec[%i] = %i \n", i, k_vec[i]);
  }
  for (int i = 0; i < 2*level - 1; i+=2) {
    y_vec[i/2] = k_vec[i+1];
    x_vec[i/2] = k_vec[i];
    // t8_global_productionf("x_vec[%i] = %i & y_vec[%i] = %i \n", i/2, x_vec[i/2], i/2, y_vec[i/2]);
  }
  for (int i = 0; i < level ; i++){
    x += pow(2,i)*x_vec[i];
    y += pow(2,i)*y_vec[i];
  }
  t8_global_productionf("(x,y) = (%i, %i) \n", x, y);
  //return x, y;
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
  // int point_per_level = latlength * lonlength;
  // int level = pos / point_per_level;
  // t8_global_productionf("level: %i \n", level);
  // int point_in_level = pos % point_per_level;
  // t8_global_productionf("point_in_level: %i \n", point_in_level);
  // int y = point_in_level / latlength;
  // int x = point_in_level % latlength;
  t8_global_productionf("@ level %i \n", level_pos);
}

/* This is the user data that we will pass on to the
 * adaptation callback. */

struct MESSy_data
{
  size_t levlength; /*#level*/              
  size_t latlength; /*#y-values*/
  size_t lonlength; /*#x-values*/
  int corr_levlength;
  float* temperature; /* temperature values according to 2^level2 */
  float* geoplot; /* geopot values according to 2^level2 -> geopot/9.81 = height in m*/
};

// calculate_temperature_gradients (t8_forest forest, t8_locidx_t ltreeid, const t8_element_t *element, 
//                                   MESSy_data temperature, float gradients[]) 
// {
//   /*Calculate gradients for temperature*/
//   //how do I know what column to consider? @Lukas ->get linear id
// }

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
 * \param [in] elements     The element or family of elements to consider for refinement/coarsening.
 */
int
t8_2_5D_adapt_callback (t8_forest_t forest, t8_forest_t forest_from, t8_locidx_t which_tree, t8_locidx_t lelement_id,
                         t8_eclass_scheme_c *ts, const int is_family, const int num_elements, t8_element_t *elements[])
{
  /* The adaptation criterion for the MESSy application is to look at temperature data given for several positions and
  * different levels throughout the world. 
  * The temperature change inside the columns shall be considered. To do so, the temperature gradients are analysed.
  * The less 5% of temperature gradients are coarsened and the steepest 5% are getting refined. */
  
  //double average; /* Will hold the average of the column where the elements belong to. */
  
  /* In MESSy_2_5D_adapt_forest we pass a MESSy_data pointer as user data to the
   * t8_forest_new_adapt function. This pointer is stored as the used data of the new forest
   * and we can now access it with t8_forest_get_user_data (forest). */
  const struct MESSy_data *adapt_data = (const struct MESSy_data *) t8_forest_get_user_data (forest);
  double temp_gradient[adapt_data->levlength];
  int perc = adapt_data->levlength*5/100; /*5% quantil needed to determine the steepest and less steepest gradients*/
  // double dist; /* Will store the distance of the element's midpoint and the sphere midpoint. */

  /* You can use T8_ASSERT for assertions that are active in debug mode (when configured with --enable-debug).
   * If the condition is not true, then the code will abort.
   * In this case, we want to make sure that we actually did set a user pointer to forest and thus
   * did not get the NULL pointer from t8_forest_get_user_data.
   */
  T8_ASSERT (adapt_data != NULL);

  /* Compute the temperature gradients for the column. */
  // int column = 5; //for testing -> how do I know what column to consider? @Lukas
  // calculate_temperature_gradients (forest_from, which_tree, elements[0], adapt_data->temperature, temp_gradient);


 

  // /* Compute the distance to our sphere midpoint. */
  // dist = t8_vec_dist (centroid, adapt_data->midpoint);
  // if (dist < adapt_data->refine_if_inside_radius) {
  //   /* Refine this element. */
  //   return 1;
  // }
  // else if (is_family && dist > adapt_data->coarsen_if_outside_radius) {
  //   /* Coarsen this family. Note that we check for is_family before, since returning < 0
  //    * if we do not have a family as input is illegal. */
  //   return -1;
  // }
  /* Do not change this element. */
  return 0;
}

/* Adapt a forest according to our t8_2_5D_adapt_callback function.
 * This will create a new forest and return it. */
t8_forest_t
t8_2_5D_adapt_forest (t8_forest_t forest, MESSy_data adapt_data)
{
  t8_forest_t forest_adapt;
  // struct MESSy_data adapt_data = {
  //   { 0.0, 0.0, 1 }, /* Midpoints of the sphere. */
  //   // 0.1,             /* Refine if inside this radius. */
  //   0.5,             /* Refine if inside this radius. */
  //   0.6             /* Coarsen if outside this radius. */
  // };

  /* Check that forest is a committed, that is valid and usable, forest. */
  T8_ASSERT (t8_forest_is_committed (forest));

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
t8_2_5D_output_data_to_vtu (t8_forest_t forest, struct MESSy_data_per_element *data, const char *prefix)
{
  t8_locidx_t num_elements = t8_forest_get_local_num_elements (forest);
  t8_locidx_t ielem;
  /* We need to allocate a new array to store the volumes on their own.
   * The arrays have one entry per local element. */
  double *height = T8_ALLOC (double, num_elements);
  double *temperature = T8_ALLOC (double, num_elements);
  double *gradient = T8_ALLOC (double, num_elements);
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
  /* Copy the element's height from our data array to the output array. */
  for (ielem = 0; ielem < num_elements; ++ielem) {
    height[ielem] = data->height[ielem];
    temperature[ielem] = data->temperature[ielem];
    gradient[ielem] = data->gradient[ielem];
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
  T8_FREE (gradient);
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

  lonlength = 4;
  latlength = 4;
  levlength = 90;
  t8_global_productionf("lonlength: %i & latlength: %i & levlength: %i \n", lonlength, latlength, levlength);

  int x,y,z;
  int x_lev, y_lev, z_lev;
  int x_check, y_check, z_check;

  for (int i = 0; i < 8; i++) { //x,y,z <= 256
  int p = pow(2,i);
    // t8_global_productionf("i = %i & pow(2,i) = %i \n", i, p);
    if (lonlength <= pow(2,i)){
      x = pow(2,i);
      x_lev = i;
      // t8_global_productionf("x = %i \n", x);
      x_check = x - lonlength;
      // t8_global_productionf("x_check = %i \n", x_check);
      if (x_check >= 0){
        break;
      }
    }
  }
  for (int i = 0; i < 8; i++) { //x,y,z <= 256
    if (latlength <= pow(2,i)){
      y = pow(2,i);
      y_lev = i;
      y_check = y - latlength;
      if (y_check >= 0){
        break;
      }
    }
  }
  for (int i = 0; i < 8; i++) { //x,y,z <= 256
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

  for (int i = 0; i < 90; i++){
    t8_global_productionf("tm1 at pos %i = %f \n", i, tm_1[i]);
  }

  t8_global_productionf("Test");

  int geopot_id;
  static size_t start_geopot[] = {0, 0, 0, 0};
  static size_t count_geopot[] = {1, levlength, latlength, lonlength};
  float geopot[levlength*latlength*lonlength];

  nc_inq_varid (ncid, "geopot", &geopot_id);
  nc_get_vara_float(ncid, geopot_id, start_geopot, count_geopot, geopot);

  t8_global_productionf("Test");

  double alt;
  for (int i = 0; i < 90; i++){
    alt = geopot[i]/9.81;
    t8_global_productionf(" geopot at position %i : %f \n", i, geopot[i]);
    t8_global_productionf(" Altidude above sea level at position %i : %f \n", i, alt);
  }

  float tm_1_col_k[levlength];
  float geopot_col_0[levlength];
  float geopot_col_1[levlength];
  get_values_for_column_k(0, levlength, latlength, lonlength, tm_1, tm_1_col_k);
  get_values_for_column_k(0, levlength, latlength, lonlength, geopot, geopot_col_0);
  get_values_for_column_k(1, levlength, latlength, lonlength, geopot, geopot_col_1);

  for (int i = 0; i < levlength; i++){
    alt = geopot_col_0[i]/9.81;
    float alt_compare = geopot_col_1[i]/9.81;
    float diff = alt - alt_compare;
    t8_global_productionf(" Dry Air Temperature in the first column in Kelvin: %f @ height %f in m (comparasion: %f => difference = %f) due to the geopotential of %f @ level %i\n", tm_1_col_k[i], alt, alt_compare, diff, geopot_col_0[i], i);
  }


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
    num_cmesh = x/y;
    level1 = x_lev;
    quadlength = x;
  }

  level2 = z_lev;

  t8_global_productionf("num_cmesh: %i & level1: %i & level2: %i\n", num_cmesh, level1, level2);

  /*Sorting the Data: the underlying uniform forest will contain the quad scheme and therefore the iterative Z-order in xy-direction
  */

  t8_global_productionf("quadlength: %i", quadlength);

  // quadlength = 5;
  bool exit_loop;
  int corr_levlength = pow(2, level2);
  t8_global_productionf("corr_levlength: %i", corr_levlength);
  // float* tm_1_col = new float[levlength];
  // float* geopot_col = new float[levlength];
  float* temp_grad_sort = new float[corr_levlength*latlength*lonlength];
  float* tm_1_sort = new float[corr_levlength*latlength*lonlength];
  float* geopot_sort = new float[corr_levlength*latlength*lonlength];
  float* height_sort = new float[corr_levlength*latlength*lonlength];
  int col_per_cmesh = (latlength * lonlength)/num_cmesh;
  int elems_per_cmesh = (corr_levlength * latlength * lonlength)/num_cmesh;
  for (int cmesh = 0; cmesh < num_cmesh; cmesh++){
    // int col_per_cmesh = 9;
    for (int check = 0; check < col_per_cmesh; check++){
      exit_loop = false;
      // t8_global_productionf("check: %i \n", check);
      for (int x_val = 0; x_val < quadlength && !exit_loop; x_val++){
        // t8_global_productionf("________________________");
        // t8_global_productionf("x_val: %i \n", x_val);
        for (int y_val = 0; y_val < quadlength; y_val++){
          // t8_global_productionf("y_val: %i \n", y_val);
          if (get_morton_index(x_val, y_val, level1) == check) {
            int pos = elems_per_cmesh * cmesh + check * corr_levlength;
            // t8_global_productionf("elems_per_cmesh * cmesh + check * levlength: %i", pos);
            // t8_global_productionf("levlength: %i", levlength);

            // get_values_for_column_xy(x_val, y_val, level1, corr_levlength, levlength, latlength, lonlength, tm_1, tm_1_col);
            // get_values_for_column_xy(x_val, y_val, level1, corr_levlength, levlength, latlength, lonlength, geopot, geopot_col);

            // for (int i = 0; i < levlength - 1; i++){
            //   float diff_temp = abs(tm_1_col[i] - tm_1_col[i+1]);
            //   float diff_hei = abs(geopot_col[i] - geopot_col[i+1])/; 
            // }

            get_values_for_column_xy(x_val, y_val, level1, corr_levlength, levlength, latlength, lonlength, tm_1, tm_1_sort + pos);
            get_values_for_column_xy(x_val, y_val, level1, corr_levlength, levlength, latlength, lonlength, geopot, geopot_sort + pos);
            for (int j = pos; j < pos + levlength -1; j++){
              height_sort[j] = geopot_sort[j]/9.81;
              float diff_hei = abs(geopot_sort[j] - geopot_sort[j+1])/9.81;
              if (diff_hei != 0){
                temp_grad_sort[j] = abs(tm_1_sort[j] - tm_1_sort[j+1])/diff_hei;
              }
              else {
                temp_grad_sort[j] = 0;
              }
            }
            height_sort[pos + levlength - 1] = geopot_sort[pos + levlength - 1]/9.81;
            temp_grad_sort[pos + levlength - 1] = -0.00000001;
            for (int j = pos + levlength; j < pos + corr_levlength; j++){
              height_sort[j] = 0;
              temp_grad_sort[j] = -0.00000001;
              tm_1_sort[j] = 200;//tm_1_sort[pos + levlength - 1]
              geopot_sort[j] = 0; //braucht man gar nicht
            }
            // t8_global_productionf("get_morton_index(x_val, y_val, level1) == check: %i = %i \n", get_morton_index(x_val, y_val, level1), check);
            exit_loop = true;
            break;
          }
        }
      }
    }
  }

  // float* temp_grad_sort = new float[corr_levlength*latlength*lonlength];
  // // for (int i = 0; i < latlength*lonlength; i++){
  // // for (int i = 0; i < 1; i++){
  //   // for (int j = i*corr_levlength; j < i*corr_levlength + levlength - 1; j++){
  //   for (int j = 0; j < 7; j++){
  //     float diff_hei = abs(geopot_sort[j] - geopot_sort[j+1])/9.81;
  //     if (diff_hei != 0){
  //       temp_grad_sort[j] = abs(tm_1_sort[j] - tm_1_sort[j+1])/diff_hei;
  //     }
  //     else {
  //       temp_grad_sort[j] = 0;
  //     }
  //   }
  //   // for (int j = i*corr_levlength + levlength - 1; j = (i+1)*corr_levlength; j++){
  //   //   temp_grad_sort[j] = -1;
  //   // }
  // // }

  //TESTING

  // float tm_1_test[levlength];
  // get_values_for_column_xy(0, 0, level1, levlength, latlength, lonlength, tm_1, tm_1_test);
  // t8_global_productionf("tm_1_test[2]: %f\n", tm_1_test[2]);

  // for (int i = 90; i < 180; i ++){
  //   t8_global_productionf("tm_1_sort[%i] = %f \n", i, tm_1_sort[i]);
  // }

  int length = 7;
  // int test_val[length] = {4050, 4070, 4090, 4099, 5001, 10078, 8490};
  // int test_val[length] = {990, 1310, 400, 670, 680, 140};
  int test_val[length] = {9, 23, 37, 40, 42, 43, 109};
  for (int i = 0; i < length; i ++){
    test_data(test_val[i], corr_levlength, latlength, lonlength, level1);
    t8_global_productionf("tm_1_sort[%i] = %f \n", test_val[i], tm_1_sort[test_val[i]]);
    t8_global_productionf("tm_1_sort[%i] = %f \n", test_val[i]+1, tm_1_sort[test_val[i]+1]);
    t8_global_productionf("geopot_sort[%i] = %f \n", test_val[i], geopot_sort[test_val[i]]);
    t8_global_productionf("geopot_sort[%i] = %f \n", test_val[i]+1, geopot_sort[test_val[i]+1]);
    float diff_temper = abs(tm_1_sort[test_val[i]]-tm_1_sort[test_val[i]+1]);
    float diff_geopot = abs(geopot_sort[test_val[i]]-geopot_sort[test_val[i]+1]);
    float diff_height = diff_geopot/9.81;
    t8_global_productionf("Difference temperature: %f & Difference geopot: %f & Difference height: %f", diff_temper, diff_geopot, diff_height);
    t8_global_productionf("This results in [%i] = %f \n", test_val[i], temp_grad_sort[test_val[i]]);
  }

  // for (int i = 4070; i < 4099; i ++){
  //   test_data(i, levlength, latlength, lonlength, level1);
  //   t8_global_productionf("tm_1_sort[%i] = %f \n", i, tm_1_sort[i]);
  // }

  // get_coords_of_morton (15, level1);
  // get_coords_of_morton (3015, level1);
  // get_coords_of_morton (3016, level1);
  // get_coords_of_morton (3017, level1);

  t8_global_productionf("level for quad scheme is %i \n", level1);
  get_morton_index(2, 3, level1);


  // // for testing:
  // level1 = 1;
  // level2 = 2;

  /*
  * Setup.
  * Build cmesh and uniform forest.
  */

  cmesh = t8_cmesh_new_row_of_cubes (num_cmesh, 1, 0, comm);
  t8_global_productionf (" [2_5D] Created coarse mesh.\n");
  forest = t8_forest_new_uniform_2_5D (cmesh, t8_scheme_new_2_5dimension_cxx (), level1, level2, 0, comm);

  /* Print information of the forest. */
  t8_global_productionf (" [2_5D] Created uniform forest.\n");
  t8_global_productionf (" [2_5D] Refinement level1:\t%i\n", level1);
  t8_global_productionf (" [2_5D] Refinement level2:\t%i\n", level2);
  t8_2_5D_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_uniform);
  t8_global_productionf (" [2_5D] Wrote uniform forest to vtu files: %s*\n", prefix_uniform);


  struct MESSy_data_per_element data = {
    // geopot_sort,
    height_sort, //muss noch berechnet werden
    tm_1_sort,
    temp_grad_sort
  };

  /*
   * Output the volume data to vtu.
   */
  t8_2_5D_output_data_to_vtu (forest, &data, prefix_forest_with_data);
  t8_global_productionf (" [step5] Wrote forest and volume data to %s*.\n", prefix_forest_with_data);

  struct MESSy_data adapt_data = {
    levlength,             
    latlength,
    lonlength,
    corr_levlength,
    tm_1_sort, /* temperature values according to 2^level2 */
    geopot_sort /* geopot values according to 2^level2 -> geopot/9.81 = height in m*/ 
  };

  t8_forest_set_user_data (forest, &adapt_data);


  nc_close (ncid);




  /*
   * clean-up
   */

  /* Destroy the forest. */
  t8_forest_unref (&forest);
  t8_global_productionf (" [2_5D] Destroyed forest.\n");

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

T8_EXTERN_C_END ();