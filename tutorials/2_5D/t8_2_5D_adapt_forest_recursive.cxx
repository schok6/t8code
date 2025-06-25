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

/* This is the adapt implementation of t8code for a 2_5 dimensional/anisotropic forest.
 * 
 * 
 * Note: Non-recursive refinement is used, that means that the refinement level
 * of any element will change by at most +-1.
 * 
 */

#include <t8.h>                                           /* General t8code header, always include this. */
#include <t8_cmesh.h>                                     /* cmesh definition and basic interface. */
#include <t8_cmesh/t8_cmesh_examples.h>                   /* A collection of exemplary cmeshes */
#include <t8_forest/t8_forest_general.h>                  /* forest definition and basic interface. */
#include <t8_forest/t8_forest_io.h>                       /* save forest */
#include <t8_forest/t8_forest_geometrical.h>              /* geometrical information of the forest */
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx> /* 2_5D refinement scheme. */
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx> /* default refinement scheme. */
#include <t8_types/t8_vec.h>                              /* Basic operations on 3D vectors. */
#include <tutorials/2_5D/t8_2_5D_adapt.hxx>
#include <t8_forest/t8_forest_types.h> /* needed to write SFC indices */

/* Build a uniform anisotropic forest/ a uniform biforest on a cmesh 
 * using the 2.5 dimensional refinement scheme.
 * \param [in] comm   MPI Communicator to use.
 * \param [in] cmesh  The coarse mesh to use.
 * \param [in] level1  The initial uniform refinement level in horizontal direction.
 * \param [in] level2  The initial uniform refinement level in vertical direction.
 * \return            A uniform biforest with the given refinement level1 and level2 that is
 *                    partitioned across the processes in \a comm.
 */
static t8_forest_t
t8_2_5D_build_uniform_forest (sc_MPI_Comm comm, t8_cmesh_t cmesh, int level1, int level2)
{
  t8_forest_t forest;

  const t8_scheme *scheme = t8_scheme_new_2_5dimension ();

  forest = t8_forest_new_uniform_2_5D (cmesh, scheme, level1, level2, 0, comm);

  return forest;
}

/* This is our own defined data that we will pass on to the
 * adaptation callback. */

/* There is one adaptation callback for each direction, horizontal and vertical. Depending on the considered direction 
 * the corresponding callback function is called. The function will be called once for each element
 * and the return value decides whether this element should be refined or not.
 *   return > 0 -> This element should get refined in the considered direction.
 *   return = 0 -> This element should not get refined.
 * If the current element is the first element of a family regarding the considered direction (= all level l elements that arise from refining
 * the same level l-1 element) then this function is called with the whole family of elements
 * as input and the return value additionally decides whether the whole family should get coarsened.
 *   return > 0 -> This first element should get refined in the considered direction.
 *   return = 0 -> The first element should not get refined.
 *   return < 0 -> The whole family should get coarsened in the considered direction..
 *  
 * \param [in] forest       The current biforest that is in construction.
 * \param [in] forest_from  The biforest from which we adapt the current biforest (in our case, the uniform biforest)
 * \param [in] which_tree   The process local id of the current tree.
 * \param [in] lelement_id  The tree local index of the current element (or the first of the family).
 * \param [in] scheme       The refinement scheme for this tree's element class.
 * \param [in] is_family    if 1, the first \a num_elements entries in \a elements form a family. If 0, they do not.
 * \param [in] num_elements The number of entries in \a elements elements that are defined.
 * \param [in] elements     The element or family of elements to consider for refinement/coarsening.
 */
