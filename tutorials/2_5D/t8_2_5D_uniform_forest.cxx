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

#include <t8.h>                          /* General t8code header, always include this. */
#include <t8_cmesh.h>                    /* cmesh definition and basic interface. */
#include <t8_cmesh/t8_cmesh_examples.h>  /* A collection of exemplary cmeshes */
#include <t8_forest/t8_forest_general.h> /* forest definition and general interface. */
#include <t8_forest/t8_forest_io.h>      /* forest io interface. */
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx>
#include <t8_schemes/t8_2_5dimension/t8_mixed_scheme.hxx>
#include <tutorials/2_5D/t8_2_5D_vtu.hxx>

/* Builds cmesh of shapes for horizontal eclass (LINE (1.5D), QUAD or TRIANGLE) and for vertical eclass (LINE)
 * and thus total eclass T8_ECLASS_{QUAD,HEX,PRISM} that build up a unit cube.
 * \param [in] comm   MPI Communicator to use.
 * \return            The coarse mesh.
 */
static t8_cmesh_t
t8_2_5D_build_hypercube_coarse_mesh (sc_MPI_Comm comm, t8_eclass_t eclass)
{
  t8_cmesh_t cmesh;

  /* Build a coarse mesh 
  * first parameter: T8_ECLASS_QUAD, T8_ECLASS_HEX or T8_ECLASS_PRISM
   * The second argument is the MPI communicator to use for this cmesh.
   * The remaining arguments are 3 flags that control
   *   do_bcast     - If non-zero only the root process will build the cmesh and will broadcast it to the other processes. The result is the same.
   *   do_partition - If non-zero the cmesh will be partitioned among the processes. If 0 each process has a copy of the whole cmesh.
   *   periodic     - If non-zero the cube will have periodic boundaries. That is, i.e. the left face is connected to the right face.
   */

  cmesh = t8_cmesh_new_hypercube (eclass, comm, 0, 0, 0);
  //cmesh = t8_cmesh_new_row_of_cubes (2, 1, 0, comm);

  t8_global_productionf (" [2_5D] Constructed coarse mesh with T8_ECLASS_QUAD.\n");

  return cmesh;
}

/* Build a uniform 2.5 dimensional/anisotropic forest on a cmesh 
 * using the 2.5 dimensional/anisotropic refinement scheme.
 * \param [in] comm   MPI Communicator to use.
 * \param [in] cmesh  The coarse mesh to use.
 * \param [in] level1  The initial uniform refinement level in horizontal direction.
 * \param [in] level2  The initial uniform refinement level in vertical direction.
 * \return            A uniform 2.5 dimensional/anisotropic forest with the given refinement levels in horizontal 
 *                    and vertical direction that is partitioned across the processes in \a comm.
 */
static t8_forest_t
t8_2_5D_build_uniform_forest (sc_MPI_Comm comm, t8_cmesh_t cmesh, int level1, int level2)
{
  t8_forest_t forest;

  /* Create the refinement scheme. */

  forest
    = t8_forest_new_uniform_2_5D (cmesh, (const t8_scheme *) t8_scheme_new_2_5dimension (), level1, level2, 0, comm);

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

int
main (int argc, char **argv)
{
  int mpiret;
  sc_MPI_Comm comm;
  t8_eclass_t eclass;
  t8_cmesh_t cmesh;
  t8_forest_t forest;

  /* The prefix for our output files. */

  const char prefix[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST";
  const char prefix_highlight[BUFSIZ] = "t8_2_5D_UNIFORM_FOREST_HIGHLIGHT_Partition";

  /* The uniform refinement level of the forest. */
  const int level1 = 2;
  const int level2 = 1;
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
  t8_global_productionf (" [2_5D] Hello, this is the example for the 2.5D scheme of t8code (with the old cmesh).\n");
  t8_global_productionf (" [2_5D] In this example a we build a uniform 2.5D forest and output it to vtu files.\n");
  t8_global_productionf (" [2_5D] \n");

  /* We will use MPI_COMM_WORLD as a communicator. */
  comm = sc_MPI_COMM_WORLD;
  /* Create the cmesh */
  // eclass = T8_ECLASS_HEX;
  eclass = T8_ECLASS_PRISM;
  cmesh = t8_2_5D_build_hypercube_coarse_mesh (comm, eclass);
  /* Build the uniform forest, it is automatically partitioned among the processes. */
  forest = t8_2_5D_build_uniform_forest (comm, cmesh, level1, level2);
  /* Get the local number of elements. */
  local_num_elements = t8_forest_get_local_num_leaf_elements (forest);
  /* Get the global number of elements. */
  global_num_elements = t8_forest_get_global_num_leaf_elements (forest);

  /* Print information on the forest. */
  t8_global_productionf (" [2_5D] Created uniform forest.\n");
  t8_global_productionf (" [2_5D] Refinement level1:\t\t\t%i\n", level1);
  t8_global_productionf (" [2_5D] Refinement level2:\t\t\t%i\n", level2);
  t8_global_productionf (" [2_5D] Local number of elements:\t\t%i\n", local_num_elements);
  t8_global_productionf (" [2_5D] Global number of elements:\t\t%li\n", global_num_elements);

  /* Write forest to vtu files. */
  t8_2_5D_write_forest_vtk (forest, prefix);
  t8_global_productionf (" [2_5D] Wrote forest to vtu files:\t%s*\n", prefix);
  t8_global_productionf ("Eclass: %i, level1: %i, level2: %i", eclass, level1, level2);

  double *highlight = T8_ALLOC_ZERO (double, global_num_elements);
  highlight[3] = 1;

  t8_2_5D_output_data_to_vtu (forest, level1, level2, highlight, prefix_highlight);

  /* Destroy the forest. */
  t8_2_5D_destroy_forest (forest);
  t8_global_productionf (" [2_5D] Destroyed forest.\n");

  T8_FREE (highlight);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
