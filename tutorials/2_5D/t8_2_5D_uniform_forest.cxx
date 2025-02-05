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

#include <t8.h>                         /* General t8code header, always include this. */
#include <t8_cmesh.h>                   /* cmesh definition and basic interface. */
//#include <t8_cmesh_vtk_writer.h>        /* cmesh-writer interface. */
#include <t8_cmesh/t8_cmesh_examples.h> /* A collection of exemplary cmeshes */
#include <t8_forest/t8_forest_general.h>            /* forest definition and general interface. */
#include <t8_forest/t8_forest_io.h>                 /* forest io interface. */
#include <t8_schemes/t8_default/t8_default.hxx> /* default refinement scheme. */

#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx>


#include <t8_schemes/t8_default/t8_default_line/t8_default_line.hxx>
#include <t8_schemes/t8_default/t8_default_quad/t8_default_quad.hxx>

//needed to write SFC index
#include "t8_forest/t8_forest_types.h"



/* Builds cmesh of shapes for eclass1 (LINE (1.5D), QUAD or TRIANGLE) and for eclass1 (LINE) that build up a unit cube.
 * \param [in] comm   MPI Communicator to use.
 * \return            The coarse mesh.
 */
static t8_cmesh_t
// t8_2_5D_build_hypercube_coarse_mesh (t8_eclass_t eclass1, t8_eclass_t eclass2, sc_MPI_Comm comm)
t8_2_5D_build_hypercube_coarse_mesh (sc_MPI_Comm comm)
{
  t8_cmesh_t cmesh;

  /* Build a coarse mesh of 1 linear/1 quadrilateral or 2 triangular tree/trees and 1linear tree that form a cube.
   * You can modify the first and second parameter to build a cube with different
   * tree shapes, i.e. T8_ECLASS_QUAD and T8_ECLASS_LINE for a unit cune with 1 quadrilateral tree and 1 linear tree.
   * See t8_eclass.h, t8_cmesh.h for all possible shapes.
   * 
   * The third argument is the MPI communicator to use for this cmesh.
   * The remaining arguments are 3 flags that control
   *   do_bcast     - If non-zero only the root process will build the cmesh and will broadcast it to the other processes. The result is the same.
   *   do_partition - If non-zero the cmesh will be partitioned among the processes. If 0 each process has a copy of the whole cmesh.
   *   periodic     - If non-zero the cube will have periodic boundaries. That is, i.e. the left face is connected to the right face.
   */
  // cmesh = t8_cmesh_new_hypercube (T8_ECLASS_QUAD, comm, 0, 0, 0);
  // cmesh = t8_cmesh_new_hypercube (T8_ECLASS_PRISM, comm, 0, 0, 0);
  //cmesh = t8_cmesh_new_row_of_cubes (2, 1, 0, comm);
  cmesh = t8_cmesh_new_hypercube (T8_ECLASS_HEX, comm, 0, 0, 0);
  // // cmesh = t8_cmesh_new_hypercube_hybrid (comm, 0, 0); //Abort

  // t8_global_productionf (" [2_5D] Constructed coarse mesh with T8_ECLASS_QUAD.\n");
  // t8_global_productionf (" [2_5D] Constructed coarse mesh with T8_ECLASS_PRISM.\n");
  t8_global_productionf (" [2_5D] Constructed coarse mesh with T8_ECLASS_HEX.\n");

  return cmesh;
}

/* Build a uniform forest on a cmesh 
 * using the default refinement scheme.
 * \param [in] comm   MPI Communicator to use.
 * \param [in] cmesh  The coarse mesh to use.
 * \param [in] level  The initial uniform refinement level.
 * \return            A uniform forest with the given refinement level that is
 *                    partitioned across the processes in \a comm.
 */
static t8_forest_t
t8_2_5D_build_uniform_forest (sc_MPI_Comm comm, t8_cmesh_t cmesh, int level1, int level2)
{
  t8_forest_t forest;
  t8_scheme_cxx_t *scheme;

  /* Create the refinement scheme. */
  scheme = t8_scheme_new_2_5dimension_cxx ();

  // t8_global_productionf (" [2_5D] Build uniform forest.\n");

  // /* Creat the uniform forest. */
  // //forest = t8_forest_new_uniform (cmesh, scheme, level, 0, comm);
  // // std::vector level = {level1, level2};
  
  forest = t8_forest_new_uniform_2_5D (cmesh, scheme, level1, level2, 0, comm);

  return forest;
}