int
t8_2_5D_adapt_callback_horizontal (t8_forest_t forest, t8_forest_t forest_from, t8_locidx_t which_tree,
                                   [[maybe_unused]] t8_eclass_t tree_class, [[maybe_unused]] t8_locidx_t lelement_id,
                                   [[maybe_unused]] const t8_scheme *scheme, const int is_family,
                                   [[maybe_unused]] const int num_elements, t8_element_t *elements[])
{
  /* Our adaptation criterion is to look at the midpoint coordinates of the current element and if
   * they are inside a sphere around a given midpoint we refine, if they are outside, we coarsen. */
  double centroid[3]; /* Will hold the element midpoint. */
  /* In t8_2_5D_adapt_forest we pass a t8_2_5D_adapt_data pointer as user data to the
   * t8_forest_new_adapt function. This pointer is stored as the used data of the new biforest
   * and we can now access it with t8_forest_get_user_data (forest). */
  const struct t8_2_5D_adapt_data *adapt_data = (const struct t8_2_5D_adapt_data *) t8_forest_get_user_data (forest);
  double dist; /* Will store the distance of the element's midpoint and the sphere midpoint. */

  T8_ASSERT (adapt_data != NULL);

  /* Compute the element's centroid coordinates. */
  t8_forest_element_centroid (forest_from, which_tree, elements[0], centroid);

  /* Compute the distance to our sphere midpoint. */
  /* TODO: write distance function which considers only the horizontal coordinates */
  dist = t8_dist (centroid, adapt_data->midpoint);
  if (dist < adapt_data->refine_if_inside_radius) {
    /* Refine this element. */
    return 1;
  }
  else if (is_family && dist > adapt_data->coarsen_if_outside_radius) {
    /* Coarsen this family. Note that we check for is_family before, since returning < 0
     * if we do not have a family as input is illegal. */
    return -1;
  }
  /* Do not change this element. */
  return 0;
}

/* Adapt a biforest according to our t8_2_5D_adapt_callback function in horizontal directions.
 * This will create a new biforest and return it. */
t8_forest_t
t8_2_5D_adapt_forest_horizontal (t8_forest_t forest)
{
  t8_forest_t forest_adapt;
  struct t8_2_5D_adapt_data adapt_data = {
    { 0, 0, 0 }, /* Midpoints of the sphere. */
    0.5,         /* Refine if inside this radius. */
    sqrt (0.5)   /* Coarsen if outside this radius. */
  };

  /* Check that forest is a committed, that is valid and usable, forest. */
  T8_ASSERT (t8_forest_is_committed (forest));

  /* Create a new biforest that is adapted from \a forest with our adaptation callback.
   * We provide the adapt_data as user data that is stored as the used_data pointer of the
   * new forest (see also t8_forest_set_user_data).
   * The 0, 0, 1 arguments are flags that control
   *   recursive  -    If non-zero adaptation is recursive, thus if an element is adapted the children
   *                   or parents are plugged into the callback again recursively until the forest does not
   *                   change any more. If you use this you should ensure that refinement will stop eventually.
   *                   One way is to check the element's level against a given maximum level.
   *   do_face_ghost - If non-zero additionally a layer of ghost elements is created for the forest.
   *                   We will discuss ghost in later steps of the tutorial.
   *   direction     - If one a biforest is adapted in horizontal directions, zero for a isotropic forest and two if
   *                   a biforest is adapted in vertical direction.
   */
  forest_adapt = t8_forest_new_adapt (forest, t8_2_5D_adapt_callback_horizontal, 0, 0, 1, &adapt_data);

  return forest_adapt;
}

/* There is one adaptation callback for each direction, horizontal and vertical. Depending on the considered direction 
  * the corresponding callback function is called. The function will be called once for each element
 * and the return value decides whether this element should be refined or not.
 *   return > 0 -> This element should get refined in the considered direction.
 *   return = 0 -> This element should not get refined.
 * If the current element is the first element of a family regarding the considered direction (= all level l elements that arise from refining
 * the same level l-1 element) then this function is called with the whole family of elements
 * as input and the return value additionally decides whether the whole family should get coarsened.
 *   return > 0 -> This first element should get refined in the considered direction.
 *   return = 0 -> The first element should not get refined.
 *   return < 0 -> The whole family should get coarsened in the considered direction..
 *  
 * \param [in] forest       The current biforest that is in construction.
 * \param [in] forest_from  The biforest from which we adapt the current biforest (in our case, the uniform biforest)
 * \param [in] which_tree   The process local id of the current tree.
 * \param [in] lelement_id  The tree local index of the current element (or the first of the family).
 * \param [in] scheme       The refinement scheme for this tree's element class.
 * \param [in] is_family    if 1, the first \a num_elements entries in \a elements form a family. If 0, they do not.
 * \param [in] num_elements The number of entries in \a elements elements that are defined.
 * \param [in] elements     The element or family of elements to consider for refinement/coarsening.
 */
