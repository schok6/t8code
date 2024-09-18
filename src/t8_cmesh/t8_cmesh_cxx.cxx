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

#include <t8_cmesh.h>
#include <t8_element_cxx.hxx>
#include "t8_cmesh_types.h"

/** \file t8_cmesh_cxx.cxx
 *  This file collects all general cmesh routines that need c++ compilation.
 *  Particularly those functions that use the element interface from \ref t8_element_cxx.hxx.
 *
 * TODO: document this file
 */

void
t8_cmesh_uniform_bounds (t8_cmesh_t cmesh, int level, t8_scheme_cxx_t *ts, t8_gloidx_t *first_local_tree,
                         t8_gloidx_t *child_in_tree_begin, t8_gloidx_t *last_local_tree, t8_gloidx_t *child_in_tree_end,
                         int8_t *first_tree_shared)
{
  int is_empty;

  T8_ASSERT (cmesh != NULL);
  T8_ASSERT (cmesh->committed);
  T8_ASSERT (level >= 0);
  T8_ASSERT (ts != NULL);

  *first_local_tree = 0;
  if (child_in_tree_begin != NULL) {
    *child_in_tree_begin = 0;
  }
  *last_local_tree = 0;
  if (child_in_tree_end != NULL) {
    *child_in_tree_end = 0;
  }

  t8_gloidx_t global_num_children;
  t8_gloidx_t first_global_child;
  t8_gloidx_t child_in_tree_begin_temp;
  t8_gloidx_t last_global_child;
  t8_gloidx_t children_per_tree = 0;
#ifdef T8_ENABLE_DEBUG
  t8_gloidx_t prev_last_tree = -1;
#endif
  int tree_class;
  t8_eclass_scheme_c *tree_scheme;

  /* Compute the number of children on level in each tree */
  global_num_children = 0;
  for (tree_class = T8_ECLASS_ZERO; tree_class < T8_ECLASS_COUNT; ++tree_class) {
    /* We iterate over each element class and get the number of children for this
     * tree class.
     */
    if (cmesh->num_trees_per_eclass[tree_class] > 0) {
      tree_scheme = ts->eclass_schemes[tree_class];
      T8_ASSERT (tree_scheme != NULL);
      children_per_tree = tree_scheme->t8_element_count_leaves_from_root (level);
      T8_ASSERT (children_per_tree >= 0);
      global_num_children += cmesh->num_trees_per_eclass[tree_class] * children_per_tree;
    }
  }
  T8_ASSERT (children_per_tree != 0);

  if (cmesh->mpirank == 0) {
    first_global_child = 0;
    // if (child_in_tree_begin != NULL) {
    //   *child_in_tree_begin = 0;
    // }
  }
  else {
    /* The first global child of processor p
     * with P total processor is (the biggest int smaller than)
     * (total_num_children * p) / P
     * We cast to long double and double first to prevent integer overflow.
     */
    first_global_child = ((long double) global_num_children * cmesh->mpirank) / (double) cmesh->mpisize;
  }
  if (cmesh->mpirank != cmesh->mpisize - 1) {
    last_global_child = ((long double) global_num_children * (cmesh->mpirank + 1)) / (double) cmesh->mpisize;
  }
  else {
    last_global_child = global_num_children;
  }

  T8_ASSERT (0 <= first_global_child && first_global_child <= global_num_children);
  T8_ASSERT (0 <= last_global_child && last_global_child <= global_num_children);

  *first_local_tree = first_global_child / children_per_tree;
  child_in_tree_begin_temp = first_global_child - *first_local_tree * children_per_tree; //What happens here? Always 0 if children_per_tree != 0? @Chiara
  if (child_in_tree_begin != NULL) {
    *child_in_tree_begin = child_in_tree_begin_temp;
  }

  *last_local_tree = (last_global_child - 1) / children_per_tree;

  is_empty = *first_local_tree >= *last_local_tree && first_global_child >= last_global_child;
  if (first_tree_shared != NULL) {
#ifdef T8_ENABLE_DEBUG
    prev_last_tree = (first_global_child - 1) / children_per_tree;
    T8_ASSERT (cmesh->mpirank > 0 || prev_last_tree <= 0);
#endif
    if (!is_empty && cmesh->mpirank > 0 && child_in_tree_begin_temp > 0) {
      /* We exclude empty partitions here, by def their first_tree_shared flag is zero */
      /* We also exclude that the previous partition was empty at the beginning of the
       * partitions array */
      /* We also exclude the case that we have the first global element but
       * are not rank 0. */
      *first_tree_shared = 1;
    }
    else {
      *first_tree_shared = 0;
    }
  }
  t8_global_productionf("*last_local_tree: %li \n", *last_local_tree);
  t8_global_productionf("child_in_tree_end: %li \n", *child_in_tree_end);
  if (child_in_tree_end != NULL) {
    if (*last_local_tree > 0) {
      *child_in_tree_end = last_global_child - *last_local_tree * children_per_tree;
      t8_global_productionf("child_in_tree_end - if case: %li \n", *child_in_tree_end);
    }
    else {
      *child_in_tree_end = last_global_child;
      t8_global_productionf("child_in_tree_end - else case: %li \n", *child_in_tree_end);
    }
  }
  if (is_empty) {
    /* This process is empty */
    /* We now set the first local tree to the first local tree on the
     * next nonempty rank, and the last local tree to first - 1 */
    *first_local_tree = last_global_child / children_per_tree;
    if (first_global_child % children_per_tree != 0) {
      /* The next nonempty process shares this tree. */
      (*first_local_tree)++;
    }

    *last_local_tree = *first_local_tree - 1;
  }
}