/* Write vtk (or more accurately vtu) files of the forest.
 * \param [in] forest   A forest.
 * \param [in] prefix   A string that is used as a prefix of the output files.
 * 
 * This will create the file prefix.pvtu
 * and additionally one file prefix_MPIRANK.vtu per MPI rank.
 */
static void
t8_2_5D_write_forest_vtk (t8_forest_t forest, const char *prefix)
{
  t8_forest_write_vtk (forest, prefix);
}

/* Destroy a forest. This will free all allocated memory.
 * \param [in] forest    A forest.
 * NOTE: This will also free the memory of the scheme and the cmesh, since
 *       the forest took ownership of them.
 *       If we do not want this behaviour, but want to reuse for example the cmesh,
 *       we need to call t8_cmesh_ref (cmesh) before passing it to t8_forest_new_uniform.
 */
static void
t8_2_5D_destroy_forest (t8_forest_t forest)
{
  t8_forest_unref (&forest);
}

// /* Write the forest as vtu and also write the highlighted element in the file.
//  * 
//  * t8code supports writing element based data to vtu as long as its stored
//  * as doubles. Each of the data fields to write has to be provided in its own
//  * array of length num_local_elements.
//  * We support two types: T8_VTK_SCALAR - One double per element
//  *                  and  T8_VTK_VECTOR - 3 doubles per element
//  */
// static void
// t8_2_5D_output_data_to_vtu (t8_forest_t forest, double *array, const char *prefix)
// {
//   t8_locidx_t num_elements = t8_forest_get_local_num_elements (forest);
//   t8_locidx_t ielem;
//   /* We need to allocate a new array to store the volumes on their own.
//    * The arrays have one entry per local element. */
//   double *highlight = T8_ALLOC (double, num_elements);
//   /* The number of user defined data fields to write. */
//   int num_data = 1;
//   /* For each user defined data field we need one t8_vtk_data_field_t variable */
//   t8_vtk_data_field_t vtk_data;
//   /* Set the type of this variable. Since we have for each array one value per element, we pick T8_VTK_SCALAR */
//   vtk_data.type = T8_VTK_SCALAR;
//   /* The name of the field as should be written to the file. */
//   strcpy (vtk_data.description, "highlight");
//   vtk_data.data = highlight;
//   /* Copy the element's height from our data array to the output array. */
//   for (ielem = 0; ielem < num_elements; ++ielem) {
//     highlight[ielem] = array[ielem];
//   }
//   {
//     /* To write user defined data, we need to extended output function t8_forest_vtk_write_file
//      * from t8_forest_vtk.h. Despite writing user data, it also offers more control over which 
//      * properties of the forest to write. */
//     int write_treeid = 1;
//     int write_mpirank = 1;
//     int write_level = 1;
//     int write_element_id = 1;
//     int write_ghosts = 0;
//     t8_forest_write_vtk_ext (forest, prefix, write_treeid, write_mpirank, write_level, write_element_id, write_ghosts,
//                              0, 0, num_data, &vtk_data);
//   }
//   /* clean-up */
//   T8_FREE (highlight);
// }

/* Write the forest as vtu and also write the highlighted element in the file.
 * 
 * t8code supports writing element based data to vtu as long as its stored
 * as doubles. Each of the data fields to write has to be provided in its own
 * array of length num_local_elements.
 * We support two types: T8_VTK_SCALAR - One double per element
 *                  and  T8_VTK_VECTOR - 3 doubles per element
 */