int
t8_2_5D_adapt_callback_vertical (t8_forest_t forest, t8_forest_t forest_from, t8_locidx_t which_tree,
                                 [[maybe_unused]] t8_eclass_t tree_class, [[maybe_unused]] t8_locidx_t lelement_id,
                                 [[maybe_unused]] const t8_scheme *scheme, const int is_family,
                                 [[maybe_unused]] const int num_elements, t8_element_t *elements[])
{
  /* Our adaptation criterion is to look at the midpoint coordinates of the current element and if
   * they are inside a sphere around a given midpoint we refine, if they are outside, we coarsen. */
  double centroid[3]; /* Will hold the element midpoint. */
  /* In t8_2_5D_adapt_forest we pass a t8_2_5D_adapt_data pointer as user data to the
   * t8_forest_new_adapt function. This pointer is stored as the used data of the new forest
   * and we can now access it with t8_forest_get_user_data (forest). */
  const struct t8_2_5D_adapt_data *adapt_data = (const struct t8_2_5D_adapt_data *) t8_forest_get_user_data (forest);
  double dist; /* Will store the distance of the element's midpoint and the sphere midpoint. */

  T8_ASSERT (adapt_data != NULL);

  /* Compute the element's centroid coordinates. */
  t8_forest_element_centroid (forest_from, which_tree, elements[0], centroid);

  /* Compute the distance to our sphere midpoint. */
  /* TODO: write distance function which considers only the vertical coordinate */
  dist = t8_dist (centroid, adapt_data->midpoint);
  if (dist < adapt_data->refine_if_inside_radius) {
    /* Refine this element. */
    return 1;
  }
  else if (is_family && dist > adapt_data->coarsen_if_outside_radius) {
    /* Coarsen this family. Note that we check for is_family before, since returning < 0
     * if we do not have a family as input is illegal. */
    return -1;
  }
  /* Do not change this element. */
  return 0;
}

/* Adapt a forest according to our t8_2_5D_adapt_callback function in vertical direction.
 * This will create a new biforest and return it. */
t8_forest_t
t8_2_5D_adapt_forest_vertical (t8_forest_t forest)
{
  t8_forest_t forest_adapt;

  struct t8_2_5D_adapt_data adapt_data = {
    { 0.375, 0.0, 1.0 }, /* Midpoints of the sphere. */
    0.2,                 /* Refine if inside this radius. */
    0.4                  /* Coarsen if outside this radius. */
  };
  /* Check that forest is a committed, that is valid and usable, forest. */
  T8_ASSERT (t8_forest_is_committed (forest));

  /* Create a new biforest that is adapted from \a forest with our adaptation callback.
   * We provide the adapt_data as user data that is stored as the used_data pointer of the
   * new forest (see also t8_forest_set_user_data).
   * The 0, 0, 1 arguments are flags that control
   *   recursive  -    If non-zero adaptation is recursive, thus if an element is adapted the children
   *                   or parents are plugged into the callback again recursively until the forest does not
   *                   change any more. If you use this you should ensure that refinement will stop eventually.
   *                   One way is to check the element's level against a given maximum level.
   *   do_face_ghost - If non-zero additionally a layer of ghost elements is created for the forest.
   *                   We will discuss ghost in later steps of the tutorial.
   *   direction     - If two a biforest is adapted in vertical direction, zero for a isotropic forest and one if
   *                   a biforest is adapted in horizonatal directions.
   */
  forest_adapt = t8_forest_new_adapt (forest, t8_2_5D_adapt_callback_vertical, 0, 0, 2, &adapt_data);

  return forest_adapt;
}