void
t8_cmesh_uniform_bounds_2_5D (t8_cmesh_t cmesh, int level1, int level2, t8_scheme_comb_cxx_t *ts, t8_gloidx_t *first_local_tree,
                         t8_gloidx_t *child_in_tree_begin, t8_gloidx_t *last_local_tree, t8_gloidx_t *child_in_tree_end,
                         int8_t *first_tree_shared)
// t8_cmesh_uniform_bounds_2_5D (t8_cmesh_t cmesh, int level1, int level2, t8_scheme_comb_cxx_t *ts, t8_gloidx_t *first_local_tree1,
//                               t8_gloidx_t *first_local_tree2, t8_gloidx_t *child_in_tree_begin1, t8_gloidx_t *child_in_tree_begin2, t8_gloidx_t *last_local_tree1,
//                               t8_gloidx_t *last_local_tree2, t8_gloidx_t *child_in_tree_end1, t8_gloidx_t *child_in_tree_end2, int8_t *first_tree_shared)
{
  int is_empty;
  int trees_eclass1;

  T8_ASSERT (cmesh != NULL);
  T8_ASSERT (cmesh->committed);
  T8_ASSERT (level1 >= 0);
  T8_ASSERT (level2 >= 0);
  T8_ASSERT (ts != NULL);

  // for (int i = 0, trees_eclass1 = 0; i < t8_cmesh_get_num_trees (cmesh); i++)
  // {
  //   t8_eclass_t eclass_tree = t8_cmesh_get_tree_class (cmesh, i);
  //   t8_eclass_t eclass1 = ts->t8_element_get_eclass(1);
  //   if (eclass_tree == eclass1) {
  //     trees_eclass1++;
  //   }
  // }

  *first_local_tree = 0;
  if (child_in_tree_begin != NULL) {
    *child_in_tree_begin = 0;
  }
  *last_local_tree = 0;
  if (child_in_tree_end != NULL) {
    *child_in_tree_end = 0;
  }

  // *first_local_tree1 = 0;
  // //*first_local_tree2 = 0;
  // if (child_in_tree_begin1 != NULL) {
  //   *child_in_tree_begin1 = 0;
  // }
  // if (child_in_tree_begin2 != NULL) {
  //   *child_in_tree_begin2 = 0;
  // }
  // *last_local_tree1 = 0;
  // //*last_local_tree2 = 0;
  // if (child_in_tree_end1 != NULL) {
  //   *child_in_tree_end1 = 0;
  // }
  // if (child_in_tree_end2 != NULL) {
  //   *child_in_tree_end2 = 0;
  // }

  t8_gloidx_t global_num_children;
  t8_gloidx_t first_global_child;
  t8_gloidx_t child_in_tree_begin_temp;
  // t8_gloidx_t child_in_tree_begin_temp1;
  // t8_gloidx_t child_in_tree_begin_temp2;
  t8_gloidx_t last_global_child;
  //children_per_tree1 & 2 needed?
  t8_gloidx_t children_per_tree1 = 0;
  t8_gloidx_t children_per_tree2 = 0;
  t8_gloidx_t correct;
#ifdef T8_ENABLE_DEBUG
  t8_gloidx_t prev_last_tree = -1;
#endif
  int tree_class1;
  int tree_class2;
  t8_eclass_scheme_c *tree_scheme;

  /* Compute the number of children on level1 for eclass1 and level2 for eclass2 in each tree */
  global_num_children = 0;
  
  for (tree_class1 = T8_ECLASS_ZERO, trees_eclass1 = 0; tree_class1 < T8_ECLASS_COUNT; ++tree_class1) {
    for (tree_class2 = T8_ECLASS_ZERO; tree_class2 < T8_ECLASS_COUNT; ++tree_class2) {
    /* We iterate over each element class and get the number of children for this
     * tree class.
     */
      if (cmesh->num_trees_per_eclass[tree_class1] > 0 && cmesh->num_trees_per_eclass[tree_class2] > 0 
          && ts->eclass_schemes_comb[tree_class1][tree_class2] != NULL && tree_class1 != tree_class2 
          && (t8_eclass_t) tree_class2 == T8_ECLASS_LINE) {
        tree_scheme = ts->eclass_schemes_comb[tree_class1][tree_class2];
        t8_global_productionf (" tree_class1 %i \n", tree_class1);
        t8_global_productionf (" tree_class2 %i \n", tree_class2);
        t8_global_productionf (" t8_cmesh_get_num_trees (cmesh) %li \n", t8_cmesh_get_num_trees (cmesh));
        
        for (int i = 0; i < t8_cmesh_get_num_trees (cmesh); i++)
        {
          t8_eclass_t eclass_tree = t8_cmesh_get_tree_class (cmesh, i);
          t8_global_productionf (" eclass_tree: %i \n", eclass_tree);
          //t8_eclass_t eclass1 = tree_scheme->t8_element_get_eclass(1);
          // -> t8_element_get_eclass => don't need this function
          // if (eclass_tree == eclass1) {
           if (eclass_tree == tree_class1) {
            t8_global_productionf (" i: %i \n", i);
            trees_eclass1++;
          }
        }
        T8_ASSERT (tree_scheme != NULL);
        /* Children per tree for eclass1 */
        children_per_tree1 = tree_scheme->t8_element_count_leaves_from_root (level1, 1);
        T8_ASSERT (children_per_tree1 >= 0);
        t8_global_productionf (" children_per_tree1: %li \n", children_per_tree1);
        // global_num_children += cmesh->num_trees_per_eclass[tree_class1] * children_per_tree1;
        /* Children per tree for eclass2 */
        // t8_eclass_t eclass = (t8_eclass_t) tree_class2;
        children_per_tree2 = tree_scheme->t8_element_count_leaves_from_root (level2, 2);
        t8_global_productionf (" children_per_tree2: %li \n", children_per_tree2);
        T8_ASSERT (children_per_tree2 >= 0);
        // global_num_children += cmesh->num_trees_per_eclass[tree_class2] * children_per_tree2;
        global_num_children += cmesh->num_local_trees_per_eclass[tree_class1] * children_per_tree1 * children_per_tree2;
                              //* cmesh->num_local_trees_per_eclass[tree_class2] 
      }
      // else if ((t8_eclass_t) tree_class1 == T8_ECLASS_LINE && (t8_eclass_t) tree_class2 == T8_ECLASS_LINE)
      // {
      //   tree_scheme = ts->eclass_schemes_comb[tree_class1][tree_class2];
      //   T8_ASSERT (tree_scheme != NULL);
      //   /* Children per tree for eclass1 */
      //   children_per_tree1 = (tree_scheme->t8_element_count_leaves_from_root (level1, 1));
      //   T8_ASSERT (children_per_tree1 >= 0);
      //   // global_num_children += cmesh->num_trees_per_eclass[tree_class1]/2 * children_per_tree1;
      //   /* Children per tree for eclass2 */
      //   children_per_tree2 = (tree_scheme->t8_element_count_leaves_from_root (level2, 2));
      //   T8_ASSERT (children_per_tree2 >= 0);
      //   // global_num_children += cmesh->num_trees_per_eclass[tree_class2]/2 * children_per_tree2;
      //   global_num_children += (cmesh->num_local_trees_per_eclass[tree_class1]/2) * children_per_tree1 
      //                         * (cmesh->num_local_trees_per_eclass[tree_class2]/2) * children_per_tree2;
      //   t8_global_productionf (" CASE LINE & LINE. \n");
      // }
    }
  }
  t8_global_productionf (" trees_eclass1: %i \n", trees_eclass1);
  t8_global_productionf (" level1: %i \n", level1);
  // t8_global_productionf (" children_per_tree1: %li \n", children_per_tree1);
  t8_global_productionf (" level2: %i \n", level2);
  // t8_global_productionf (" children_per_tree2: %li \n", children_per_tree2);
  t8_global_productionf (" global_num_children: %li \n", global_num_children);
  T8_ASSERT (children_per_tree1 != 0);
  T8_ASSERT (children_per_tree2 != 0);

  if (cmesh->mpirank == 0) {
    first_global_child = 0;
    // if (child_in_tree_begin != NULL) {
    //   *child_in_tree_begin = 0;
    // }
    // if (child_in_tree_begin1 != NULL) {
    //   *child_in_tree_begin1 = 0;
    // }
    // if (child_in_tree_begin2 != NULL) {
    //   *child_in_tree_begin2 = 0;
    // }
    t8_global_productionf (" mpirank == 0 \n");
  }
  //TODO!!
  else {
    /* The first global child of processor p
     * with P total processor is (the biggest int smaller than)
     * (total_num_children * p) / P
     * We cast to long double and double first to prevent integer overflow.
     */
    first_global_child = ((long double) global_num_children * cmesh->mpirank) / (double) cmesh->mpisize;
    t8_global_productionf (" first_global_child: %li \n", first_global_child);
    /* Check if processes are large enough*/
    /* Columns (children_per_tree2) shall be on one process */
    T8_ASSERT (first_global_child < children_per_tree2);
    for (int i = 1; i < children_per_tree1; i++)
    {
      correct = first_global_child;
      if (correct > i * children_per_tree2)
      {
        first_global_child = i * children_per_tree2;
      }
    }  
  }
  t8_global_productionf (" cmesh->mpirank: %i \n", cmesh->mpirank);
  t8_global_productionf (" cmesh->mpisize: %i \n", cmesh->mpisize);
  if (cmesh->mpirank != cmesh->mpisize - 1) {
    last_global_child = ((long double) global_num_children * (cmesh->mpirank + 1)) / (double) cmesh->mpisize;
    for (int i = 1; i < children_per_tree1; i++)
    {
      correct = last_global_child;
      if (correct > i * children_per_tree2)
      {
        last_global_child = (i * children_per_tree2) - 1;
      }
    }  
  }
  else {
    last_global_child = global_num_children;
  }
  t8_global_productionf("last_global_child: %li", last_global_child);

  T8_ASSERT (0 <= first_global_child && first_global_child <= global_num_children);
  T8_ASSERT (0 <= last_global_child && last_global_child <= global_num_children);

  *first_local_tree = first_global_child / (children_per_tree1 * children_per_tree2);
  child_in_tree_begin_temp = first_global_child - *first_local_tree * (children_per_tree1 * children_per_tree2); //What happens here? Always 0 if children_per_tree != 0? @Chiara
  if (child_in_tree_begin != NULL) {
    *child_in_tree_begin = child_in_tree_begin_temp;
  }

  *last_local_tree = ((last_global_child - 1) / (children_per_tree1 * children_per_tree2)) * 2 + 1; 
  // *last_local_tree = (((last_global_child/children_per_tree1) - 1) / (children_per_tree2));
  // *last_local_tree = (last_global_child - 1) / (children_per_tree2 * (children_per_tree1/trees_eclass1));
  // *last_local_tree = ((last_global_child - 1) / (children_per_tree1 * children_per_tree2)) * trees_eclass1 - 1; // oder / (children_per_tree1 * children_per_tree2)
  //*last_local_tree = (last_global_child - 1) / children_per_tree2 - 1;
  t8_global_productionf("children_per_tree1: %li", children_per_tree1);
  t8_global_productionf("children_per_tree2: %li", children_per_tree2);

  is_empty = *first_local_tree >= *last_local_tree && first_global_child >= last_global_child;
  if (first_tree_shared != NULL) {
#ifdef T8_ENABLE_DEBUG
    prev_last_tree = (first_global_child - 1) / (children_per_tree1 * children_per_tree1);
    T8_ASSERT (cmesh->mpirank > 0 || prev_last_tree <= 0);
#endif
  if (!is_empty && cmesh->mpirank > 0 && child_in_tree_begin_temp > 0) {
      /* We exclude empty partitions here, by def their first_tree_shared flag is zero */
      /* We also exclude that the previous partition was empty at the beginning of the
       * partitions array */
      /* We also exclude the case that we have the first global element but
       * are not rank 0. */
      *first_tree_shared = 1;
    }
    else {
      *first_tree_shared = 0;
    }
  }
  t8_global_productionf("last_local_tree: %li", *last_local_tree);
  t8_global_productionf("(child_in_tree_end: %li", *child_in_tree_end);
  if (child_in_tree_end != NULL) {
    if (*last_local_tree > 1) {
      //PASST DAS SO??
      *child_in_tree_end = last_global_child - ((*last_local_tree - 1)/2) * (children_per_tree2 * children_per_tree1); // *children_per_tree1
      t8_global_productionf("if case");
   }
    else {
      *child_in_tree_end = last_global_child;
      t8_global_productionf("else case");
    }
  }
  if (is_empty) {
    /* This process is empty */
    /* We now set the first local tree to the first local tree on the
     * next nonempty rank, and the last local tree to first - 1 */
    *first_local_tree = last_global_child / (children_per_tree1 * children_per_tree2); //??
    if (first_global_child % (children_per_tree1 * children_per_tree2) != 0) {
      /* The next nonempty process shares this tree. */
      (*first_local_tree)++;
    }

    *last_local_tree = *first_local_tree - 1;
  }
}
//   /* interested in trees for eclass1 -> ???every second tree in cmesh*/
//   // *first_local_tree = (first_global_child / children_per_tree2) * 2;
//   *first_local_tree1 = first_global_child / (children_per_tree1 * children_per_tree2);
//   (*first_local_tree1 % 2 == 0) ? *first_local_tree1 : *first_local_tree1++;
//   // t8_global_productionf (" first_local_tree1: %li \n", *first_local_tree1);