static void
t8_2_5D_output_data_to_vtu (t8_forest_t forest, int level1, int level2, double *array, const char *prefix)
{
  t8_locidx_t num_elements = t8_forest_get_local_num_elements (forest);
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
  strcpy (vtk_data[1].description, "SFCindex");
  vtk_data[1].data = sfc_index;
  /* Copy the element's height from our data array to the output array. */
  t8_tree_t tree;
  t8_locidx_t itree;
  t8_locidx_t element_index, elems_in_tree;
  t8_locidx_t element_index_in_tree;
  t8_locidx_t num_global_trees;
  t8_locidx_t num_local_trees;
  t8_element_t *element;
  t8_eclass_scheme_c *scheme;
  //t8_element_t **elements = T8_ALLOC (t8_element_t *, num_elements);
  num_global_trees = t8_forest_get_num_global_trees (forest);
  t8_global_productionf ("num_global_trees: %i", num_global_trees);
  element_index = 0;
  element_index_in_tree = 0;

  for (itree = 0; itree < num_global_trees; itree++) {
    /* Get the tree that stores the elements */
    num_local_trees = t8_forest_get_num_local_trees (forest);
    if (itree < num_local_trees){
      tree = t8_forest_get_tree (forest, itree); 
      /* Get the eclass scheme of the tree */
      scheme = t8_forest_get_eclass_scheme (forest, t8_forest_get_tree_class (forest, itree));
      elems_in_tree = (t8_locidx_t) t8_element_array_get_count (&tree->elements);
      element_index_in_tree += elems_in_tree;
      for (element_index; element_index < element_index_in_tree; element_index++) {
        /* Get a pointer to the element */
        element = t8_forest_get_element (forest, tree->elements_offset + element_index, NULL);
        std::vector<int> levels = {level1, level2};
        sfc_index[element_index] = (scheme->t8_element_get_linear_id (element, levels));
      }
      element_index += elems_in_tree;
      t8_global_productionf ("num_elements: %li \n", num_elements);
      t8_global_productionf ("element_index_in_tree: %li \n", element_index_in_tree);
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

int
main (int argc, char **argv)
{
  int mpiret;
  sc_MPI_Comm comm;
  t8_cmesh_t cmesh;
  t8_forest_t forest;

  /* The prefix for our output files. */
  // const char prefix[BUFSIZ] = "t8_1_5D_UNIFORM_FOREST_LINE_LINE";
  // const char prefix[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_TRI_LINE_2_1_Partition"; 
  const char prefix[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_QUAD_LINE_1_2"; 
  // const char prefix[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_TEST"; 
  // const char prefix[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_TRI_LINE_1_3";
  // const char prefix_highlight[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_QUAD_LINE_1_3_HIGHLIGHT";
  // const char prefix_highlight[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_TRI_LINE_2_1_HIGHLIGHT_Partition";
  const char prefix_highlight[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_QUAD_LINE_1_1_HIGHLIGHT_Partition";

  /* The uniform refinement level of the forest. */
  const int level1 = 1;
  const int level2 = 2;
  t8_locidx_t local_num_elements;
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
  t8_global_productionf (" [2_5D] Hello, this is the example for the 2_5D scheme of t8code (with the old cmesh).\n");
  t8_global_productionf (" [2_5D] In this example we build a uniform forest for two eclasses and output it to vtu files.\n");
  t8_global_productionf (" [2_5D] \n");

    /* We will use MPI_COMM_WORLD as a communicator. */
  comm = sc_MPI_COMM_WORLD;
  /* Create the cmesh */
  // cmesh = t8_2_5D_build_hypercube_coarse_mesh (T8_ECLASS_LINE, T8_ECLASS_LINE, comm);
  // cmesh = t8_2_5D_build_hypercube_coarse_mesh (T8_ECLASS_QUAD, T8_ECLASS_LINE, comm);
  // cmesh = t8_2_5D_build_hypercube_coarse_mesh (T8_ECLASS_TRIANGLE, T8_ECLASS_LINE, comm);
  cmesh = t8_2_5D_build_hypercube_coarse_mesh (comm);
  /* Build the uniform forest, it is automatically partitioned among the processes. */
  forest = t8_2_5D_build_uniform_forest (comm, cmesh, level1, level2);
  /* Get the local number of elements. */
  local_num_elements = t8_forest_get_local_num_elements (forest);
  /* Get the global number of elements. */
  global_num_elements = t8_forest_get_global_num_elements (forest);

  /* Print information on the forest. */
  t8_global_productionf (" [2_5D] Created uniform forest.\n");
  t8_global_productionf (" [2_5D] Refinement level1:\t\t\t%i\n", level1);
  t8_global_productionf (" [2_5D] Refinement level2:\t\t\t%i\n", level2);
  t8_global_productionf (" [2_5D] Local number of elements:\t\t%i\n", local_num_elements);
  t8_global_productionf (" [2_5D] Global number of elements:\t%li\n", global_num_elements);

  /* Write forest to vtu files. */
  t8_2_5D_write_forest_vtk (forest, prefix);
  t8_global_productionf (" [2_5D] Wrote forest to vtu files:\t%s*\n", prefix);

  double *highlight = T8_ALLOC_ZERO (double, global_num_elements);
  highlight[3] = 1;

  t8_2_5D_output_data_to_vtu(forest, level1, level2, highlight, prefix_highlight);

  /* Destroy the forest. */
  t8_2_5D_destroy_forest (forest);
  t8_global_productionf (" [2_5D] Destroyed forest.\n");

  T8_FREE(highlight);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
