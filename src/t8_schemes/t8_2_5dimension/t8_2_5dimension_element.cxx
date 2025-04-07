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

#include <iostream>
#include <vector>
#include <t8_refcount.h>
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension_element.hxx>
#include <sc_functions.h>
#include <t8_schemes/t8_2_5dimension/t8_2_5D.hxx>
#include <t8_schemes/t8_default/t8_default_line/t8_dline.h>
#include <t8_schemes/t8_default/t8_default_quad/t8_dquad.h>
#include <t8_schemes/t8_default/t8_default_tri/t8_dtri.h>
#include <t8_schemes/t8_scheme.hxx>

/* This functions are used by other element functions and we thus need to
 * declare it up here */

/** This function assumes an sc_mempool_t as context.
 * It is suitable as the 2_5D_elem_new callback in \ref t8_eclass_scheme_t???.
 * We assume that the mempool has been created with the correct element size.
 * \param [in,out] scheme_context   An element is allocated in this sc_mempool_t.
 * \param [in]     length       Non-negative number of elements to allocate.
 * \param [in,out] elem         Array of correct size whose members are filled.
 */
static void
t8_2_5D_mempool_alloc (sc_mempool_t *scheme_context, int length, t8_element_t **elem);

/** This class independent function assumes an sc_mempool_t as context.
 * It is suitable as the 2_5D_elem_destroy callback in \ref t8_eclass_scheme_t???.
 * We assume that the mempool has been created with the correct element size.
 * \param [in,out] scheme_context   An element is returned to this sc_mempool_t.
 * \param [in]     length       Non-negative number of elements to destroy.
 * \param [in,out] elem         Array whose members are returned to the mempool.
 */
static void
t8_2_5D_mempool_free (sc_mempool_t *scheme_context, int length, t8_element_t **elem);