//   *first_local_tree2 = *first_local_tree1 + 1; 
//   // t8_global_productionf (" first_local_tree2: %li \n", *first_local_tree2);

  // // Why do we need this?
  // child_in_tree_begin_temp1 = first_global_child - *first_local_tree1 * children_per_tree1;// * children_per_tree2; //???
  // if (child_in_tree_begin1 != NULL) {
  //   *child_in_tree_begin1 = child_in_tree_begin_temp1;
  // }

  // child_in_tree_begin_temp2 = first_global_child - (*first_local_tree2 - 1) * children_per_tree2;// * children_per_tree2; //???
  // if (child_in_tree_begin2 != NULL) {
  //   *child_in_tree_begin2 = child_in_tree_begin_temp2;
  // }

//   /* interested in trees for eclass1 -> every second tree in cmesh*/
//   *last_local_tree1 = last_global_child/children_per_tree2 - 1;
//   //(*last_local_tree1 % 2 == 0) ? *last_local_tree1 : *last_local_tree1++;
//   t8_global_productionf (" last_local_tree1: %li \n", *last_local_tree1);

//   /* interested in trees for eclass2 -> children_per_tree1 many after *last_local_tree1 //-> every second tree in cmesh*/
//   *last_local_tree2 = *last_local_tree1 + 1;
//   t8_global_productionf (" last_local_tree2: %li \n", *last_local_tree2);