/* Print the local and global number of elements of a forest. */
void
t8_2_5D_adapt_print_forest_information (t8_forest_t forest)
{
  t8_locidx_t local_num_elements;
  t8_gloidx_t global_num_elements;

  /* Check that forest is a committed, that is valid and usable, forest. */
  T8_ASSERT (t8_forest_is_committed (forest));

  /* Get the local number of elements. */
  local_num_elements = t8_forest_get_local_num_leaf_elements (forest);
  /* Get the global number of elements. */
  global_num_elements = t8_forest_get_global_num_leaf_elements (forest);
  t8_productionf (" [2_5D] Local number of elements:\t\t%i\n", local_num_elements);
  t8_global_productionf (" [2_5D] Global number of elements:\t\t%li\n", global_num_elements);
}

int
t8_2_5D_adapt_main (int argc, char **argv)
{
  int mpiret;
  sc_MPI_Comm comm;
  t8_cmesh_t cmesh;
  t8_forest_t forest;

  /* The prefix for our output files. */
  const char *prefix_uniform = "t8_2_5D_uniform_first_horizontal_then_vertical";
  const char prefix_uniform_highlight[BUFSIZ] = "t8_2_5D_uniform_first_horizontal_then_vertical_highlight";
  const char *prefix_adapt_horizontal = "t8_2_5D_adapted_forest_horizontal_REC";
  const char prefix_adapt_horizontal_highlight[BUFSIZ] = "t8_2_5D_adapted_forest_horizontal_highlight";
  const char *prefix_adapt_vertical = "t8_2_5D_adapted_forest_vertical_REC";
  const char prefix_adapt_vertical_highlight[BUFSIZ] = "t8_2_5D_adapted_forest_vertical_highlight";

  /* The uniform refinement levels of the forest. */
  const int level1 = 3;  //horizontal refinement level
  const int level2 = 3;  //vertical refinement level

  t8_gloidx_t global_num_elements;

  /* Initialize MPI. This has to happen before we initialize sc or t8code. */
  mpiret = sc_MPI_Init (&argc, &argv);
  /* Error check the MPI return value. */
  SC_CHECK_MPI (mpiret);

  /* Initialize the sc library, has to happen before we initialize t8code. */
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEBUG);
  /* Initialize t8code with log level SC_LP_PRODUCTION. See sc.h for more info on the log levels. */
  t8_init (SC_LP_DEBUG);

  /* Print a message on the root process. */
  t8_global_productionf (" [2_5D] \n");
  t8_global_productionf (" [2_5D] Hello, this is the 2_5D adapt example of t8code.\n");
  t8_global_productionf (" [2_5D] In this example we will refine and coarsen a forest.\n");
  t8_global_productionf (" [2_5D] \n");

  /* We will use MPI_COMM_WORLD as a communicator. */
  comm = sc_MPI_COMM_WORLD;

  /*
   * Setup.
   * Build cmesh and uniform forest.
   */

  /* Build a cube cmesh with hex or prism trees. */
  // cmesh = t8_cmesh_new_hypercube (T8_ECLASS_QUAD, comm, 0, 0, 0);
  // cmesh = t8_cmesh_new_hypercube (T8_ECLASS_PRISM, comm, 0, 0, 0);
  cmesh = t8_cmesh_new_hypercube (T8_ECLASS_HEX, comm, 0, 0, 0);
  t8_global_productionf (" [2_5D] Created coarse mesh.\n");
  forest = t8_2_5D_build_uniform_forest (comm, cmesh, level1, level2);

  /* Get the global number of elements. */
  global_num_elements = t8_forest_get_global_num_leaf_elements (forest);

  /* Print information of the forest. */
  t8_global_productionf (" [2_5D] Created uniform forest.\n");
  t8_global_productionf (" [2_5D] Horizontal refinement level:\t%i\n", level1);
  t8_global_productionf (" [2_5D] Vertical refinement level:\t%i\n", level2);
  t8_2_5D_adapt_print_forest_information (forest);
  /* create graphics with a marked element*/
  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_uniform);
  t8_global_productionf (" [2_5D] Wrote uniform forest to vtu files: %s*\n", prefix_uniform);

  /* create graphics with a marked element*/
  double *highlight = T8_ALLOC_ZERO (double, global_num_elements);
  // for (int i = 0; i < 16; i++) {
  //   highlight[i] = 1;
  // }
  // for (int i = 22; i < 24; i++) {
  //   highlight[i] = 2;
  // }

  // t8_2_5D_output_data_to_vtu(forest, level1, level2, highlight, prefix_uniform_highlight);

  T8_FREE (highlight);

  /*
   *  Adapt the forest horizontal.
   */

  /* Adapt the biforest horizontally. We can reuse the forest variable, since the new adapted
   * biforest will take ownership of the old biforest and destroy it.
   * Note that the adapted biforest is a new biforest, though. */
  forest = t8_2_5D_adapt_forest_horizontal (forest);

  /*
   *  Output.
   */

  /* Print information of our new biforest. */
  t8_global_productionf (" [2_5D] Adapted biforest horizontally.\n");
  t8_2_5D_adapt_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt_horizontal);
  t8_global_productionf (" [2_5D] Wrote horizontally adapted biforest to vtu files: %s*\n", prefix_adapt_horizontal);

  /* Get the global number of elements of adapted forest. */
  global_num_elements = t8_forest_get_global_num_leaf_elements (forest);

  /* create graphics with a marked element*/
  double *highlight_adapt_horizontal = T8_ALLOC_ZERO (double, global_num_elements);
  for (int i = 0; i < 4; i++) {
    highlight_adapt_horizontal[i] = 1;
  }
  for (int i = 10; i < 12; i++) {
    highlight_adapt_horizontal[i] = 2;
  }

  t8_2_5D_output_data_to_vtu (forest, level1 + 1, level2, highlight_adapt_horizontal,
                              prefix_adapt_horizontal_highlight);

  T8_FREE (highlight_adapt_horizontal);

  /*
   *  Adapt the forest verical.
   */

  /* Adapt the biforest vertically. We can reuse the forest variable, since the new adapted
   * biforest will take ownership of the old biforest and destroy it.
   * Note that the adapted biforest is a new biforest, though. */
  forest = t8_2_5D_adapt_forest_vertical (forest);
  //forest = t8_2_5D_adapt_forest_vertical (forest);

  /*
   *  Output.
   */

  /* Print information of our new forest. */
  t8_global_productionf (" [2_5D] Adapted biforest.\n");
  t8_2_5D_adapt_print_forest_information (forest);

  /* Write forest to vtu files. */
  t8_forest_write_vtk (forest, prefix_adapt_vertical);
  t8_global_productionf (" [2_5D] Wrote vertically adapted forest to vtu files: %s*\n", prefix_adapt_vertical);

  /* Get the global number of elements of adapted forest. */
  global_num_elements = t8_forest_get_global_num_leaf_elements (forest);

  /* create graphics with a marked element*/
  double *highlight_adapt_vertical = T8_ALLOC_ZERO (double, global_num_elements);
  // for (int i=0; i<16; i++){
  //   highlight_adapt_vertical[i] = 1;
  // }
  // for (int i=53; i<55; i++){
  //   highlight_adapt_vertical[i] = 2;
  // }
  // highlight_adapt_vertical[9]=1;
  // highlight_adapt_vertical[21]=1;

  t8_2_5D_output_data_to_vtu (forest, level1 + 1, level2 + 2, highlight_adapt_vertical,
                              prefix_adapt_vertical_highlight);
  t8_2_5D_output_data_to_vtu (forest, level1 + 1, level2 + 1, highlight_adapt_vertical,
                              prefix_adapt_vertical_highlight);

  T8_FREE (highlight_adapt_vertical);

  /*
   * clean-up
   */

  /* Destroy the forest. */
  t8_forest_unref (&forest);
  t8_global_productionf (" [2_5D] Destroyed biforest.\n");

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