t8_2_5dimension_scheme::t8_2_5dimension_scheme (const t8_scheme *scheme, t8_eclass eclass1, t8_eclass eclass2)
  : scheme{scheme}
  , eclass1{eclass1}
  , eclass2{eclass2}
{
  if (eclass2 == T8_ECLASS_LINE && (eclass1 == T8_ECLASS_QUAD || eclass1 == T8_ECLASS_TRIANGLE)){
    element_size = 2 * sizeof (void *);
    scheme_context = sc_mempool_new (element_size);
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

t8_2_5dimension_scheme::~t8_2_5dimension_scheme ()
{
  T8_ASSERT (scheme_context != NULL);
  SC_ASSERT (((sc_mempool_t *) scheme_context)->elem_count == 0);
  sc_mempool_destroy ((sc_mempool_t *) scheme_context);
  t8_global_productionf("scheme->rc: %i", scheme->rc.refcount);
  scheme->unref ();
  // scheme->unref ();
}

size_t
t8_2_5dimension_scheme::get_element_size (void) const
{
  return 2 * sizeof (void *); //scheme->get_element_size(eclass1) * scheme->get_element_size(eclass2); //@TODO +? @Lukas
}

t8_eclass_t //@TODO add direction
t8_2_5dimension_scheme::get_eclass (void) const
{
  // if (dir == 1) {
  //   return eclass1;
  // }
  // else if (dir == 2) {
  //   return eclass2;
  // }
  // else {
  //   SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  // }
  return eclass1;
}

int
t8_2_5dimension_scheme::refines_irregular (void) const
{
  return 0;
}

int
t8_2_5dimension_scheme::get_maxlevel (void) const
{
  return 21;
}

int
t8_2_5dimension_scheme::element_get_level (const t8_element_t *elem, int dir) const
{
  T8_ASSERT (element_is_valid (elem));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme->element_get_level (eclass1, el->elem1);
  }
  else if (dir == 2) {
    return scheme->element_get_level (eclass2, el->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme::element_copy (const t8_element_t *source, t8_element_t *dest) const
{ 
  T8_ASSERT (element_is_valid (source));
  T8_ASSERT (element_is_valid (dest));

  const t8_2_5D_t *s = (const t8_2_5D_t *) source;
  t8_2_5D_t *d = (t8_2_5D_t *) dest;
  scheme->element_copy (eclass1, s->elem1, d->elem1);
  scheme->element_copy (eclass2, s->elem2, d->elem2);
}

int
t8_2_5dimension_scheme::element_compare (const t8_element_t *elem1, const t8_element_t *elem2) const
{
  T8_ASSERT (element_is_valid (elem1));
  T8_ASSERT (element_is_valid (elem2));

  const t8_2_5D_t *el1 = (const t8_2_5D_t *) elem1;
  const t8_2_5D_t *el2 = (const t8_2_5D_t *) elem2;
  int id1, id2;
  id1 = scheme->element_compare (eclass1, el1->elem1, el2->elem1);
  id2 = scheme->element_compare (eclass2, el1->elem2, el2->elem2);
  /*if same elements for eclass1, result compare is dependent from id2 */
  if (id1 == 0){ 
    return id2;
  }
  /* if elements for eclass1 are different, 2_5D element has to be different in the same way */
  else { 
    return id1;
  }
}

int
t8_2_5dimension_scheme::element_is_equal (const t8_element_t *elem1, const t8_element_t *elem2) const
{
  T8_ASSERT (element_is_valid (elem1));
  T8_ASSERT (element_is_valid (elem2));

  const t8_2_5D_t *el1 = (const t8_2_5D_t *) elem1;
  const t8_2_5D_t *el2 = (const t8_2_5D_t *) elem2;
  return scheme->element_is_equal (eclass1, el1->elem1, el2->elem1) && scheme->element_is_equal (eclass2, el1->elem2, el2->elem2);
}

void
t8_2_5dimension_scheme::element_get_parent (const t8_element_t *elem, t8_element_t *parent, int dir) const
{
  T8_ASSERT (element_is_valid (elem));
  T8_ASSERT (element_is_valid (parent));
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;  
  t8_2_5D_t *parent2_5D = (t8_2_5D_t *) parent;
  /* return parent of eclass1*/
  if (dir == 1) {
    scheme->element_get_parent (eclass1, el->elem1, parent2_5D->elem1);
    scheme->element_copy (eclass2, el->elem2, parent2_5D->elem2);
}
  /* return parent of eclass2*/
  else if (dir == 2) {
    /* For now it's required to have a line in z-coordinates/ eclass2 == T8_ECLASS_LINE*/  
    scheme->element_copy (eclass1, el->elem1, parent2_5D->elem1);
    scheme->element_get_parent (eclass2, el->elem2, parent2_5D->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

//parent in direction1 -> stattdessen t8_element_t *parent zu t8_element_t *p[] in parent Funktion?
void
t8_2_5dimension_scheme::element_get_parent_2_5D (const t8_element_t *elem, t8_element_t *p[]) const
{
  T8_ASSERT (element_is_valid (elem));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;

  #ifdef T8_ENABLE_DEBUG
  {
    int i;
    for (i = 0; i < scheme->element_get_num_children(eclass1, el->elem1, 1); i++) {
      T8_ASSERT (element_is_valid (p[i]));
    }
  }
  #endif  

  t8_2_5D_t **parent = (t8_2_5D_t **) p;

  int level2 = scheme->element_get_level (eclass2, el->elem2);
 
  int num_elems_per_column = scheme->count_leaves_from_root (eclass2, level2);  

  scheme->element_get_parent (eclass1, el->elem1, parent[0]->elem1); 
  scheme->element_copy (eclass2, el->elem2, parent[0]->elem2);

  // scheme1->t8_element_debug_print (parent[0]->elem1);
  // scheme2->t8_element_debug_print(parent[0]->elem2);

  for (int i = 1; i < num_elems_per_column; i++){
    scheme->element_get_parent (eclass1, el->elem1, parent[i]->elem1); //muss 4 mal berechnet werden
    scheme->element_construct_successor (eclass2, parent[i - 1]->elem2, parent[i]->elem2);
  }
}

int
t8_2_5dimension_scheme::element_get_num_siblings (const t8_element_t *elem, int dir) const
{
  T8_ASSERT (element_is_valid (elem));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    int num_siblings;
    int num_siblings1;
    int num_siblings2;
    int level2;
    num_siblings1 = scheme->element_get_num_siblings (eclass1, el->elem1);
    num_siblings2 = scheme->element_get_num_siblings (eclass2, el->elem2);        
    level2 = scheme->element_get_level (eclass2, el->elem2);

    num_siblings = num_siblings1 * pow(num_siblings2, level2);

    return num_siblings;
  }
  else if (dir == 2) {
    return scheme->element_get_num_siblings (eclass2, el->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme::element_get_sibling (const t8_element_t *elem, int sibid, t8_element_t *sibling, int dir) const
{
  T8_ASSERT (element_is_valid (elem));
  T8_ASSERT (element_is_valid (sibling));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t *sib = (t8_2_5D_t *) sibling;
  /*direct sibling in eclass1 */
  if (dir == 1) {
    scheme->element_get_sibling (eclass1, el->elem1, sibid, sib->elem1);
    scheme->element_copy (eclass2, el->elem2, sib->elem2);
    /*TODO
    * iterativ auch weitere Elemente der Säule in Sibling bezüglich eclass1 verfeinern
    */
  } 
  else if (dir == 2) {
    scheme->element_copy (eclass1, el->elem1, sib->elem1);
    scheme->element_get_sibling (eclass2, el->elem2, sibid, sib->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme::element_get_num_corners (const t8_element_t *elem) const
{
  T8_ASSERT (element_is_valid (elem));
  
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  return scheme->element_get_num_corners (eclass1, el->elem1) * scheme->element_get_num_corners (eclass2, el->elem2);
}

int
t8_2_5dimension_scheme::element_get_num_faces (const t8_element_t *elem) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO; /* suppresses compiler warning */
}

int
t8_2_5dimension_scheme::element_get_max_num_faces (const t8_element_t *elem) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme::element_get_num_children (const t8_element_t *elem, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    int num_children;
    int num_children1;
    int num_children2;
    int level2;
    num_children1 = scheme->element_get_num_children (eclass1, el->elem1);
    num_children2 = scheme->element_get_num_children (eclass2, el->elem2);        
    level2 = scheme->element_get_level (eclass2, el->elem2);

    num_children = num_children1*pow(num_children2,level2);

    return num_children;
  }
  else if (dir == 2) {
    return scheme->element_get_num_children (eclass2, el->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme::element_get_num_face_children (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme::element_get_face_corner (const t8_element_t *element, int face, int corner) const
{
  SC_ABORT ("This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme::element_get_corner_face (const t8_element_t *element, int corner, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

void
t8_2_5dimension_scheme::element_get_child (const t8_element_t *elem, int childid, t8_element_t *child, int dir) const
{
  T8_ASSERT (element_is_valid (elem));
  T8_ASSERT (element_is_valid (child));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t *c = (t8_2_5D_t *) child;
  /*direct child in eclass1 */
  if (dir == 1) {
    scheme->element_get_child (eclass1, el->elem1, childid, c->elem1);
    scheme->element_copy (eclass2, el->elem2, c->elem2);
    /*TODO
    * iterativ auch weitere Elemente der Säule in Kinder bezüglich eclass1 verfeinern
    */
  }
  else if (dir == 2) {
    scheme->element_copy (eclass1, el->elem1, c->elem1);
    scheme->element_get_child (eclass2, el->elem2, childid, c->elem2);
  }
  /*EXTRA CASE
  * childid possible between 0 and element_get_num_children(elem, 1) (=max number of possible children) 
  * SC_ABORT ("The function element_get_child is not uniquely defined for eclass1.\n");
  */
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme::element_get_children (const t8_element_t *elem, int length, t8_element_t *c[], int dir) const
{
  // //*c[] = *[t8_element_t, t8_element_t, t8_element_t, ...]
  T8_ASSERT (element_is_valid (elem));
  #ifdef T8_ENABLE_DEBUG
  {
    int i;
    for (i = 0; i < length; i++) {
      T8_ASSERT (element_is_valid (c[i]));
    }
  }
  #endif

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t **children = (t8_2_5D_t **) c;

  if (dir == 1) {
    int num_children1 = scheme->element_get_num_children (eclass1, el->elem1);
    t8_element_t **c1 = T8_ALLOC(t8_element_t *, num_children1);
    int num_children2 = scheme->element_get_num_children (eclass2, el->elem2);
    int level2 = scheme->element_get_level (eclass2, el->elem2);

    scheme->element_new (eclass1, num_children1, c1);
    scheme->element_get_children (eclass1, el->elem1, num_children1, c1);
    
    //only refined in dir1
    if (level2 == 0){

      for (int i = 0; i < num_children1; i++){
        scheme->element_copy (eclass1, c1[i], children[i]->elem1);
        scheme->element_copy (eclass2, el->elem2, children[i]->elem2);
      }
    }
    else{

      int num_elems_per_column = scheme->count_leaves_from_root(eclass2, level2);  

      for (int i = 0; i < num_children1; i++){
        scheme->element_copy (eclass1, c1[i], children[i * num_elems_per_column]->elem1);
        scheme->element_copy (eclass2, el->elem2, children[i * num_elems_per_column]->elem2);
        t8_global_productionf ("i: %i", i);
        for (int j = 1; j < num_elems_per_column; j++){
          int pos = i * num_elems_per_column + j;
          scheme->element_copy (eclass1, c1[i], children[pos]->elem1);
          scheme->element_construct_successor (eclass2, children[pos-1]->elem2, children[pos]->elem2);
        }
      }
    }

    scheme->element_destroy (eclass1, num_children1, c1);

    T8_FREE(c1);
  }
  else if (dir == 2) {

    t8_element_t **c2 = T8_ALLOC(t8_element_t *, length); 
    scheme->element_new (eclass2, length, c2);
    scheme->element_get_children (eclass2, el->elem2, length, c2);
    for (int i = 0; i < length; i++)
    {
      scheme->element_copy (eclass1, el->elem1, children[i]->elem1);
      scheme->element_copy (eclass2, c2[i], children[i]->elem2);
    }

    scheme->element_destroy (eclass2, length, c2);
    T8_FREE(c2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme::element_get_child_id (const t8_element_t *elem, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme->element_get_child_id (eclass1, el->elem1);
  }
  else if (dir == 2) {
    return scheme->element_get_child_id (eclass2, el->elem2);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

int
t8_2_5dimension_scheme::element_get_ancestor_id (const t8_element_t *elem, int level, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 1) {
    return scheme->element_get_ancestor_id (eclass1, el->elem1, level);
  }
  else if (dir == 2) {
    return scheme->element_get_ancestor_id (eclass2, el->elem2, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}


int
t8_2_5dimension_scheme::elements_are_family (t8_element_t *const *fam, int dir) const
{
  int is_family;

  if (dir == 1) {
    t8_global_productionf("CASE elements_are_family direction1");
    #ifdef T8_ENABLE_DEBUG
    {
      int i;
      for (i = 0; i < element_get_num_children(fam[0], 1); i++) {
        T8_ASSERT (element_is_valid (fam[i]));
      }
    }
    #endif
    const t8_2_5D_t *f0 = (const t8_2_5D_t *) fam[0];
    
    /*general assumption: columns look alike 
    *-> horizontal refinement only done after uniform forest was constructed and before vertical refinement
    */
    int num_siblings1 = scheme->element_get_num_siblings (eclass1, f0->elem1);
    t8_global_productionf ("num_siblings1: %i", num_siblings1);

    int level2_elem0 = scheme->element_get_level (eclass2, f0->elem2);
    int num_elems_dir2 = scheme->count_leaves_from_root (eclass2, level2_elem0);

    t8_global_productionf ("num_elems_dir2: %i", num_elems_dir2);
    //holds due to assumption thatin direction 2 uniform refined
    int elems_in_family = num_siblings1 * num_elems_dir2;

    int level1_elem0 = scheme->element_get_level (eclass1, f0->elem1);

    std::vector<int> levels_elem0 = {level1_elem0, level2_elem0};
    int lin_id_elem1 = element_get_linear_id (fam[0], levels_elem0);

    const t8_2_5D_t *f1 = (const t8_2_5D_t *) fam[num_elems_dir2]; 

    int level1_elem1 = scheme->element_get_level (eclass1, f1->elem1);
    int level2_elem1 = scheme->element_get_level (eclass2, f1->elem2);
    std::vector<int> levels_elem1 = {level1_elem1, level2_elem1};
    int lin_id_elem2 = element_get_linear_id (fam[num_elems_dir2], levels_elem1);

    if (lin_id_elem1 + num_elems_dir2 == lin_id_elem2 && level2_elem0 != 0 && level2_elem1 != 0){ //braucht man letzte && Bedingung? oder level2_elem0 == level2_elem1??  
      t8_element_t **fam1;
      fam1 = T8_ALLOC (t8_element_t *, num_siblings1); 
      int is_equal = 0;
      int is_equal_dir1 = 0;
      for (int i = 0; i < elems_in_family; i+=num_elems_dir2){
        const t8_2_5D_t *elem = (const t8_2_5D_t *) fam[i];
        int pos = i / num_elems_dir2;
        fam1[pos] = elem->elem1;
      }
      for (int i = 0; i < num_elems_dir2 ; i++){
        is_equal_dir1 = 0;
        for (int j = 0; j < num_siblings1 - 1; j++){
          const t8_2_5D_t *elem = (const t8_2_5D_t *) fam[i + j * num_elems_dir2];
          const t8_2_5D_t *elem_comp = (const t8_2_5D_t *) fam[i + (j+1) * num_elems_dir2];
          if (scheme->element_is_equal (eclass2, elem->elem2, elem_comp->elem2)){
            is_equal_dir1+=1;
          }
        }
        if (is_equal_dir1 == num_siblings1-1){
          is_equal++;
        }
      }

      is_family = scheme->elements_are_family (eclass1, fam1) && (is_equal == num_elems_dir2);
      T8_FREE (fam1);
      return is_family;
    }
    else{
      return 0;
    }
  }
  else if (dir == 2) {
    #ifdef T8_ENABLE_DEBUG
    {
      int i;
      for (i = 0; i < element_get_num_children(fam[0], 2); i++) {
        T8_ASSERT (element_is_valid (fam[i]));
      }
    }
    #endif

    const t8_2_5D_t *f0 = (const t8_2_5D_t *) fam[0];
    const t8_2_5D_t *f1 = (const t8_2_5D_t *) fam[1];

    int num_siblings2 = scheme->element_get_num_siblings (eclass2, f0->elem2);

    int level1_elem0 = scheme->element_get_level (eclass1, f0->elem1);
    int level2_elem0 = scheme->element_get_level (eclass2, f0->elem2);
    std::vector<int> levels_elem0 = {level1_elem0, level2_elem0};
    int lin_id_elem1 = element_get_linear_id (fam[0], levels_elem0);

    int level1_elem1 = scheme->element_get_level (eclass1, f1->elem1);
    int level2_elem1 = scheme->element_get_level (eclass2, f1->elem2);
    std::vector<int> levels_elem1 = {level1_elem1, level2_elem1};
    int lin_id_elem2 = element_get_linear_id (fam[1], levels_elem1);
    //+1 as this is dependent direction
    if (lin_id_elem1 + 1 == lin_id_elem2 && level2_elem0 != 0 && level2_elem1 != 0){ 
      t8_element **fam2;
      fam2 = T8_ALLOC (t8_element_t *, num_siblings2); 
      int is_equal = 0;
      for (int i = 0; i < num_siblings2; i++){
        t8_global_productionf ("i: %i", i);
        const t8_2_5D_t *elem = (const t8_2_5D_t *) fam[i];
        fam2[i] = elem->elem2;
        if (i < num_siblings2 - 1){
          const t8_2_5D_t *elem_comp = (const t8_2_5D_t *) fam[i+1];
          if (scheme->element_is_equal (eclass1, elem->elem1, elem_comp->elem1)){
            is_equal+=1;
          }
        }
      }

      is_family = scheme->elements_are_family (eclass2, fam2) && (is_equal == (num_siblings2 - 1));
      T8_FREE (fam2);
      return is_family;
    }
    else{
      return 0;
    }
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme::element_get_nca (const t8_element_t *elem1, const t8_element_t *elem2, t8_element_t *nca,
                                          int dir) const
{
  if (dir == 1) {
    return scheme->element_get_nca (eclass1, elem1, elem2, nca);
  }
  else if (dir == 2) {
    return scheme->element_get_nca (eclass2, elem1, elem2, nca);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

t8_element_shape_t
t8_2_5dimension_scheme::element_get_face_shape (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

void
t8_2_5dimension_scheme::element_get_children_at_face (const t8_element_t *elem, int face, t8_element_t *children[],
                                                       int num_children, int *child_indices) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
}

int
t8_2_5dimension_scheme::element_face_get_child_face (const t8_element_t *elem, int face, int face_child, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. Waiting for scheme interface.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme::element_face_get_parent_face (const t8_element_t *elem, int face, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. Waiting for scheme interface.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme::element_get_tree_face (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

void
t8_2_5dimension_scheme::element_transform_face (const t8_element_t *elem1, t8_element_t *elem2, int orientation,
                                                     int sign, int is_smaller_face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
}

int
t8_2_5dimension_scheme::element_extrude_face (const t8_element_t *face, t8_element_t *elem, const int root_face,
                                                  const t8_scheme *scheme, int dir) const
{
  SC_ABORT ("[FACE] Waiting for scheme interface.\n");
  return T8_ECLASS_ZERO;
}

void
t8_2_5dimension_scheme::element_get_boundary_face (const t8_element_t *elem, int face, t8_element_t *boundary,
                                                    [[maybe_unused]] const t8_scheme *scheme) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");

}

void
t8_2_5dimension_scheme::element_get_first_descendant_face (const t8_element_t *elem, int face,
                                                            t8_element_t *first_desc, int level, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. & Waiting for scheme interface.\n");
}

void
t8_2_5dimension_scheme::element_get_last_descendant_face (const t8_element_t *elem, int face, t8_element_t *last_desc,
                                                           int level, int dir) const
{
  SC_ABORT ("[FACE] This function is not implemented yet. & Waiting for scheme interface.\n");
}

// void
// t8_2_5dimension_scheme::element_boundary (const t8_element_t *elem, int min_dim, int length,
//                                                t8_element_t **boundary) const
// {
//   SC_ABORT ("[FACE] This function is not implemented yet.\n");
// }

int
t8_2_5dimension_scheme::element_is_root_boundary (const t8_element_t *elem, int face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

int
t8_2_5dimension_scheme::element_get_face_neighbor_inside (const t8_element_t *elem, t8_element_t *neigh, int face,
                                                           int *neigh_face) const
{
  SC_ABORT ("[FACE] This function is not implemented yet.\n");
  return T8_ECLASS_ZERO;
}

t8_element_shape_t
t8_2_5dimension_scheme::element_get_shape (const t8_element_t *elem) const
{
  t8_2_5D_t *el = (t8_2_5D_t *) elem;
  if (scheme->element_get_shape (eclass1, el->elem1) == T8_ECLASS_QUAD && scheme->element_get_shape (eclass2, el->elem2) == T8_ECLASS_LINE) {
    return T8_ECLASS_HEX;
  }
  else if (scheme->element_get_shape (eclass1, el->elem1) == T8_ECLASS_TRIANGLE && scheme->element_get_shape (eclass2, el->elem2) == T8_ECLASS_LINE){
    return T8_ECLASS_PRISM;
  }
  else {
    t8_global_productionf ("Invalid combination of eclass_schemes: %i x %i", scheme->element_get_shape (eclass1, el->elem1), scheme->element_get_shape (eclass2, el->elem2));
    SC_ABORT ("Invalid combination of eclass_schemes.\n");
  }
} 

void
t8_2_5dimension_scheme::element_set_linear_id (t8_element_t *elem, std::vector<int>& levels, t8_linearidx_t id) const
{
  T8_ASSERT (element_is_valid (elem));
  t8_2_5D_t *el = (t8_2_5D_t *) elem;
  t8_linearidx_t id_scheme;
  std::vector<int> level1 = {levels[0]};
  std::vector<int> level2 = {levels[1]};

  int num_elems_per_column = scheme->count_leaves_from_root(eclass2, levels[1]);
  t8_global_productionf ("num_elems_per_column: %i\n", num_elems_per_column);
  id_scheme = id / num_elems_per_column;
  t8_global_productionf ("id for eclass1: %li\n", id_scheme);
  scheme->element_set_linear_id (eclass1, el->elem1, level1, id_scheme);
  id_scheme = id % num_elems_per_column;
  t8_global_productionf ("id for eclass2: %li\n", id_scheme);
  scheme->element_set_linear_id (eclass2, el->elem2, level2, id_scheme);
}

t8_linearidx_t
t8_2_5dimension_scheme::element_get_linear_id (const t8_element_t *elem, std::vector<int>& levels) const
{
  T8_ASSERT (element_is_valid (elem));
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  std::vector<int> level1 = {levels[0]};
  std::vector<int> level2 = {levels[1]};
  t8_linearidx_t lin_id;

  if (levels[1] == 0){
    lin_id = scheme->element_get_linear_id (eclass1, el->elem1, level1);
  }
  else{
    lin_id = scheme->element_get_linear_id (eclass1, el->elem1, level1) * sc_intpow (scheme->element_get_num_children (eclass2, el->elem2), levels[1]) 
                  + scheme->element_get_linear_id (eclass2, el->elem2, level2);
  }
  return lin_id;
}

void
t8_2_5dimension_scheme::element_get_first_descendant (const t8_element_t *elem, t8_element_t *desc, std::vector<int>& levels) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t *d = (t8_2_5D_t *) desc;
  std::vector<int> level1 = {levels[0]};
  std::vector<int> level2 = {levels[1]};

  scheme->element_get_first_descendant (eclass1, el->elem1, d->elem1, level1);
  scheme->element_get_first_descendant (eclass2, el->elem2, d->elem2, level2);
}

void
t8_2_5dimension_scheme::element_get_last_descendant (const t8_element_t *elem, t8_element_t *desc, std::vector<int>& levels) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  t8_2_5D_t *d = (t8_2_5D_t *) desc;
  std::vector<int> level2 = {levels[1]};

  int level1_max = get_maxlevel(); 
  t8_productionf("level1_max: %i", level1_max);
  int level1 = scheme->element_get_level (eclass1, el->elem1);
  if (level1_max == level1){
    scheme->element_copy (eclass1, el->elem1, d->elem1);
  }
  else{
    scheme->element_copy (eclass1, el->elem1, d->elem1);
    while (level1 < level1_max){
      t8_productionf("level1: %i", level1);
      scheme->element_get_child(eclass1, d->elem1, 0, d->elem1);
      level1 += 1;
    }
  }

  scheme->element_get_last_descendant (eclass2, el->elem2, d->elem2, level2);
}

void
t8_2_5dimension_scheme::element_construct_successor (const t8_element_t *t, t8_element_t *s, [[maybe_unused]] int dir) const
{
  T8_ASSERT (element_is_valid (t));
  t8_global_productionf ("Test Succ");
  T8_ASSERT (element_is_valid (s));

  const t8_2_5D_t *tel = (const t8_2_5D_t *) t;
  t8_2_5D_t *sel = (t8_2_5D_t *) s;

  int level1 = element_get_level (t, 1); //scheme->element_get_level (eclass1, tel->elem1);
  int level2 = element_get_level (t, 2);

  std::vector<int> levels = {level1, level2};
  t8_linearidx_t lin_id = element_get_linear_id (t, levels);
  int num_elems_per_column = scheme->count_leaves_from_root(eclass2, levels[1]);

  if ((lin_id + 1) % num_elems_per_column != 0) {
    scheme->element_copy (eclass1, tel->elem1, sel->elem1);
    scheme->element_construct_successor (eclass2, tel->elem2, sel->elem2);
  }
  else {
    scheme->set_to_root (eclass2, sel->elem2);
    int level_t = scheme->element_get_level (eclass2, tel->elem2);
    for (int i = 0; i < level_t; i++) {
      scheme->element_get_child (eclass2, sel->elem2, 0, sel->elem2);
    }
    scheme->element_construct_successor (eclass1, tel->elem1, sel->elem1);
  }
}

void
t8_2_5dimension_scheme::element_get_vertex_reference_coords (const t8_element_t *t, const int vertex,
                                                              double coords[]) const
{
  SC_ABORT ("Next.\n");
}

void
t8_2_5dimension_scheme::element_get_reference_coords (const t8_element_t *elem, const double *ref_coords,
                                                       const size_t num_coords, double *out_coords) const
{
  T8_ASSERT (element_is_valid (elem));
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  int dim1, dim2;
  dim1 = t8_eclass_to_dimension[eclass1];
  dim2 = t8_eclass_to_dimension[eclass2];
  for (size_t coord = 0; coord < num_coords; ++coord) {
    scheme->element_get_reference_coords (eclass1, el->elem1, (ref_coords + (coord * (dim1 + dim2))), num_coords, (out_coords + (coord * (dim1 + dim2))));
    scheme->element_get_reference_coords (eclass2, el->elem2, ref_coords + (coord * (dim1 + dim2) + dim1), num_coords, out_coords + (coord * (dim1 + dim2) + dim1));
  }
  size_t size = (dim1 + dim2)*num_coords;
}

t8_gloidx_t
t8_2_5dimension_scheme::element_count_leaves (const t8_element_t *elem, const int level, int dir) const
{ 
  if (dir == 1) {
    return scheme->element_count_leaves (eclass1, elem, level);
  }
  else if (dir == 2) {
    return scheme->element_count_leaves (eclass2, elem, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

t8_gloidx_t
t8_2_5dimension_scheme::count_leaves_from_root (const int level, int dir) const
{
  if (dir == 1) {
    return scheme->count_leaves_from_root (eclass1, level);
  }
  else if (dir == 2) {
    return scheme->count_leaves_from_root (eclass2, level);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
}

void
t8_2_5dimension_scheme::element_general_function (const t8_element_t *elem, const void *indata,
                                                       void *outdata) const
{
  SC_ABORT ("This function is not implemented yet.\n");
}

bool
t8_2_5dimension_scheme::element_is_refinable (const t8_element_t *elem) const
{
  T8_ASSERT (scheme->element_is_valid (eclass1, elem));
  T8_ASSERT (scheme->element_is_valid (eclass2, elem));

  return scheme->element_is_refinable (eclass1, elem) && scheme->element_is_refinable (eclass2, elem);
}

#ifdef T8_ENABLE_DEBUG

int
t8_2_5dimension_scheme::element_is_valid (const t8_element_t *elem) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  return scheme->element_is_valid (eclass1, el->elem1) && scheme->element_is_valid (eclass2, el->elem2);
}

void
t8_2_5dimension_scheme::element_debug_print (const t8_element_t *elem) const
{
  T8_ASSERT(element_is_valid(elem));

  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  scheme->element_debug_print (eclass1, el->elem1);
  scheme->element_debug_print (eclass2, el->elem2);
}

void
t8_2_5dimension_scheme::element_to_string (const t8_element_t *elem, char *debug_string,
                                                const int string_size) const
{
  SC_ABORT ("Not implemented for 2.5D.\n");
}
#endif

static void
t8_2_5D_mempool_alloc (sc_mempool_t *scheme_context, int length, t8_element_t **elem)
{
  int i;

  T8_ASSERT (scheme_context != NULL);
  T8_ASSERT (0 <= length);
  T8_ASSERT (elem != NULL);

  for (i = 0; i < length; ++i) {
    elem[i] = (t8_element_t *) sc_mempool_alloc (scheme_context);
    
  }
}

void
t8_2_5dimension_scheme::element_new (int length, t8_element_t **elem) const
{
    t8_2_5D_mempool_alloc ((sc_mempool_t *) this->scheme_context, length, elem);
    for (int i = 0; i < length; i++) {
      t8_2_5D_t *el = (t8_2_5D_t *) elem[i];
      scheme->element_new (eclass1, 1, &el->elem1);
      scheme->element_new (eclass2, 1, &el->elem2);
    }

    /* in debug mode, set sensible default values. */
  #ifdef T8_ENABLE_DEBUG
    {
      for (int i = 0; i < length; i++) {
        set_to_root (elem[i]);
      }
    }
  #endif
}

void
t8_2_5dimension_scheme::element_init (int length, t8_element_t *elem) const
{
    t8_2_5D_t *el = (t8_2_5D_t *) elem;
    /* Set all values to 0 */

    for (int i = 0; i < length; i++) {
      t8_2_5D_t *elem_init = el + i;
      scheme->element_new (eclass1, 1, &elem_init->elem1);
      scheme->element_new (eclass2, 1, &elem_init->elem2);
    }
}

void //@TODO Unterschied zu element_destroy
t8_2_5dimension_scheme::element_deinit (int length, t8_element_t *elem) const
{
    for (int i = 0; i < length; i++) {
      t8_2_5D_t *el = (t8_2_5D_t *) elem + i;
      scheme->element_destroy (eclass1, 1, &el->elem1);
      scheme->element_destroy (eclass2, 1, &el->elem2);
    }
}

static void
t8_2_5D_mempool_free (sc_mempool_t *scheme_context, int length, t8_element_t **elem)
{
  T8_ASSERT (scheme_context != NULL);
  T8_ASSERT (0 <= length);
  T8_ASSERT (elem != NULL);

  for (int i = 0; i < length; ++i) {
    sc_mempool_free (scheme_context, elem[i]);
  }
}

void
t8_2_5dimension_scheme::element_destroy (int length, t8_element_t **elem) const
{
    for (int i = 0; i < length; i++) {
      t8_2_5D_t *el = (t8_2_5D_t *) elem[i];
      scheme->element_destroy (eclass1, 1, &el->elem1);
      scheme->element_destroy (eclass2, 1, &el->elem2);
    }

    t8_2_5D_mempool_free ((sc_mempool_t *) this->scheme_context, length, elem);
}

void
t8_2_5dimension_scheme::set_to_root (t8_element_t *elem) const
{
  t8_2_5D_t *el = (t8_2_5D_t *) elem;
  scheme->set_to_root (eclass1, el->elem1);
  scheme->set_to_root (eclass2, el->elem2);
}

int //besser: void
t8_2_5dimension_scheme::element_get_variable (const t8_element_t *elem, int var, int dir) const
{
  const t8_2_5D_t *el = (const t8_2_5D_t *) elem;
  if (dir == 0) {
    int x = scheme->element_get_variable (eclass1, el->elem1, 1);
    int y = scheme->element_get_variable (eclass1, el->elem1, 2);
    int z = scheme->element_get_variable (eclass2, el->elem2, 1);
    t8_global_productionf ("element coordinates 2_5D: (%i,%i) x %i \n", x, y, z);
  }
  else if (dir == 1) {
    scheme->element_get_variable (eclass1, el->elem1, var);
  }
  else if (dir == 2) {
    scheme->element_get_variable (eclass2, el->elem2, var);
  }
  else {
    SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
  }
  return 0;
}

void
t8_2_5dimension_scheme::element_MPI_Pack (t8_element_t **const elements, const unsigned int count,
                                               void *send_buffer, int buffer_size, int *position,
                                               sc_MPI_Comm comm) const
{
  SC_ABORT ("Not implemented for 2.5D.\n");
}

void
t8_2_5dimension_scheme::element_MPI_Pack_size (const unsigned int count, sc_MPI_Comm comm, int *pack_size) const
{
  SC_ABORT ("Not implemented for 2.5D.\n");
}

void
t8_2_5dimension_scheme::element_MPI_Unpack (void *recvbuf, const int buffer_size, int *position,
                                                 t8_element_t **elements, const unsigned int count,
                                                 sc_MPI_Comm comm) const
{
  SC_ABORT ("Not implemented for 2.5D.\n");
}