//   is_empty = *first_local_tree1 >= *last_local_tree1 && *first_local_tree2 >= *last_local_tree2 && first_global_child >= last_global_child;
//   if (first_tree_shared != NULL) {
// #ifdef T8_ENABLE_DEBUG
//     prev_last_tree = (first_global_child - 1) / (children_per_tree1 * children_per_tree2); //??
//     T8_ASSERT (cmesh->mpirank > 0 || prev_last_tree <= 0);
// #endif
//     if (!is_empty && cmesh->mpirank > 0 && child_in_tree_begin_temp1 > 0 && child_in_tree_begin_temp2 > 0) {
//       /* We exclude empty partitions here, by def their first_tree_shared flag is zero */
//       /* We also exclude that the previous partition was empty at the beginning of the
//        * partitions array */
//       /* We also exclude the case that we have the first global element but
//        * are not rank 0. */
//       *first_tree_shared = 1;
//     }
//     else {
//       *first_tree_shared = 0;
//     }
//   }

//   if (child_in_tree_end1 != NULL) {
//     if (*last_local_tree1 > 0) {
//       *child_in_tree_end1 = last_global_child - *last_local_tree1 * children_per_tree1;// * children_per_tree2; //????
//     }
//     else {
//       *child_in_tree_end1 = last_global_child;
//     }
//   }

//   if (child_in_tree_end2 != NULL) {
//     if (*last_local_tree1 > 0) {
//       *child_in_tree_end2 = last_global_child - *last_local_tree1 * children_per_tree1;// * children_per_tree2; //????
//     }
//     else {
//       *child_in_tree_end2 = last_global_child;
//     }
//   }
//   if (is_empty) {
//     /* This process is empty */
//     /* We now set the first local tree to the first local tree on the
//      * next nonempty rank, and the last local tree to first - 1 */
//     *first_local_tree1 = (last_global_child / (children_per_tree1 * children_per_tree2));
//     *first_local_tree2 = *first_local_tree1 + 1;
//     if (first_global_child % children_per_tree2 != 0) {
//       /* The next nonempty process shares this tree (regarding eclass1). */
//       (*first_local_tree1)++;
//       /* The next nonempty process shares this tree (regarding eclass1). */
//       (*first_local_tree2)++;
//     }

//     *last_local_tree1 = *first_local_tree1 - 1;
//     *last_local_tree2 = *first_local_tree2 - 1;
//   }
// }
