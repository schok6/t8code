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

#include "t8_2_5dimension_element_cxx.hxx"
#include <t8_schemes/t8_default/t8_default_line/t8_dline.h>
#include <t8_schemes/t8_default/t8_default_quad/t8_dquad.h>
#include <t8_schemes/t8_default/t8_default_tri/t8_dtri.h>
//#include <t8_schemes/t8_2_5dimension/t8_2_5D.hxx>

// /* This function is used by other element functions and we thus need to
//  * declare it up here */
// t8_linearidx_t
// t8_element_get_linear_id (const t8_element_t *elem, int level, int dir);

t8_2_5dimension_scheme_c::t8_2_5dimension_scheme_c (t8_eclass_scheme_c *scheme1, t8_eclass_scheme_c *scheme2)
{
  this->scheme1 = scheme1;
  this->scheme2 = scheme1;
  t8_eclass_t eclass1 = scheme1->eclass;
  t8_eclass_t eclass2 = scheme2->eclass;
  t8_global_productionf ("eclass1:\t%i\n", eclass1);
  t8_global_productionf ("eclass2:\t%i\n", eclass2);
  //for testing
  if (eclass1 == T8_ECLASS_LINE && eclass2 == T8_ECLASS_LINE) {
    //if (eclass2 == T8_ECLASS_LINE && (eclass1 == T8_ECLASS_QUAD || eclass1 == T8_ECLASS_TRIANGLE)){
    //if ((eclass1 == T8_ECLASS_LINE && (eclass2 == T8_ECLASS_QUAD || eclass2 == T8_ECLASS_TRIANGLE)) //leave this out for now
    //|| (eclass2 == T8_ECLASS_LINE && (eclass1 == T8_ECLASS_QUAD || eclass1 == T8_ECLASS_TRIANGLE)))) {
    element_size = sizeof (scheme1->t8_element_size ()) * sizeof (scheme2->t8_element_size ());
    ts_context = sc_mempool_new (element_size);
    return;
  }
  /* TODO:
  * Generalize the approach that also eclass1 can be line 
  */
  else if (eclass1 == T8_ECLASS_LINE && (eclass2 == T8_ECLASS_QUAD || eclass2 == T8_ECLASS_TRIANGLE)) {
    SC_ABORT ("For now consider depending refinement in vertical direction (z-dimension).\n");
  }
  SC_ABORT ("3 dimensions are needed in order to apply the scheme for 2_5 dimension.\n");
}

t8_2_5dimension_scheme_c::~t8_2_5dimension_scheme_c ()
{
  T8_ASSERT (ts_context != NULL);
  sc_mempool_destroy ((sc_mempool_t *) ts_context);
}

int
t8_2_5dimension_scheme_c::t8_element_refines_irregular (void) const
{
  return 0;
}

int
t8_2_5dimension_scheme_c::t8_element_maxlevel (void) const
{
  //langt ein maxlevel?
  return 3; //for testing
  //return SC_MIN (scheme1->t8_element_maxlevel (), scheme2->t8_element_maxlevel ());
}

int
t8_2_5dimension_scheme_c::t8_element_level (const t8_element_t *elem, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme1->t8_element_level (el->elem1);
  }
  else if (dir == 2) {
    return scheme2->t8_element_level (el->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme_c::t8_element_copy (const t8_element_t *source, t8_element_t *dest) const
{
  const t8_2_5D_t *s = (const t8_2_5D_t *) source;
  t8_2_5D_t *d = (t8_2_5D_t *) dest;
  scheme1->t8_element_copy (s->elem1, d->elem1);
  scheme2->t8_element_copy (s->elem2, d->elem2);
}

int
t8_2_5dimension_scheme_c::t8_element_compare (const t8_element_t *elem1, const t8_element_t *elem2) const
{
  const t8_2_5D_t *el1 = (const t8_2_5D_t *) elem1;
  const t8_2_5D_t *el2 = (const t8_2_5D_t *) elem2;
  return scheme1->t8_element_compare (el1->elem1, el2->elem1) && scheme2->t8_element_compare (el1->elem2, el2->elem2);
}

int
t8_2_5dimension_scheme_c::t8_element_equal (const t8_element_t *elem1, const t8_element_t *elem2) const
{
  const t8_2_5D_t *el1 = (const t8_2_5D_t *) elem1;
  const t8_2_5D_t *el2 = (const t8_2_5D_t *) elem2;
  return scheme1->t8_element_equal (el1->elem1, el2->elem1) && scheme2->t8_element_equal (el1->elem2, el2->elem2);
}

void
t8_2_5dimension_scheme_c::t8_element_parent (const t8_element_t *elem, t8_element_t *parent, int dir) const
{
  //DISKUSSION
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;  
  t8_2_5D_t *parent2_5D = (t8_2_5D_t *) parent;
  /* return parent of eclass1*/
  if (dir == 1) {
    /*
    * SC_ABORT ("A list of all elements children from eclass1 with the according refinement of eclass2 needs\
    * to be considered. / Needed: Siblings to go through them in z-direction.\n");
    * see: t8_2_5dimension_scheme_c::t8_2_5D_parent (const t8_element_t *elem, t8_element_t *parent, t8_element_t **listofelem) const
    */

    scheme1->t8_element_parent(el->elem1, parent2_5D->elem1);
    scheme2->t8_element_copy (el->elem2, parent2_5D->elem2);

    //über bit informationen an parent kommen -> nicht möglich, weil nicht eindeutig definiert
    //=>direktes parent in Richtung eclass1 und dann über forest Suchfunktionalität schauen, ob Element existiert
    //wenn nicht, dann iterativ parent in Richtung eclass2 erstellen und wieder schauen

  }
  /* return parent of eclass2*/
  else if (dir == 2) {
    /* For now it's required to have a line in z-coordinates/ eclass2 == T8_ECLASS_LINE*/  
    scheme1->t8_element_copy(el->elem1, parent2_5D->elem1);
    scheme2->t8_element_parent (el->elem2, parent2_5D->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

// //NOT NEEDED
// int32_t
// t8_2_5dimension_scheme_c::t8_element_get_coord (const t8_element_t *elem, int dir_coord) const
// //t8_2_5dimension_scheme_c::t8_element_get_coord (const t8_element_t *elem, t8_element_t *elem_new, int dir_scheme, int dir_coord) const
// {
//   if (dir_coord == 1) {
//     t8_eclass_t eclass = scheme1->eclass;
//     switch (eclass) {
//       case T8_ECLASS_QUAD:
//       {
//         t8_dquad_t *el = (t8_dquad_t *) elem;
//         return el->x;
//       }
//       case T8_ECLASS_TRIANGLE:
//       {
//         t8_dtri_t *el = (t8_dtri_t *) elem;
//         return el->x;
//       }
//       case T8_ECLASS_LINE:
//       {
//         t8_dline_t *el = (t8_dline_t *) elem;
//         return el->x;
//       }
//       default:
//         SC_ABORT ("Only schemes with eclass QUAD or TRIANGLE are allowed for scheme1 so far.\n");
//         break;
//     }
//   }
//   else if (dir_coord == 2) {
//     t8_eclass_t eclass = scheme2->eclass;
//     switch (eclass) {
//       case T8_ECLASS_QUAD:
//       {
//         t8_dquad_t *el = (t8_dquad_t *) elem;
//         return el->y;
//       }
//       case T8_ECLASS_TRIANGLE:
//       {
//         t8_dtri_t *el = (t8_dtri_t *) elem;
//         return el->y;
//       }
//       default:
//         SC_ABORT ("Only schemes with eclass QUAD or TRIANGLE are allowed for scheme1 so far.\n");
//         break;
//     }
//   }
//   else {
//     SC_ABORT ("Direction parameter is missing. For 2_5D only t8_eclass_scheme with at most 2D is needed.\n");
//   }
// }


int8_t
t8_2_5dimension_scheme_c::t8_element_get_level (const t8_element_t *elem, int dir) const
{
  if (dir == 1) {
    t8_eclass_t eclass = scheme1->eclass;
    switch (eclass) {
    case T8_ECLASS_LINE: {
      const t8_dline_t *elem_line = (const t8_dline_t *) elem;
      return elem_line->level;
      break;
    }
    case T8_ECLASS_QUAD: {
      const t8_dquad_t *elem_quad = (const t8_dquad_t *) elem;
      return elem_quad->level;
      break;
    }
    case T8_ECLASS_TRIANGLE: {
      const t8_dtri_t *elem_tri = (const t8_dtri_t *) elem;
      return elem_tri->level;
      break;
    }
    default:
      SC_ABORT ("Only schemes with eclass LINE, QUAD or TRIANGLE are allowed for 2.5D.\n");
      break;
    }
  }
  else if (dir == 2) {
    t8_eclass_t eclass = scheme2->eclass;
    switch (eclass) {
    case T8_ECLASS_LINE: {
      const t8_dline_t *elem_line = (const t8_dline_t *) elem;
      return elem_line->level;
      break;
    }
    case T8_ECLASS_QUAD: {
      const t8_dquad_t *elem_quad = (const t8_dquad_t *) elem;
      return elem_quad->level;
      break;
    }
    case T8_ECLASS_TRIANGLE: {
      const t8_dtri_t *elem_tri = (const t8_dtri_t *) elem;
      return elem_tri->level;
      break;
    }
    default:
      SC_ABORT ("Only schemes with eclass LINE, QUAD or TRIANGLE are allowed for 2.5D.\n");
      break;
    }
  }
  else {
    SC_ABORT ("Direction parameter is missing. For 2_5D only t8_eclass_scheme with at most 2D is needed.\n");
  }
}

int
t8_2_5dimension_scheme_c::t8_element_num_siblings (const t8_element_t *elem, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme1->t8_element_num_siblings (el->elem1);
  }
  else if (dir == 2) {
    return scheme2->t8_element_num_siblings (el->elem2);
  }
  else if (dir == 3) {
    //SC_ABORT ("The function t8_element_num_siblings is indefined for eclass1 (has a value between num_siblings_scheme1 and (num_siblings_scheme2 ^ maxlevel2).\n");
    //not uniquely defined! this is the maximum number of possible sibling
    int poss_level = scheme2->t8_element_maxlevel () - t8_element_get_level(el->elem2, 2) - 1;
    return (t8_element_num_siblings (elem, 1) - 1) * sc_intpow (t8_element_num_children (elem, 2), poss_level) + 1;
    //scheme2->t8_element_num_siblings (el->elem2) == t8_element_num_siblings (elem, 2)
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme_c::t8_element_sibling (const t8_element_t *elem, int sibid, t8_element_t *sibling, int dir) const
{
  T8_ASSERT (t8_element_is_valid (elem));
  T8_ASSERT (t8_element_is_valid (sibling));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t *sib = (t8_2_5D_t *) sibling;
  /*direct sibling in eclass1 */
  if (dir == 1) {
    scheme1->t8_element_sibling(el->elem1, sibid, sib->elem1);
    scheme2->t8_element_copy (el->elem2, sib->elem2);
    /*TODO
    * iterativ auch weitere Elemente der Säule in Siblings bezüglich eclass1 verfeinern
    */
  } 
  else if (dir == 2) {
    scheme1->t8_element_copy (el->elem1, sib->elem1);
    scheme2->t8_element_sibling (el->elem2, sibid, sib->elem2);
    //return scheme2->t8_element_sibling (el->elem2, sibid, sib->elem2); ERROR hier erzeugem #TODO
  }
  /*EXTRA CASE
  * sibid possible between 0 and t8_element_num_siblings(elem, 1) (=max number of possible siblings) 
  * SC_ABORT ("The function t8_element_sibling is not uniquely defined for eclass1.\n");
  */
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme_c::t8_element_num_corners (const t8_element_t *elem) const
{
  T8_ASSERT (t8_element_is_valid (elem));
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  return scheme1->t8_element_num_corners (el->elem1) * scheme2->t8_element_num_corners (el->elem2);
}

int
t8_2_5dimension_scheme_c::t8_element_num_faces (const t8_element_t *elem) const
{
  // T8_ASSERT (t8_element_is_valid (elem));
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO; /* suppresses compiler warning */
}

int
t8_2_5dimension_scheme_c::t8_element_max_num_faces (const t8_element_t *elem) const
{
  // T8_ASSERT (t8_element_is_valid (elem));
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme_c::t8_element_num_children (const t8_element_t *elem, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme1->t8_element_num_children (el->elem1);
  }
  else if (dir == 2) {
    return scheme2->t8_element_num_children (el->elem2);
  }
  else if (dir == 3) {
    //not uniquely defined! this is the maximum number of possible children
    int poss_level = scheme2->t8_element_maxlevel () - t8_element_get_level(el->elem2, 2);
    return ((t8_element_num_children (elem, 1) - 1) * sc_intpow (t8_element_num_children (elem, 2), poss_level)) + 1;
    //scheme1->t8_element_num_children (el->elem1) = t8_element_num_children (elem, 1)
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme_c::t8_element_num_face_children (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
  // T8_ASSERT (t8_element_is_valid (elem));
  // // Not true anymore for 4D with pyramids as faces
  // if constexpr (eclass_T == T8_ECLASS_VERTEX) {
  //   return 0;
  // }
  // else {
  //   return 1 << (T8_ELEMENT_DIM[eclass_T] - 1);
  // }
}

int
t8_2_5dimension_scheme_c::t8_element_get_face_corner (const t8_element_t *element, int face, int corner) const
{
  SC_ABORT ("This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme_c::t8_element_get_corner_face (const t8_element_t *element, int corner, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

void
t8_2_5dimension_scheme_c::t8_element_child (const t8_element_t *elem, int childid, t8_element_t *child, int dir) const
{
  T8_ASSERT (t8_element_is_valid (elem));
  T8_ASSERT (t8_element_is_valid (child));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t *c = (t8_2_5D_t *) child;
  /*direct child in eclass1 */
  if (dir == 1) {
    scheme1->t8_element_child(el->elem1, childid, c->elem1);
    scheme2->t8_element_copy (el->elem2, c->elem2);
    /*TODO
    * iterativ auch weitere Elemente der Säule in Kinder bezüglich eclass1 verfeinern
    */
  }
  else if (dir == 2) {
    scheme1->t8_element_copy(el->elem1, c->elem1);
    scheme2->t8_element_child (el->elem2, childid, c->elem2);
    /*TODO
    * iterativ auch weitere Elemente der Säule in Kinder bezüglich eclass1 verfeinern
    */
  }
  /*EXTRA CASE
  * childid possible between 0 and t8_element_num_children(elem, 1) (=max number of possible children) 
  * SC_ABORT ("The function t8_element_child is not uniquely defined for eclass1.\n");
  */
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme_c::t8_element_children (const t8_element_t *elem, int length, t8_element_t *c[], int dir) const
{
  SC_ABORT ("Next.\n");
  // //*c[] = *[t8_element_t, t8_element_t, t8_element_t, ...]
  // const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  // t8_element_t ***children = (t8_element_t ***) c;
  // //t8_2_5D_t ***children = T8_ALLOC(t8_2_5D_t **, length);
  // if (dir == 1) {
  //   //scheme1->t8_element_children (el->elem1, length, c); +++
  //   scheme1->t8_element_children (el->elem1, length, children[0]); //---
  //   for (int i = 0; i < length; i++)
  //   {
  //     //scheme1->t8_element_copy(c[i], children->elem1[i]); +++
  //     scheme2->t8_element_copy(el->elem2, children[1][i]);
  //   }
  // }
  // else if (dir == 2) {
  //   //scheme2->t8_element_children (el->elem2, length, c); +++
  //   scheme2->t8_element_children (el->elem2, length, children[1]); //---
  //   for (int i = 0; i < length; i++)
  //   {
  //     scheme1->t8_element_copy(el->elem1, children->[0][i]);
  //     //scheme2->t8_element_copy(c[i], children[1][i]); +++
  //   }
  // }
  // else {
  //   SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  // }
}

int
t8_2_5dimension_scheme_c::t8_element_child_id (const t8_element_t *elem, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme1->t8_element_child_id (el->elem1);
  }
  else if (dir == 2) {
    return scheme2->t8_element_child_id (el->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme_c::t8_element_ancestor_id (const t8_element_t *elem, int level, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme1->t8_element_ancestor_id (el->elem1, level);
  }
  else if (dir == 2) {
    return scheme2->t8_element_ancestor_id (el->elem2, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme_c::t8_element_is_family (t8_element_t *const *fam) const
{
  SC_ABORT("Next.");
//   //direction parameter?
//   t8_element_t *const **f = (t8_element_t *const **) fam;
//   //const t8_2_5D_t ***f = (const t8_2_5D_t ***) fam;
//   if (scheme1->t8_element_is_family (f->elem1) == 1 && scheme2->t8_element_is_family (fam) == 1) {
//     SC_ABORT ("Not possible to be a family in both given eclasses.\n");
//   }
//   return scheme1->t8_element_is_family (f->elem1) || scheme2->t8_element_is_family (fam);
}

void
t8_2_5dimension_scheme_c::t8_element_nca (const t8_element_t *elem1, const t8_element_t *elem2, t8_element_t *nca,
                                          int dir) const
{
  if (dir == 1) {
    return scheme1->t8_element_nca (elem1, elem2, nca);
  }
  else if (dir == 2) {
    return scheme2->t8_element_nca (elem1, elem2, nca);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

t8_element_shape_t
t8_2_5dimension_scheme_c::t8_element_face_shape (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

void
t8_2_5dimension_scheme_c::t8_element_children_at_face (const t8_element_t *elem, int face, t8_element_t *children[],
                                                       int num_children, int *child_indices) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  // t8_sele_children_at_face<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem, face,
  //                                     (t8_standalone_element_t<eclass_T> **) children, num_children, child_indices);
}

int
t8_2_5dimension_scheme_c::t8_element_face_child_face (const t8_element_t *elem, int face, int face_child, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. Waiting for scheme interface.\n");
  return T8_ECLASS_ZERO;
  // return t8_sele_face_child_face<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem, face, face_child);
}

int
t8_2_5dimension_scheme_c::t8_element_face_parent_face (const t8_element_t *elem, int face, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. Waiting for scheme interface.\n");
  return T8_ECLASS_ZERO;
  // return t8_sele_face_parent_face<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem, face);
}

int
t8_2_5dimension_scheme_c::t8_element_tree_face (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
  // return t8_sele_tree_face ((const t8_standalone_element_t<eclass_T> *) elem, face);
}

void
t8_2_5dimension_scheme_c::t8_element_transform_face (const t8_element_t *elem1, t8_element_t *elem2, int orientation,
                                                     int sign, int is_smaller_face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  // t8_sele_transform_face<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem1,
  //                                   (t8_standalone_element_t<eclass_T> *) elem2, orientation, sign, is_smaller_face);
}

int
t8_2_5dimension_scheme_c::t8_element_extrude_face (const t8_element_t *face, const t8_eclass_scheme_c *face_scheme,
                                                   t8_element_t *elem, int root_face, int dir) const
{
  SC_ABORT ("[FACE] Waiting for scheme interface.\n");
  return T8_ECLASS_ZERO;
  //  T8_ASSERT(t8_eclass_scheme is correct)
  // t8_eclass_t face_eclass = T8_ECLASS_ZERO;
  // if constexpr (eclass_T == T8_ECLASS_VERTEX)
  //   SC_ABORT_NOT_REACHED ();
  // if constexpr (eclass_T == T8_ECLASS_LINE)
  //   face_eclass = T8_ECLASS_VERTEX;
  // if constexpr (eclass_T == T8_ECLASS_QUAD)
  //   face_eclass = T8_ECLASS_LINE;
  // if constexpr (eclass_T == T8_ECLASS_HEX)
  //   face_eclass = T8_ECLASS_QUAD;
  // if constexpr (T8_ELEMENT_NUM_EQUATIONS[eclass_T]) {
  //   face_eclass = t8_sele_lut_rootface_to_eclass<eclass_T>[root_face];
  // }
  // if (face_eclass == T8_ECLASS_VERTEX) {
  //   return t8_sele_extrude_face<eclass_T, T8_ECLASS_VERTEX> ((const t8_standalone_element_t<T8_ECLASS_VERTEX> *) face,
  //                                                            (t8_standalone_element_t<eclass_T> *) elem, root_face);
  // }
  // if (face_eclass == T8_ECLASS_LINE) {
  //   return t8_sele_extrude_face<eclass_T, T8_ECLASS_LINE> ((const t8_standalone_element_t<T8_ECLASS_LINE> *) face,
  //                                                          (t8_standalone_element_t<eclass_T> *) elem, root_face);
  // }
  // if (face_eclass == T8_ECLASS_TRIANGLE) {
  //   return t8_sele_extrude_face<eclass_T, T8_ECLASS_TRIANGLE> (
  //     (const t8_standalone_element_t<T8_ECLASS_TRIANGLE> *) face, (t8_standalone_element_t<eclass_T> *) elem,
  //     root_face);
  // }
  // if (face_eclass == T8_ECLASS_QUAD) {
  //   return t8_sele_extrude_face<eclass_T, T8_ECLASS_QUAD> ((const t8_standalone_element_t<T8_ECLASS_QUAD> *) face,
  //                                                          (t8_standalone_element_t<eclass_T> *) elem, root_face);
  // }
  // return 0;
}

void
t8_2_5dimension_scheme_c::t8_element_boundary_face (const t8_element_t *elem, int face, t8_element_t *boundary,
                                                    const t8_eclass_scheme_c *boundary_scheme) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  // //  T8_ASSERT(t8_eclass_scheme is correct)
  // int root_face = t8_element_tree_face (elem, face);
  // t8_eclass_t face_eclass = T8_ECLASS_ZERO;
  // if constexpr (eclass_T == T8_ECLASS_VERTEX)
  //   SC_ABORT_NOT_REACHED ();
  // if constexpr (eclass_T == T8_ECLASS_LINE)
  //   face_eclass = T8_ECLASS_VERTEX;
  // if constexpr (eclass_T == T8_ECLASS_QUAD)
  //   face_eclass = T8_ECLASS_LINE;
  // if constexpr (eclass_T == T8_ECLASS_HEX)
  //   face_eclass = T8_ECLASS_QUAD;
  // if constexpr (T8_ELEMENT_NUM_EQUATIONS[eclass_T]) {
  //   face_eclass = t8_sele_lut_rootface_to_eclass<eclass_T>[root_face];
  // }
  // if (face_eclass == T8_ECLASS_VERTEX) {
  //   t8_debugf ("call t8_sele_boundary_face with face_eclass = T8_ECLASS_VERTEX\n");
  //   t8_sele_boundary_face<eclass_T, T8_ECLASS_VERTEX> ((const t8_standalone_element_t<eclass_T> *) elem, root_face,
  //                                                      (t8_standalone_element_t<T8_ECLASS_VERTEX> *) boundary);
  // }
  // if (face_eclass == T8_ECLASS_LINE) {
  //   t8_debugf ("call t8_sele_boundary_face with face_eclass = T8_ECLASS_LINE\n");
  //   t8_sele_boundary_face<eclass_T, T8_ECLASS_LINE> ((const t8_standalone_element_t<eclass_T> *) elem, root_face,
  //                                                    (t8_standalone_element_t<T8_ECLASS_LINE> *) boundary);
  // }
  // if (face_eclass == T8_ECLASS_TRIANGLE) {
  //   t8_debugf ("call t8_sele_boundary_face with face_eclass = T8_ECLASS_TRIANGLE\n");
  //   t8_sele_boundary_face<eclass_T, T8_ECLASS_TRIANGLE> ((const t8_standalone_element_t<eclass_T> *) elem, root_face,
  //                                                        (t8_standalone_element_t<T8_ECLASS_TRIANGLE> *) boundary);
  // }
  // if (face_eclass == T8_ECLASS_QUAD) {
  //   t8_debugf ("call t8_sele_boundary_face with face_eclass = T8_ECLASS_QUAD\n");
  //   t8_sele_boundary_face<eclass_T, T8_ECLASS_QUAD> ((const t8_standalone_element_t<eclass_T> *) elem, root_face,
  //                                                    (t8_standalone_element_t<T8_ECLASS_QUAD> *) boundary);
  // }
}

void
t8_2_5dimension_scheme_c::t8_element_first_descendant_face (const t8_element_t *elem, int face,
                                                            t8_element_t *first_desc, int level, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. & Waiting for scheme interface.\n");
  // t8_sele_first_descendant_face ((const t8_standalone_element_t<eclass_T> *) elem, face,
  //                                (t8_standalone_element_t<eclass_T> *) first_desc, level);
}

void
t8_2_5dimension_scheme_c::t8_element_last_descendant_face (const t8_element_t *elem, int face, t8_element_t *last_desc,
                                                           int level, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. & Waiting for scheme interface.\n");
  // t8_sele_last_descendant_face ((const t8_standalone_element_t<eclass_T> *) elem, face,
  //                               (t8_standalone_element_t<eclass_T> *) last_desc, level);
}

void
t8_2_5dimension_scheme_c::t8_element_boundary (const t8_element_t *elem, int min_dim, int length,
                                               t8_element_t **boundary) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
}

int
t8_2_5dimension_scheme_c::t8_element_is_root_boundary (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
  //   return t8_sele_is_root_boundary<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem, face);
}

int
t8_2_5dimension_scheme_c::t8_element_face_neighbor_inside (const t8_element_t *elem, t8_element_t *neigh, int face,
                                                           int *neigh_face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
  // return t8_sele_face_neighbor<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem,
  //                                         (t8_standalone_element_t<eclass_T> *) neigh, face, neigh_face);
}

t8_element_shape_t
t8_2_5dimension_scheme_c::t8_element_shape (const t8_element_t *elem) const
{
  SC_ABORT ("[NOT NEEDED] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
  //   if (dir == 1)
  //   {
  //     return scheme1->t8_element_shape(elem);
  //   }
  //   else if (dir == 2)
  //   {
  //     return scheme2->t8_element_shape(const t8_element_t *elem);
  //   }
  //   else{
  //     SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  //   }
  // }
}

void
t8_2_5dimension_scheme_c::t8_element_set_linear_id (t8_element_t *elem, int level, t8_linearidx_t id, int dir) const
{
  if (dir == 1) {
    return scheme1->t8_element_set_linear_id (elem, level, id);
  }
  else if (dir == 2) {
    return scheme2->t8_element_set_linear_id (elem, level, id);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

t8_linearidx_t
t8_2_5dimension_scheme_c::t8_element_get_linear_id (const t8_element_t *elem, int level, int dir) const
{
  if (dir == 1) {
    return scheme1->t8_element_get_linear_id (elem, level);
  }
  else if (dir == 2) {
    return scheme2->t8_element_get_linear_id (elem, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme_c::t8_2_5D_get_linear_id (const t8_element_t *elem1, const t8_element_t *elem2, int level1, int level2) const
{
  return t8_element_get_linear_id (elem1, level1, 1) * sc_intpow (t8_element_num_children (elem2, 2), level2)
         + t8_element_get_linear_id (elem2, level2, 2);
}

void
t8_2_5dimension_scheme_c::t8_element_first_descendant (const t8_element_t *elem, t8_element_t *desc, int level,
                                                       int dir) const
{
  if (dir == 1) {
    scheme1->t8_element_first_descendant (elem, desc, level);
  }
  else if (dir == 2) {
    scheme2->t8_element_first_descendant (elem, desc, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme_c::t8_element_last_descendant (const t8_element_t *elem, t8_element_t *desc, int level,
                                                      int dir) const
{
  if (dir == 1) {
    scheme1->t8_element_last_descendant (elem, desc, level);
  }
  else if (dir == 2) {
    scheme2->t8_element_last_descendant (elem, desc, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme_c::t8_element_successor (const t8_element_t *t, t8_element_t *s, int dir) const
{
  if (dir == 1) {
    scheme1->t8_element_successor (t, s);
  }
  else if (dir == 2) {
    scheme2->t8_element_successor (t, s);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme_c::t8_element_vertex_reference_coords (const t8_element_t *t, const int vertex,
                                                              double coords[]) const
{
  SC_ABORT ("Next.\n");
  //   T8_ASSERT (t8_element_is_valid (t));
  //   t8_sele_vertex_reference_coords<eclass_T> ((const t8_standalone_element_t<eclass_T> *) t, vertex, coords);
}

void
t8_2_5dimension_scheme_c::t8_element_reference_coords (const t8_element_t *elem, const double *ref_coords,
                                                       const size_t num_coords, double *out_coords) const
{
  SC_ABORT ("This function is not implemented yet.\n");
}

t8_gloidx_t
t8_2_5dimension_scheme_c::t8_element_count_leaves (const t8_element_t *elem, int level, int dir) const
{
  SC_ABORT ("Waiting for scheme interface.\n");
  //return 100000000;
  //return;
  //   return t8_sele_num_descendants_at_leveldiff<eclass_T> ((const t8_standalone_element_t<eclass_T> *) elem,
  //                                                          level - t8_element_level (elem));
}

t8_gloidx_t
t8_2_5dimension_scheme_c::t8_element_count_leaves_from_root (int level, int dir) const
{
  SC_ABORT ("Waiting for scheme interface.\n");
  //return 100000000;
  // T8_ASSERT (level >= 0);
  // // t8_linearidx_t two_to_l = 1LL << level;
  // // t8_linearidx_t eight_to_l = 1LL << (3 * level);
  // // if constexpr ((eclass_T1 == T8_ECLASS_PYRAMID) && (eclass_T2 == T8_ECLASS_PYRAMID)){
  // //   t8_linearidx_t leafs_from_root[] = {((eight_to_l << 2) - two_to_l) / 3, ((eight_to_l << 2) - two_to_l) / 3};
  // //   return *leafs_from_root;
  // // }
  // // else if constexpr ((eclass_T1 == T8_ECLASS_PYRAMID) && (eclass_T2 != T8_ECLASS_PYRAMID)){
  // //   t8_gloidx_t leafs_from_root[] = {((eight_to_l << 2) - two_to_l) / 3, 1LL << (level * T8_ELEMENT_DIM[eclass_T2])};
  // //   return *leafs_from_root;
  // // }
  // // else if constexpr ((eclass_T1 != T8_ECLASS_PYRAMID) && (eclass_T2 == T8_ECLASS_PYRAMID)){
  // //   t8_gloidx_t leafs_from_root[] = {1LL << (level * T8_ELEMENT_DIM[eclass_T1]), ((eight_to_l << 2) - two_to_l) / 3};
  // //   return *leafs_from_root;
  // // }
  // t8_gloidx_t leafs_from_root[] = {1LL << (level * T8_ELEMENT_DIM[eclass_T1]), 1LL << (level * T8_ELEMENT_DIM[eclass_T2])};
  // return *leafs_from_root;
}

void
t8_2_5dimension_scheme_c::t8_element_general_function (const t8_element_t *elem, const void *indata,
                                                       void *outdata) const
{
  SC_ABORT ("This function is not implemented yet.\n");
}

#ifdef T8_ENABLE_DEBUG

int
t8_2_5dimension_scheme_c::t8_element_is_valid (const t8_element_t *elem) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  //return scheme1->t8_element_is_valid (e1) && scheme2->t8_element_is_valid (e2);
  return scheme1->t8_element_is_valid (el->elem1) && scheme2->t8_element_is_valid (el->elem2);

  //return t8_sele_is_valid<eclass_T1> ((const t8_standalone_element_t<eclass_T1> *) elem) && t8_sele_is_valid<eclass_T2> ((const t8_standalone_element_t<eclass_T2> *) elem);
}

void
t8_2_5dimension_scheme_c::t8_element_debug_print (const t8_element_t *elem) const
{
  // T8_ASSERT(t8_element_is_valid(elem));

  //WOFÜR BRAUCHE ICH DAS?

  // t8_sele_debug_print<eclass_T1> ((const t8_standalone_element_t<eclass_T1> *) elem);
  // t8_sele_debug_print<eclass_T2> ((const t8_standalone_element_t<eclass_T2> *) elem);
  // T8_ASSERT (t8_element_is_valid (elem));
}

void
t8_2_5dimension_scheme_c::t8_element_to_string (const t8_element_t *elem, char *debug_string,
                                                const int string_size) const
{
  SC_ABORT ("Not implemented for standalone.\n");
}
#endif

void
t8_2_5dimension_scheme_c::t8_element_new (int length, t8_element_t **elem) const
{
  /* allocate memory */
  //T8_ASSERT (scheme1->ts_context != NULL);
  //T8_ASSERT (scheme2->ts_context != NULL);
  T8_ASSERT (0 <= length);
  T8_ASSERT (elem != NULL);
  //static inline void *
  //sc_mempool_alloc (sc_mempool_t * mempool) @unused@???
  scheme1->t8_element_new (length, elem);
  scheme2->t8_element_new (length, elem);

  /* in debug mode, set sensible default values. */
#ifdef T8_ENABLE_DEBUG
  {
    int i;
    for (i = 0; i < length; i++) {
      //t8_element_init (1, elem[i]);
      t8_element_root (elem[i]);
    }
  }
#endif
}

void
t8_2_5dimension_scheme_c::t8_element_init (int length, t8_element_t *elem) const
{
#ifdef T8_ENABLE_DEBUG
  {
    t8_2_5D_t *el = (t8_2_5D_t *) elem;
    /* Set all values to 0 */  //remove this line?
    scheme1->t8_element_init (length, el->elem1);
    scheme2->t8_element_init (length, el->elem2);
  }
#endif
}

void
t8_2_5dimension_scheme_c::t8_element_deinit (int length, t8_element_t *elem) const
{
  SC_ABORT ("Not implemented for standalone.\n");
}

void
t8_2_5dimension_scheme_c::t8_element_destroy (int length, t8_element_t **elem) const
{
  SC_ABORT ("Next.\n");
  //   T8_ASSERT (this->ts_context != NULL);
  //   T8_ASSERT (0 <= length);
  //   T8_ASSERT (elem != NULL);
  //   for (int i = 0; i < length; ++i) {
  //     sc_mempool_free ((sc_mempool_t *) this->ts_context, elem[i]);
  //   }
}

void
t8_2_5dimension_scheme_c::t8_element_root (t8_element_t *elem) const
{
  SC_ABORT ("Not implemented for standalone.\n");
}

void
t8_2_5dimension_scheme_c::t8_element_MPI_Pack (t8_element_t **const elements, const unsigned int count,
                                               void *send_buffer, int buffer_size, int *position,
                                               sc_MPI_Comm comm) const
{
  SC_ABORT ("Not implemented for standalone.\n");
}

void
t8_2_5dimension_scheme_c::t8_element_MPI_Pack_size (const unsigned int count, sc_MPI_Comm comm, int *pack_size) const
{
  SC_ABORT ("Not implemented for standalone.\n");
}

void
t8_2_5dimension_scheme_c::t8_element_MPI_Unpack (void *recvbuf, const int buffer_size, int *position,
                                                 t8_element_t **elements, const unsigned int count,
                                                 sc_MPI_Comm comm) const
{
  SC_ABORT ("Not implemented for standalone.\n");
}

// typedef struct t8_2_5D
// {
//   t8_eclass_t eclass1 = scheme1->eclass;
//   t8_eclass_t eclass2 = scheme2->eclass;
//   if (eclass1 == T8_ECLASS_QUAD && eclass2 == T8_ECLASS_QUAD)
//   {
//     t8_dquad_t el1;
//     t8_dquad_
//   }
  
//   int level;
//   int x;
//   // //t8_eclass_t eclass1 = scheme1->eclass;
//   //   //switch (eclass1) {
//   //   switch (scheme1) {
//   //     case new t8_default_scheme_quad_c():
//   //     {
//   //       t8_dquad_t *el1;
//   //       //return el1;
//   //     }
//   //     case new t8_default_scheme_tri_c():
//   //     {
//   //       t8_dtri_t *el1;
//   //       //return el1;
//   //     }
//   //     case new t8_default_scheme_line_c():
//   //     {
//   //       t8_dline_t *el1;
//   //       //return el1;
//   //     }
//   //     default:
//   //       SC_ABORT ("Only schemes with eclass QUAD or TRIANGLE are allowed for scheme1 so far.\n");
//   //       break;
//   //   }
//     // t8_eclass_t eclass2 = scheme2->eclass;
//     // switch (eclass2) {
//     //   case T8_ECLASS_QUAD:
//     //   {
//     //     t8_dquad_t *el2;
//     //     return el2;
//     //   }
//     //   case T8_ECLASS_TRIANGLE:
//     //   {
//     //     t8_dtri_t *el2;
//     //     return el2;
//     //   }
//     //   case T8_ECLASS_LINE:
//     //   {
//     //     t8_dline_t *el2;
//     //     return el2;
//     //   }
//     //   default:
//     //     SC_ABORT ("Only schemes with eclass QUAD or TRIANGLE are allowed for scheme1 so far.\n");
//     //     break;
//     // }
// } t8_2_5D_t;