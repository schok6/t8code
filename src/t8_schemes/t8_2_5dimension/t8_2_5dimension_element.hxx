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
#include <t8_element.h>
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef T8_2_5DIMENSION_ELEMENT_CXX_HXX
#define T8_2_5DIMENSION_ELEMENT_CXX_HXX

#include <iostream>
#include <vector>
#include <utility>

#include <t8_element.h>
#include <t8_eclass.h>
#include <sc_functions.h>
#include <t8_schemes/t8_2_5dimension/t8_2_5D.hxx>

/** Provide an implementation for 2 schemes and
 * refinement in x,y and z (or x and y,z).
*/

/** This function assumes an sc_mempool_t as context.
 * It is suitable as the 2_5D_elem_new callback in \ref t8_eclass_scheme_t???.
 * We assume that the mempool has been created with the correct element size.
 * \param [in,out] scheme_2_5D_pool   An element is allocated in this sc_mempool_t.
 * \param [in]     length       Non-negative number of elements to allocate.
 * \param [in,out] elem         Array of correct size whose members are filled.
 */
inline static void
t8_2_5D_mempool_alloc (sc_mempool_t *scheme_2_5D_pool, int length, t8_element_t **elem)
{
  int i;

  T8_ASSERT (scheme_2_5D_pool != NULL);
  T8_ASSERT (0 <= length);
  T8_ASSERT (elem != NULL);

  for (i = 0; i < length; ++i) {
    elem[i] = (t8_element_t *) sc_mempool_alloc (scheme_2_5D_pool);
  }
}

/* Forward declaration of the scheme so we can use it as an argument in the eclass schemes function. */
class t8_scheme;

template <typename TUnderlyingElementType1, typename TUnderlyingElementType2>
struct t8_2_5D_element
{
  TUnderlyingElementType1 linear_element1;
  TUnderlyingElementType2 linear_element2;
};

template <class TUnderlyingEclassScheme1, typename TUnderlyingElementType1, class TUnderlyingEclassScheme2,
          typename TUnderlyingElementType2>
class t8_2_5dimension_scheme: private TUnderlyingEclassScheme1, TUnderlyingEclassScheme2 {
 private:
  using element_2_5D = t8_2_5D_element<TUnderlyingElementType1, TUnderlyingElementType2>;

 protected:
  size_t element_size_2_5D; /**< The size in bytes of a 2.5D element of class \a eclass1 and class \a eclass2*/
  void *scheme_2_5D_pool;   /**< Memory pool for 2_5D elements. */

 public:
  t8_2_5dimension_scheme ()
    : element_size_2_5D (sizeof (element_2_5D)), scheme_2_5D_pool (sc_mempool_new (element_size_2_5D)) {};

  /** Move constructor */
  t8_2_5dimension_scheme (t8_2_5dimension_scheme &&other) noexcept
    : TUnderlyingEclassScheme1 (std::move (other)), TUnderlyingEclassScheme2 (std::move (other)),
      element_size_2_5D (other.element_size_2_5D),
      scheme_2_5D_pool (std::exchange (other.scheme_2_5D_pool, nullptr)) {};

  /** Move assignment operator */
  t8_2_5dimension_scheme &
  operator= (t8_2_5dimension_scheme &&other) noexcept
  {
    if (this != &other) {
      // Free existing resources of moved-to object
      if (scheme_2_5D_pool) {
        sc_mempool_destroy ((sc_mempool_t *) scheme_2_5D_pool);
      }

      // Transfer ownership of resources
      element_size_2_5D = other.element_size_2_5D;
      scheme_2_5D_pool = other.scheme_2_5D_pool;

      // Leave the source object in a valid state
      other.scheme_2_5D_pool = nullptr;
    }
    TUnderlyingEclassScheme1::operator= (std::move (other));
    TUnderlyingEclassScheme2::operator= (std::move (other));
    return *this;
  }

  /** Copy constructor */
  t8_2_5dimension_scheme (const t8_2_5dimension_scheme &other)
    : TUnderlyingEclassScheme1 (other), TUnderlyingEclassScheme2 (other), element_size_2_5D (other.element_size_2_5D),
      scheme_2_5D_pool (sc_mempool_new (other.element_size_2_5D)) {};

  /** Copy assignment operator */
  t8_2_5dimension_scheme &
  operator= (const t8_2_5dimension_scheme &other)
  {
    if (this != &other) {
      // Free existing resources of assigned-to object
      if (scheme_2_5D_pool) {
        sc_mempool_destroy ((sc_mempool_t *) scheme_2_5D_pool);
      }

      // Copy the values from the source object
      element_size_2_5D = other.element_size_2_5D;
      scheme_2_5D_pool = sc_mempool_new (other.element_size_2_5D);
    }
    TUnderlyingEclassScheme1::operator= (other);
    TUnderlyingEclassScheme2::operator= (other);
    return *this;
  }

  /** Destructor for 2.5D scheme */
  ~t8_2_5dimension_scheme ()
  {
    if (scheme_2_5D_pool != NULL) {
      SC_ASSERT (((sc_mempool_t *) scheme_2_5D_pool)->elem_count == 0);
      sc_mempool_destroy ((sc_mempool_t *) scheme_2_5D_pool);
    }
  }

  /** Return the tree class of this scheme.
   * \return The tree class of this scheme.
   */
  inline t8_eclass_t
  get_eclass () const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Return the size of a 2.5D element.
  * \return  The size of a 2.5D element with 2 eclasses.
  */
  inline size_t
  get_element_size (void) const
  {
    return sizeof (element_2_5D);
  }

  /** Returns true, if there is one element in the tree, that does not refine into 2^dim children.
   * Returns false otherwise.
   * \return                    non-zero if there is one element in the tree that does not refine into 2^dim children.
   */
  inline int
  refines_irregular (void) const
  {
    return 0;
  }

  /** Return the maximum allowed level for any element of a given class.
   * \return                      The maximum allowed level for elements of class \b ts.
   */
  inline int
  get_maxlevel (void) const
  {
    /* For now just one common maximum level
    * TODO: Allow different maximum levels in vertical and horizontal direction
    */
    return 21;
  }

  /** Return the level of a particular element.
   * \param [in] elem    The element whose level should be returned.
   * \return             The level of \b elem.
   */
  inline int
  element_get_level (const t8_element_t *elem) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  /** Return the level of a particular element.
   * \param [in] elem    The element whose level should be returned.
   * \return             The level of \b elem.
   */
  inline int
  element_get_level (const t8_element_t *elem, int dir) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;
    if (dir == 1) {
      return TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &el->linear_element1);
    }
    else if (dir == 2) {
      return TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &el->linear_element2);
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** Copy all entries of \b source to \b dest. \b dest must be an existing
   *  element. No memory is allocated by this function.
   * \param [in] source The element whose entries will be copied to \b dest.
   * \param [in,out] dest This element's entries will be overwrite with the
   *                    entries of \b source.
   * \note \a source and \a dest may point to the same element.
   */
  inline void
  element_copy (const t8_element_t *source, t8_element_t *dest) const
  {
    T8_ASSERT (element_is_valid (source));
    if (source == dest)
      return;
    const element_2_5D *s = (const element_2_5D *) source;
    element_2_5D *d = (element_2_5D *) dest;
    TUnderlyingEclassScheme1::element_copy ((t8_element_t *) &s->linear_element1, (t8_element_t *) &d->linear_element1);
    TUnderlyingEclassScheme2::element_copy ((t8_element_t *) &s->linear_element2, (t8_element_t *) &d->linear_element2);

    T8_ASSERT (element_is_valid (dest));
  }

  /** Compare two elements.
   * \param [in] elem1  The first element.
   * \param [in] elem2  The second element.
   * \return       negative if elem1 < elem2, zero if elem1 equals elem2
   *               and positive if elem1 > elem2.
   *  If elem2 is a copy of elem1 then the elements are equal.
   */
  inline int
  element_compare (const t8_element_t *elem1, const t8_element_t *elem2) const
  {
    T8_ASSERT (element_is_valid (elem1));
    T8_ASSERT (element_is_valid (elem2));

    const element_2_5D *el1 = (const element_2_5D *) elem1;
    const element_2_5D *el2 = (const element_2_5D *) elem2;
    int id1, id2;
    id1 = TUnderlyingEclassScheme1::element_compare ((t8_element_t *) &el1->linear_element1,
                                                     (t8_element_t *) &el2->linear_element1);
    id2 = TUnderlyingEclassScheme2::element_compare ((t8_element_t *) &el1->linear_element2,
                                                     (t8_element_t *) &el2->linear_element2);
    /*if same elements for eclass1, result compare is dependent from id2 */
    if (id1 == 0) {
      return id2;
    }
    /* if elements for eclass1 are different, 2_5D element has to be different in the same way */
    else {
      return id1;
    }
  }

  /** Check if two elements are equal.
  * \param [in] ts     Implementation of a class scheme.
  * \param [in] elem1  The first element.
  * \param [in] elem2  The second element.
  * \return            1 if the elements are equal, 0 if they are not equal
  */
  inline int
  element_is_equal (const t8_element_t *elem1, const t8_element_t *elem2) const
  {
    T8_ASSERT (element_is_valid (elem1));
    T8_ASSERT (element_is_valid (elem2));

    const element_2_5D *el1 = (const element_2_5D *) elem1;
    const element_2_5D *el2 = (const element_2_5D *) elem2;
    return TUnderlyingEclassScheme1::element_is_equal ((t8_element_t *) &el1->linear_element1,
                                                       (t8_element_t *) &el2->linear_element1)
           && TUnderlyingEclassScheme2::element_is_equal ((t8_element_t *) &el1->linear_element2,
                                                          (t8_element_t *) &el2->linear_element2);
  }

  /** Compute the parent of a given element \b elem and store it in \b parent.
   *  \b parent needs to be an existing element. No memory is allocated by this function.
   *  \b elem and \b parent can point to the same element, then the entries of
   *  \b elem are overwritten by the ones of its parent.
   * \param [in] elem   The element whose parent will be computed.
   * \param [in,out] parent This element's entries will be overwritten by those
   *                    of \b elem's parent.
   *                    The storage for this element must exist
   *                    and match the element class of the parent.
   *                    For a pyramid, for example, it may be either a
   *                    tetrahedron or a pyramid depending on \b elem's childid.
   */
  inline void
  element_get_parent (const t8_element_t *elem, t8_element_t *parent) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  inline void
  element_get_parent (const t8_element_t *elem, t8_element_t *parent, int dir) const
  {
    T8_ASSERT (element_is_valid (elem));
    const element_2_5D *el = (const element_2_5D *) elem;
    element_2_5D *parent2_5D = (element_2_5D *) parent;
    /* return parent of eclass1*/
    if (dir == 1) {
      SC_ABORT ("For now implemented in element_get_parent_2_5D.\n");
    }
    /* return parent of eclass2*/
    else if (dir == 2) {
      /* For now it's required to have a line in z-coordinates/ eclass2 == T8_ECLASS_LINE*/
      TUnderlyingEclassScheme1::element_copy ((t8_element_t *) &el->linear_element1,
                                              (t8_element_t *) &parent2_5D->linear_element1);
      TUnderlyingEclassScheme2::element_get_parent ((t8_element_t *) &el->linear_element2,
                                                    (t8_element_t *) &parent2_5D->linear_element2);
      T8_ASSERT (element_is_valid (parent));
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  // /**
  //  * @TODO: Declaration
  // */
  inline void
  element_get_parent_2_5D (const t8_element_t *elem, t8_element_t *p[]) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;

    int level2 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &el->linear_element2);

    int num_elems_per_column = TUnderlyingEclassScheme2::count_leaves_from_root (level2);

#ifdef T8_ENABLE_DEBUG
    {
      int i;
      for (i = 0; i < num_elems_per_column; i++) {
        T8_ASSERT (element_is_valid (p[i]));
      }
    }
#endif

    element_2_5D **parent = (element_2_5D **) p;

    TUnderlyingEclassScheme1::element_get_parent ((t8_element_t *) &el->linear_element1,
                                                  (t8_element_t *) &parent[0]->linear_element1);
    TUnderlyingEclassScheme2::element_copy ((t8_element_t *) &el->linear_element2,
                                            (t8_element_t *) &parent[0]->linear_element2);

    for (int i = 1; i < num_elems_per_column; i++) {
      TUnderlyingEclassScheme1::element_get_parent (
        (t8_element_t *) &el->linear_element1,
        (t8_element_t *) &parent[i]->linear_element1);  //muss 4 mal berechnet werden
      TUnderlyingEclassScheme2::element_construct_successor ((t8_element_t *) &parent[i - 1]->linear_element2,
                                                             (t8_element_t *) &parent[i]->linear_element2);
    }
  }

  /** Compute the number of siblings of an element. That is the number of 
   * Children of its parent.
   * \param [in] elem The element.
   * \return          The number of siblings of \a element.
   * Note that this number is >= 1, since we count the element itself as a sibling.
   */
  inline int
  element_get_num_siblings (const t8_element_t *elem) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  inline int
  element_get_num_siblings (const t8_element_t *elem, int dir) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;
    if (dir == 1) {
      int num_siblings;
      int num_siblings1;
      int num_siblings2;
      int level2;
      num_siblings1 = TUnderlyingEclassScheme1::element_get_num_siblings ((t8_element_t *) &el->linear_element1);
      num_siblings2 = TUnderlyingEclassScheme2::element_get_num_siblings ((t8_element_t *) &el->linear_element2);
      level2 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &el->linear_element2);

      num_siblings = num_siblings1 * pow (num_siblings2, level2);

      return num_siblings;
    }
    else if (dir == 2) {
      return TUnderlyingEclassScheme2::element_get_num_siblings ((t8_element_t *) &el->linear_element2);
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** Compute a specific sibling of a given element \b elem and store it in \b sibling.
   *  \b sibling needs to be an existing element. No memory is allocated by this function.
   *  \b elem and \b sibling can point to the same element, then the entries of
   *  \b elem are overwritten by the ones of its sibid-th sibling.
   * \param [in] elem   The element whose sibling will be computed.
   * \param [in] sibid  The id of the sibling computed.
   * \param [in,out] sibling This element's entries will be overwritten by those
   *                    of \b elem's sibid-th sibling.
   *                    The storage for this element must exist
   *                    and match the element class of the sibling.
   */
  inline void
  element_get_sibling (const t8_element_t *elem, int sibid, t8_element_t *sibling) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Compute the number of corners of a given element.
   * \param [in] elem The element.
   * \return          The number of corners of \a elem.
   */
  inline int
  element_get_num_corners (const t8_element_t *elem) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;
    return TUnderlyingEclassScheme1::element_get_num_corners ((t8_element_t *) &el->linear_element1)
           * TUnderlyingEclassScheme2::element_get_num_corners ((t8_element_t *) &el->linear_element2);
  }

  /** Compute the number of faces of a given element.
   * \param [in] elem The element.
   * \return          The number of faces of \a elem.
   */
  inline int
  element_get_num_faces (const t8_element_t *elem) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0; /* suppresses compiler warning */
  }

  /** Compute the maximum number of faces of a given element and all of its
   *  descendants.
   * \param [in] elem The element.
   * \return          The maximum number of faces of \a elem and its descendants.
   */
  inline int
  element_get_max_num_faces (const t8_element_t *elem) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Return the number of children of an element when it is refined.
   * \param [in] elem   The element whose number of children is returned.
   * \return            The number of children of \a elem if it is to be refined.
   */

  inline int
  element_get_num_children ([[maybe_unused]] const t8_element_t *elem) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  inline int
  element_get_num_children (const t8_element_t *elem, int dir) const
  {
    const element_2_5D *el = (const element_2_5D *) elem;
    if (dir == 1) {
      int num_children;
      int num_children1;
      int num_children2;
      int level2;
      num_children1 = TUnderlyingEclassScheme1::element_get_num_children ((t8_element_t *) &el->linear_element1);
      num_children2 = TUnderlyingEclassScheme2::element_get_num_children ((t8_element_t *) &el->linear_element2);
      level2 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &el->linear_element2);

      num_children = num_children1 * pow (num_children2, level2);

      return num_children;
    }
    else if (dir == 2) {
      return TUnderlyingEclassScheme2::element_get_num_children ((t8_element_t *) &el->linear_element2);
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** Return the max number of children of an eclass.
   * \return            The max number of children of \a element.
   */
  inline int
  get_max_num_children () const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Return the number of children of an element's face when the element is refined.
   * \param [in] elem   The element whose face is considered.
   * \param [in] face   A face of \a elem.
   * \return            The number of children of \a face if \a elem is to be refined.
   */
  inline int
  element_get_num_face_children (const t8_element_t *elem, int face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Return the corner number of an element's face corner.
   * Example quad: 2 x --- x 3
   *                 |     |
   *                 |     |   face 1
   *               0 x --- x 1
   *      Thus for face = 1 the output is: corner=0 : 1, corner=1: 3
   *
   * \param [in] element  The element.
   * \param [in] face     A face index for \a element.
   * \param [in] corner   A corner index for the face 0 <= \a corner < num_face_corners.
   * \return              The corner number of the \a corner-th vertex of \a face.
   *
   * The order in which the corners must be given is determined by the eclass of \a element:
   * LINE/QUAD/TRIANGLE:  No specific order.
   * HEX               :  In Z-order of the face starting with the lowest corner number.
   * TET               :  Starting with the lowest corner number counterclockwise as seen from
   *                      'outside' of the element.
   */
  inline int
  element_get_face_corner (const t8_element_t *element, int face, int corner) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
    return 0;
  }

  /** Return the face numbers of the faces sharing an element's corner.
   * Example quad: 2 x --- x 3
   *                 |     |
   *                 |     |   face 1
   *               0 x --- x 1
   *                  face 2
   *      Thus for corner = 1 the output is: face=0 : 2, face=1: 1
   * \param [in] element  The element.
   * \param [in] corner   A corner index for the face.
   * \param [in] face     A face index for \a corner.
   * \return              The face number of the \a face-th face at \a corner.
   */
  inline int
  element_get_corner_face (const t8_element_t *element, int corner, int face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Construct the child element of a given number.
   * \param [in] elem     This must be a valid element, bigger than maxlevel.
   * \param [in] childid  The number of the child to construct.
   * \param [in,out] child        The storage for this element must exist
   *                              and match the element class of the child.
   *                              For a pyramid, for example, it may be either a
   *                              tetrahedron or a pyramid depending on \a childid.
   *                              This can be checked by \a t8_element_child_eclass.
   *                              On output, a valid element.
   * It is valid to call this function with elem = child.
   * \see t8_element_child_eclass
   */
  inline void
  element_get_child (const t8_element_t *elem, int childid, t8_element_t *child) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Construct all children of a given element.
   * \param [in] elem     This must be a valid element, bigger than maxlevel.
   * \param [in] length   The length of the output array \a c must match
   *                      the number of children.
   * \param [in,out] c    The storage for these \a length elements must exist.
   *                      On output, all children are valid.
   * It is valid to call this function with elem = c[0].
   * \see t8_element_num_children
   */

  inline void
  element_get_children (const t8_element_t *elem, int length, t8_element_t *c[]) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  inline void
  element_get_children (const t8_element_t *elem, int length, t8_element_t *c[], int dir) const
  {
    //*c[] = *[t8_element_t, t8_element_t, t8_element_t, ...]
    T8_ASSERT (element_is_valid (elem));
#ifdef T8_ENABLE_DEBUG
    {
      int i;
      for (i = 0; i < length; i++) {
        T8_ASSERT (element_is_valid (c[i]));
      }
    }
#endif

    const element_2_5D *el = (const element_2_5D *) elem;
    element_2_5D **children = (element_2_5D **) c;

    if (dir == 1) {
      int num_children1 = TUnderlyingEclassScheme1::element_get_num_children ((t8_element_t *) &el->linear_element1);
      t8_element_t **c1 = T8_ALLOC (t8_element_t *, num_children1);
      int num_children2 = TUnderlyingEclassScheme2::element_get_num_children ((t8_element_t *) &el->linear_element2);
      int level2 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &el->linear_element2);
      int num_elems_per_column = TUnderlyingEclassScheme2::count_leaves_from_root (level2);

      TUnderlyingEclassScheme1::element_new (num_children1, c1);
      TUnderlyingEclassScheme1::element_get_children ((t8_element_t *) &el->linear_element1, num_children1, c1);

      //only refined in dir1
      if (level2 == 0) {
        for (int i = 0; i < num_children1; i++) {
          TUnderlyingEclassScheme1::element_copy (c1[i], (t8_element_t *) &children[i]->linear_element1);
          TUnderlyingEclassScheme2::element_copy ((t8_element_t *) &el->linear_element2,
                                                  (t8_element_t *) &children[i]->linear_element2);
        }
      }
      else {

        // int num_elems_per_column = TUnderlyingEclassScheme2::count_leaves_from_root(level2);

        for (int i = 0; i < num_children1; i++) {
          TUnderlyingEclassScheme1::element_copy (
            c1[i], (t8_element_t *) &children[i * num_elems_per_column]->linear_element1);
          TUnderlyingEclassScheme2::element_copy (
            (t8_element_t *) &el->linear_element2,
            (t8_element_t *) &children[i * num_elems_per_column]->linear_element2);
          for (int j = 1; j < num_elems_per_column; j++) {
            int pos = i * num_elems_per_column + j;
            TUnderlyingEclassScheme1::element_copy (c1[i], (t8_element_t *) &children[pos]->linear_element1);
            TUnderlyingEclassScheme2::element_construct_successor ((t8_element_t *) &children[pos - 1]->linear_element2,
                                                                   (t8_element_t *) &children[pos]->linear_element2);
          }
        }
      }

      TUnderlyingEclassScheme1::element_destroy (num_children1, c1);

      T8_FREE (c1);
    }
    else if (dir == 2) {

      t8_element_t **c2 = T8_ALLOC (t8_element_t *, length);
      TUnderlyingEclassScheme2::element_new (length, c2);
      TUnderlyingEclassScheme2::element_get_children ((t8_element_t *) &el->linear_element2, length, c2);
      for (int i = 0; i < length; i++) {
        TUnderlyingEclassScheme1::element_copy ((t8_element_t *) &el->linear_element1,
                                                (t8_element_t *) &children[i]->linear_element1);
        TUnderlyingEclassScheme2::element_copy (c2[i], (t8_element_t *) &children[i]->linear_element2);
      }

      TUnderlyingEclassScheme2::element_destroy (length, c2);
      T8_FREE (c2);
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** Compute the child id of an element.
   * \param [in] elem     This must be a valid element.
   * \return              The child id of elem.
   */
  inline int
  element_get_child_id (const t8_element_t *elem) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  inline int
  element_get_child_id (const t8_element_t *elem, int dir) const
  {
    const element_2_5D *el = (const element_2_5D *) elem;
    if (dir == 1) {
      return TUnderlyingEclassScheme1::element_get_child_id ((t8_element_t *) &el->linear_element1);
    }
    else if (dir == 2) {
      return TUnderlyingEclassScheme2::element_get_child_id ((t8_element_t *) &el->linear_element2);
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** Compute the ancestor id of an element, that is the child id
   * at a given level.
   * \param [in] elem     This must be a valid element.
   * \param [in] level    A refinement level. Must satisfy \a level < elem.level
   * \return              The child_id of \a elem in regard to its \a level ancestor.
   */
  inline int
  element_get_ancestor_id (const t8_element_t *elem, int level) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
    // const element_2_5D *el = (const element_2_5D *) elem;
    // if (dir == 1) {
    //   return TUnderlyingEclassScheme1::element_get_ancestor_id ((t8_element_t *) &el->linear_element1, level);
    // }
    // else if (dir == 2) {
    //   return TUnderlyingEclassScheme2::element_get_ancestor_id ((t8_element_t *) &el->linear_element2, level);
    // }
    // else {
    //   SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    // }
  }

  /** Query whether a given set of elements is a family or not.
   * \param [in] fam      An array of as many elements as an element of class
   *                      \b ts has siblings.
   * \return              Zero if \b fam is not a family, nonzero if it is.
   * \note level 0 elements do not form a family.
   */
  inline int
  elements_are_family (t8_element_t *const *fam) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  /** Query whether a given set of elements is a family or not.
   * \param [in] fam      An array of as many elements as an element of class
   *                      \b ts has siblings.
   * \return              Zero if \b fam is not a family, nonzero if it is.
   * \note level 0 elements do not form a family.
   */
  inline int
  elements_are_family (t8_element_t *const *fam, int dir) const
  {
    int is_family;

    if (dir == 1) {
#ifdef T8_ENABLE_DEBUG
      {
        int i;
        for (i = 0; i < element_get_num_children (fam[0], 1); i++) {
          T8_ASSERT (element_is_valid (fam[i]));
        }
      }
#endif
      const element_2_5D *f0 = (const element_2_5D *) fam[0];

      /*general assumption: columns look alike 
      *-> horizontal refinement only done after uniform forest was constructed and before vertical refinement
      */
      int num_siblings1 = TUnderlyingEclassScheme1::element_get_num_siblings ((t8_element_t *) &f0->linear_element1);

      int level2_elem0 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &f0->linear_element2);
      int num_elems_dir2 = TUnderlyingEclassScheme2::count_leaves_from_root (level2_elem0);

      //holds due to assumption that in direction 2 uniform refined
      int elems_in_family = num_siblings1 * num_elems_dir2;

      int level1_elem0 = TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &f0->linear_element1);

      std::vector<int> levels_elem0 = { level1_elem0, level2_elem0 };
      int lin_id_elem1 = element_get_linear_id (fam[0], levels_elem0);

      const element_2_5D *f1 = (const element_2_5D *) fam[num_elems_dir2];

      int level1_elem1 = TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &f1->linear_element1);
      int level2_elem1 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &f1->linear_element2);
      std::vector<int> levels_elem1 = { level1_elem1, level2_elem1 };
      int lin_id_elem2 = element_get_linear_id (fam[num_elems_dir2], levels_elem1);

      if (lin_id_elem1 + num_elems_dir2 == lin_id_elem2 && level2_elem0 != 0
          && level2_elem1 != 0) {  //braucht man letzte && Bedingung? oder level2_elem0 == level2_elem1??
        t8_element_t **fam1;
        fam1 = T8_ALLOC (t8_element_t *, num_siblings1);
        int is_equal = 0;
        int is_equal_dir1 = 0;
        for (int i = 0; i < elems_in_family; i += num_elems_dir2) {
          const element_2_5D *elem = (const element_2_5D *) fam[i];
          int pos = i / num_elems_dir2;
          fam1[pos] = (t8_element_t *) &elem->linear_element1;
        }
        for (int i = 0; i < num_elems_dir2; i++) {
          is_equal_dir1 = 0;
          for (int j = 0; j < num_siblings1 - 1; j++) {
            const element_2_5D *elem = (const element_2_5D *) fam[i + j * num_elems_dir2];
            const element_2_5D *elem_comp = (const element_2_5D *) fam[i + (j + 1) * num_elems_dir2];
            if (TUnderlyingEclassScheme2::element_is_equal ((t8_element_t *) &elem->linear_element2,
                                                            (t8_element_t *) &elem_comp->linear_element2)) {
              is_equal_dir1 += 1;
            }
          }
          if (is_equal_dir1 == num_siblings1 - 1) {
            is_equal++;
          }
        }

        is_family = TUnderlyingEclassScheme1::elements_are_family (fam1) && (is_equal == num_elems_dir2);
        T8_FREE (fam1);
        return is_family;
      }
      else {
        return 0;
      }
    }
    else if (dir == 2) {
#ifdef T8_ENABLE_DEBUG
      {
        int i;
        for (i = 0; i < element_get_num_children (fam[0], 2); i++) {
          T8_ASSERT (element_is_valid (fam[i]));
        }
      }
#endif

      const element_2_5D *f0 = (const element_2_5D *) fam[0];
      const element_2_5D *f1 = (const element_2_5D *) fam[1];

      int num_siblings2 = TUnderlyingEclassScheme2::element_get_num_siblings ((t8_element_t *) &f0->linear_element2);

      int level1_elem0 = TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &f0->linear_element1);
      int level2_elem0 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &f0->linear_element2);
      std::vector<int> levels_elem0 = { level1_elem0, level2_elem0 };
      int lin_id_elem1 = element_get_linear_id (fam[0], levels_elem0);

      int level1_elem1 = TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &f1->linear_element1);
      int level2_elem1 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &f1->linear_element2);
      std::vector<int> levels_elem1 = { level1_elem1, level2_elem1 };
      int lin_id_elem2 = element_get_linear_id (fam[1], levels_elem1);
      //+1 as this is dependent direction and thus elements need to have a consecutive linear id
      if (lin_id_elem1 + 1 == lin_id_elem2 && level2_elem0 != 0 && level2_elem1 != 0) {
        t8_element **fam2;
        fam2 = T8_ALLOC (t8_element_t *, num_siblings2);
        int is_equal = 0;
        for (int i = 0; i < num_siblings2; i++) {
          const element_2_5D *elem = (const element_2_5D *) fam[i];
          fam2[i] = (t8_element_t *) &elem->linear_element2;
          if (i < num_siblings2 - 1) {
            const element_2_5D *elem_comp = (const element_2_5D *) fam[i + 1];
            if (TUnderlyingEclassScheme1::element_is_equal ((t8_element_t *) &elem->linear_element1,
                                                            (t8_element_t *) &elem_comp->linear_element1)) {
              is_equal += 1;
            }
          }
        }

        is_family = TUnderlyingEclassScheme2::elements_are_family (fam2) && (is_equal == (num_siblings2 - 1));
        T8_FREE (fam2);
        return is_family;
      }
      else {
        return 0;
      }
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** Compute the nearest common ancestor of two elements. That is,
   * the element with highest level that still has both given elements as
   * descendants.
   * \param [in] elem1    The first of the two input elements.
   * \param [in] elem2    The second of the two input elements.
   * \param [in,out] nca  The storage for this element must exist
   *                      and match the element class of the child.
   *                      On output the unique nearest common ancestor of
   *                      \b elem1 and \b elem2.
   */
  inline void
  element_get_nca (const t8_element_t *elem1, const t8_element_t *elem2, t8_element_t *nca) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Compute the shape of the face of an element.
   * \param [in] elem     The element.
   * \param [in] face     A face of \a elem.
   * \return              The element shape of the face.
   * I.e. T8_ECLASS_LINE for quads, T8_ECLASS_TRIANGLE for tets
   *      and depending on the face number either T8_ECLASS_QUAD or
   *      T8_ECLASS_TRIANGLE for prisms.
   */
  inline t8_element_shape_t
  element_get_face_shape (const t8_element_t *elem, int face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return T8_ECLASS_ZERO;
  }

  /** Given an element and a face of the element, compute all children of
   * the element that touch the face.
   * \param [in] elem     The element.
   * \param [in] face     A face of \a elem.
   * \param [in,out] children Allocated elements, in which the children of \a elem
   *                      that share a face with \a face are stored.
   *                      They will be stored in order of their linear id.
   * \param [in] num_children The number of elements in \a children. Must match
   *                      the number of children that touch \a face.
   *                      \ref t8_element_num_face_children
   * \param [in,out] child_indices If not NULL, an array of num_children integers must be given,
   *                      on output its i-th entry is the child_id of the i-th face_child.
   * It is valid to call this function with elem = children[0].
   */
  inline void
  element_get_children_at_face (const t8_element_t *elem, int face, t8_element_t *children[], int num_children,
                                int *child_indices) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
  }

  /** Given a face of an element and a child number of a child of that face, return the face number
   * of the child of the element that matches the child face.
   * \verbatim
      x ---- x   x      x           x ---- x
      |      |   |      |           |   |  | <-- f
      |      |   |      x           |   x--x
      |      |   |                  |      |
      x ---- x   x                  x ---- x
       elem    face  face_child    Returns the face number f
     \endverbatim

   * \param [in]  elem    The element.
   * \param [in]  face    Then number of the face.
   * \param [in]  face_child A number 0 <= \a face_child < num_face_children,
   *                      specifying a child of \a elem that shares a face with \a face.
   *                      These children are counted in linear order. This coincides with
   *                      the order of children from a call to \ref t8_element_children_at_face.
   * \return              The face number of the face of a child of \a elem
   *                      that coincides with \a face_child.
   */
  inline int
  element_face_get_child_face (const t8_element_t *elem, int face, int face_child) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet. Waiting for scheme interface.\n");
    return 0;
  }

  /** Given a face of an element return the face number
     * of the parent of the element that matches the element's face. Or return -1 if
     * no face of the parent matches the face.

     * \param [in]  elem    The element.
     * \param [in]  face    Then number of the face.
     * \return              If \a face of \a elem is also a face of \a elem's parent,
     *                      the face number of this face. Otherwise -1.
     * \note For the root element this function always returns \a face.
     */
  inline int
  element_face_get_parent_face (const t8_element_t *elem, int face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet. Waiting for scheme interface.\n");
    return 0;
  }

  /** Given an element and a face of this element. If the face lies on the
   *  tree boundary, return the face number of the tree face.
   *  If not the return value is arbitrary.
   *  You can call \ref t8_element_is_root_boundary to query whether the face is
   *  at the tree boundary.
   * \param [in] elem     The element.
   * \param [in] face     The index of a face of \a elem.
   * \return The index of the tree face that \a face is a subface of, if
   *         \a face is on a tree boundary.
   *         Any arbitrary integer if \a is not at a tree boundary.
   * \warning The return value may look like a valid face of the tree even if 
   *   the element does not lie on the root boundary.
   */
  inline int
  element_get_tree_face (const t8_element_t *elem, int face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Suppose we have two trees that share a common face f.
   *  Given an element e that is a subface of f in one of the trees
   *  and given the orientation of the tree connection, construct the face
   *  element of the respective tree neighbor that logically coincides with e
   *  but lies in the coordinate system of the neighbor tree.
   *  \param [in] elem1     The face element.
   *  \param [in,out] elem2 On return the face element \a elem1 with respective
   *                        to the coordinate system of the other tree.
   *  \param [in] orientation The orientation of the tree-tree connection.
   *                        \see t8_cmesh_set_join
   *  \param [in] sign      Depending on the topological orientation of the two tree faces,
   *                        either 0 (both faces have opposite orientation)
   *                        or 1 (both faces have the same top. orientattion).
   *                        \ref t8_eclass_face_orientation
   *  \param [in] is_smaller_face Flag to declare whether \a elem1 belongs to
   *                        the smaller face. A face f of tree T is smaller than
   *                        f' of T' if either the eclass of T is smaller or if
   *                        the classes are equal and f<f'. The orientation is
   *                        defined in relation to the smaller face.
   * \note \a elem1 and \a elem2 may point to the same element.
   */
  inline void
  element_transform_face (const t8_element_t *elem1, t8_element_t *elem2, int orientation, int sign,
                          int is_smaller_face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
  }

  /** Given a boundary face inside a root tree's face construct
   *  the element inside the root tree that has the given face as a
   *  face.
   * \param [in] face     A face element.
   * \param [in] face_scheme The scheme for the face element.
   * \param [in,out] elem An allocated element. The entries will be filled with
   *                      the data of the element that has \a face as a face and
   *                      lies within the root tree.
   * \param [in] root_face The index of the face of the root tree in which \a face
   *                      lies.
   * \return              The face number of the face of \a elem that coincides
   *                      with \a face.
   */
  inline int
  element_extrude_face (const t8_element_t *face, t8_element_t *elem, const int root_face,
                        const t8_scheme *scheme) const
  {
    SC_ABORT ("[FACE] Waiting for scheme interface.\n");
    return 0;
  }

  /** Construct the boundary element at a specific face.
   * \param [in] elem     The input element.
   * \param [in] face     The index of the face of which to construct the
   *                      boundary element.
   * \param [in,out] boundary An allocated element of dimension of \a element
   *                      minus 1. The entries will be filled with the entries
   *                      of the face of \a element.
   * \param [in] boundary_scheme The scheme for the eclass of the boundary face.
   * If \a elem is of class T8_ECLASS_VERTEX, then \a boundary must be NULL
   * and will not be modified.
   */
  inline void
  element_get_boundary_face (const t8_element_t *elem, int face, t8_element_t *boundary,
                             [[maybe_unused]] const t8_scheme *scheme) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
  }

  /** Construct the first descendant of an element at a given level that touches a given face.
   * \param [in] elem      The input element.
   * \param [in] face      A face of \a elem.
   * \param [in, out] first_desc An allocated element. This element's data will be
   *                       filled with the data of the first descendant of \a elem
   *                       that shares a face with \a face.
   * \param [in] level     The level, at which the first descendant is constructed
   */
  inline void
  element_get_first_descendant_face (const t8_element_t *elem, int face, t8_element_t *first_desc, int level) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet. & Waiting for scheme interface.\n");
  }

  /** Construct the last descendant of an element at a given level that touches a given face.
   * \param [in] elem      The input element.
   * \param [in] face      A face of \a elem.
   * \param [in, out] last_desc An allocated element. This element's data will be
   *                       filled with the data of the last descendant of \a elem
   *                       that shares a face with \a face.
   * \param [in] level     The level, at which the last descendant is constructed
   */
  inline void
  element_get_last_descendant_face (const t8_element_t *elem, int face, t8_element_t *last_desc, int level) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet. & Waiting for scheme interface.\n");
  }

  /** Compute whether a given element shares a given face with its root tree.
   * \param [in] elem     The input element.
   * \param [in] face     A face of \a elem.
   * \return              True if \a face is a subface of the element's root element.
   * \note You can compute the corresponding face number of the tree via \ref t8_element_tree_face.
   */
  inline int
  element_is_root_boundary (const t8_element_t *elem, int face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Construct the face neighbor of a given element if this face neighbor
   * is inside the root tree. Return 0 otherwise.
   * \param [in] elem The element to be considered.
   * \param [in,out] neigh If the face neighbor of \a elem along \a face is inside
   *                  the root tree, this element's data is filled with the
   *                  data of the face neighbor. Otherwise the data can be modified
   *                  arbitrarily.
   * \param [in] face The number of the face along which the neighbor should be
   *                  constructed.
   * \param [out] neigh_face The number of \a face as viewed from \a neigh.
   *                  An arbitrary value, if the neighbor is not inside the root tree.
   * \return          True if \a neigh is inside the root tree.
   *                  False if not. In this case \a neigh's data can be arbitrary
   *                  on output.
   */
  inline int
  element_get_face_neighbor_inside (const t8_element_t *elem, t8_element_t *neigh, int face, int *neigh_face) const
  {
    SC_ABORT ("[FACE] This function is not implemented yet.\n");
    return 0;
  }

  /** Return the shape of an allocated element according its type.
    *  For example, a child of an element can be an element of a different shape
    *  and has to be handled differently - according to its shape.
    *  \param [in] elem     The element to be considered
    *  \return              The shape of the element as an eclass
   */
  inline t8_element_shape_t
  element_get_shape (const t8_element_t *elem) const
  {
    element_2_5D *el = (element_2_5D *) elem;
    if (TUnderlyingEclassScheme1::element_get_shape ((t8_element_t *) &el->linear_element1) == T8_ECLASS_LINE
        && TUnderlyingEclassScheme2::element_get_shape ((t8_element_t *) &el->linear_element2) == T8_ECLASS_LINE) {
      return T8_ECLASS_QUAD;
    }
    else if (TUnderlyingEclassScheme1::element_get_shape ((t8_element_t *) &el->linear_element1) == T8_ECLASS_QUAD
             && TUnderlyingEclassScheme2::element_get_shape ((t8_element_t *) &el->linear_element2) == T8_ECLASS_LINE) {
      return T8_ECLASS_HEX;
    }
    else if (TUnderlyingEclassScheme1::element_get_shape ((t8_element_t *) &el->linear_element1) == T8_ECLASS_TRIANGLE
             && TUnderlyingEclassScheme2::element_get_shape ((t8_element_t *) &el->linear_element2) == T8_ECLASS_LINE) {
      return T8_ECLASS_PRISM;
    }
    else {
      t8_global_productionf ("Invalid combination of eclass_schemes: %i x %i",
                             TUnderlyingEclassScheme1::element_get_shape ((t8_element_t *) &el->linear_element1),
                             TUnderlyingEclassScheme2::element_get_shape ((t8_element_t *) &el->linear_element2));
      SC_ABORT ("Invalid combination of eclass_schemes.\n");
    }
  }

  /** Initialize the entries of an allocated element according to a
   *  given linear id in a uniform refinement.
   * \param [in,out] elem The element whose entries will be set.
   * \param [in] level    The level of the uniform refinement to consider.
   * \param [in] id       The linear id.
   *                      id must fulfil 0 <= id < 'number of leafs in the uniform refinement'
   */
  inline void
  element_set_linear_id (t8_element_t *elem, std::vector<int> &levels, t8_linearidx_t id) const
  {
    T8_ASSERT (element_is_valid (elem));
    element_2_5D *el = (element_2_5D *) elem;
    t8_linearidx_t id_scheme;
    std::vector<int> level1 = { levels[0] };
    std::vector<int> level2 = { levels[1] };

    int num_elems_per_column = TUnderlyingEclassScheme2::count_leaves_from_root (levels[1]);
    id_scheme = id / num_elems_per_column;
    TUnderlyingEclassScheme1::element_set_linear_id ((t8_element_t *) &el->linear_element1, level1, id_scheme);
    id_scheme = id % num_elems_per_column;
    TUnderlyingEclassScheme2::element_set_linear_id ((t8_element_t *) &el->linear_element2, level2, id_scheme);
  }

  /** Compute the linear id of a given element in a hypothetical uniform
   * refinement of a given level.
   * \param [in] elem     The element whose id we compute.
   * \param [in] level    The level of the uniform refinement to consider.
   * \return              The linear id of the element.
   */
  inline t8_linearidx_t
  element_get_linear_id (const t8_element_t *elem, std::vector<int> &levels) const
  {
    T8_ASSERT (element_is_valid (elem));
    const element_2_5D *el = (const element_2_5D *) elem;
    std::vector<int> level_vec1 = { levels[0] };
    std::vector<int> level_vec2 = { levels[1] };
    t8_linearidx_t lin_id;

    if (levels[1] == 0) {
      lin_id = TUnderlyingEclassScheme1::element_get_linear_id ((t8_element_t *) &el->linear_element1, level_vec1);
    }
    else {
      lin_id
        = TUnderlyingEclassScheme1::element_get_linear_id ((t8_element_t *) &el->linear_element1, level_vec1)
            * sc_intpow (TUnderlyingEclassScheme2::element_get_num_children ((t8_element_t *) &el->linear_element2),
                         levels[1])
          + TUnderlyingEclassScheme2::element_get_linear_id ((t8_element_t *) &el->linear_element2, level_vec2);
    }
    return lin_id;
  }

  /** Compute the first descendant of a given element.
   * \param [in] elem     The element whose descendant is computed.
   * \param [out] desc    The first element in a uniform refinement of \a elem
   *                      of the given level.
   * \param [in] level    The level, at which the descendant is computed.
   */
  inline void
  element_get_first_descendant (const t8_element_t *elem, t8_element_t *desc, std::vector<int> &levels) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;
    element_2_5D *d = (element_2_5D *) desc;
    std::vector<int> level1 = { levels[0] };
    std::vector<int> level2 = { levels[1] };

    TUnderlyingEclassScheme1::element_get_first_descendant ((t8_element_t *) &el->linear_element1,
                                                            (t8_element_t *) &d->linear_element1, level1);
    TUnderlyingEclassScheme2::element_get_first_descendant ((t8_element_t *) &el->linear_element2,
                                                            (t8_element_t *) &d->linear_element2, level2);
    T8_ASSERT (element_is_valid ((t8_element_t *) d));
  }

  /** Compute the last descendant of a given element.
   * \param [in] elem     The element whose descendant is computed.
   * \param [out] desc    The last element in a uniform refinement of \a elem
   *                      of the given level.
   * \param [in] level    The level, at which the descendant is computed.
   */
  inline void
  element_get_last_descendant (const t8_element_t *elem, t8_element_t *desc, std::vector<int> &levels) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;
    element_2_5D *d = (element_2_5D *) desc;
    std::vector<int> level2 = { levels[1] };

    int level1_max = get_maxlevel ();
    int level1 = TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &el->linear_element1);
    if (level1_max == level1) {
      TUnderlyingEclassScheme1::element_copy ((t8_element_t *) &el->linear_element1,
                                              (t8_element_t *) &d->linear_element1);
    }
    else {
      TUnderlyingEclassScheme1::element_copy ((t8_element_t *) &el->linear_element1,
                                              (t8_element_t *) &d->linear_element1);
      while (level1 < level1_max) {
        TUnderlyingEclassScheme1::element_get_child ((t8_element_t *) &d->linear_element1, 0,
                                                     (t8_element_t *) &d->linear_element1);
        level1 += 1;
      }
    }

    TUnderlyingEclassScheme2::element_get_last_descendant ((t8_element_t *) &el->linear_element2,
                                                           (t8_element_t *) &d->linear_element2, level2);
    T8_ASSERT (element_is_valid (desc));
  }

  /** Construct the successor in a uniform refinement of a given element.
  * \param [in] t    The element whose successor should be constructed.
  * \param [in,out] s  The element whose entries will be set.
  */
  inline void
  element_construct_successor (const t8_element_t *t, t8_element_t *s) const
  {
    T8_ASSERT (element_is_valid (t));

    const element_2_5D *tel = (const element_2_5D *) t;
    element_2_5D *sel = (element_2_5D *) s;

    int level1 = TUnderlyingEclassScheme1::element_get_level ((t8_element_t *) &tel->linear_element1);
    int level2 = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &tel->linear_element2);

    std::vector<int> levels = { level1, level2 };
    t8_linearidx_t lin_id = element_get_linear_id (t, levels);
    int num_elems_per_column = TUnderlyingEclassScheme2::count_leaves_from_root (levels[1]);

    if ((lin_id + 1) % num_elems_per_column != 0) {
      TUnderlyingEclassScheme1::element_copy ((const t8_element_t *) &tel->linear_element1,
                                              (t8_element_t *) &sel->linear_element1);
      TUnderlyingEclassScheme2::element_construct_successor ((const t8_element_t *) &tel->linear_element2,
                                                             (t8_element_t *) &sel->linear_element2);
    }
    else {
      TUnderlyingEclassScheme2::set_to_root ((t8_element_t *) &sel->linear_element2);
      int level_t = TUnderlyingEclassScheme2::element_get_level ((t8_element_t *) &tel->linear_element2);
      for (int i = 0; i < level_t; i++) {
        TUnderlyingEclassScheme2::element_get_child ((const t8_element_t *) &sel->linear_element2, 0,
                                                     (t8_element_t *) &sel->linear_element2);
      }
      TUnderlyingEclassScheme1::element_construct_successor ((const t8_element_t *) &tel->linear_element1,
                                                             (t8_element_t *) &sel->linear_element1);
    }

    T8_ASSERT (element_is_valid ((t8_element_t *) sel));
  }

  /** Compute the coordinates of a given element vertex inside a reference tree
   *  that is embedded into [0,1]^d (d = dimension).
   *   \param [in] t      The element to be considered.
   *   \param [in] vertex The id of the vertex whose coordinates shall be computed.
   *   \param [out] coords An array of at least as many doubles as the element's dimension
   *                      whose entries will be filled with the coordinates of \a vertex.
   */
  inline void
  element_get_vertex_reference_coords (const t8_element_t *t, const int vertex, double coords[]) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Convert a point in the reference space of an element to a point in the
   *  reference space of the tree.
   * 
   * \param [in] elem         The element.
   * \param [in] coords_input The coordinates of the point in the reference space of the element.
   * \param [in] user_data    User data.
   * \param [out] out_coords  The coordinates of the point in the reference space of the tree.
   */
  inline void
  element_get_reference_coords (const t8_element_t *elem, const double *ref_coords, const size_t num_coords,
                                double *out_coords) const
  {
    T8_ASSERT (element_is_valid (elem));
    const element_2_5D *el = (const element_2_5D *) elem;
    int dim1, dim2;
    dim1 = t8_eclass_to_dimension[TUnderlyingEclassScheme1::get_eclass ()];
    dim2 = t8_eclass_to_dimension[TUnderlyingEclassScheme2::get_eclass ()];
    for (size_t coord = 0; coord < num_coords; ++coord) {
      TUnderlyingEclassScheme1::element_get_reference_coords ((t8_element_t *) &el->linear_element1,
                                                              (ref_coords + (coord * (dim1 + dim2))), num_coords,
                                                              (out_coords + (coord * (dim1 + dim2))));
      TUnderlyingEclassScheme2::element_get_reference_coords ((t8_element_t *) &el->linear_element2,
                                                              ref_coords + (coord * (dim1 + dim2) + dim1), num_coords,
                                                              out_coords + (coord * (dim1 + dim2) + dim1));
    }
  }

  /** Count how many leaf descendants of a given uniform level an element would produce.
   * \param [in] t     The element to be checked.
   * \param [in] level A refinement level.
   * \return Suppose \a t is uniformly refined up to level \a level. The return value
   * is the resulting number of elements (of the given level).
   * If \a level < t8_element_level(t), the return value should be 0.
   *
   * Example: If \a t is a line element that refines into 2 line elements on each level,
   *  then the return value is max(0, 2^{\a level - level(\a t)}).
   *  Thus, if \a t's level is 0, and \a level = 3, the return value is 2^3 = 8.
   */
  inline t8_gloidx_t
  element_count_leaves (const t8_element_t *t, const int level) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Count how many leaf descendants of a given uniform level the root element will produce.
   * \param [in] level A refinement level.
   * \return The value of \ref t8_element_count_leaves if the input element
   *      is the root (level 0) element.
   *
   * This is a convenience function, and can be implemented via
   * \ref t8_element_count_leaves.
   */
  inline t8_gloidx_t
  count_leaves_from_root (const int level) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }

  /** Count how many leaf descendants of a given uniform level the root element will produce.
   * \param [in] level A refinement level.
   * \return The value of \ref t8_element_count_leaves if the input element
   *      is the root (level 0) element.
   *
   * This is a convenience function, and can be implemented via
   * \ref t8_element_count_leaves.
   */
  inline t8_gloidx_t
  count_leaves_from_root (const int level, int dir) const
  {
    if (dir == 1) {
      return TUnderlyingEclassScheme1::count_leaves_from_root (level);
    }
    else if (dir == 2) {
      return TUnderlyingEclassScheme2::count_leaves_from_root (level);
    }
    else {
      SC_ABORT ("Direction parameter to declare t8_eclass_scheme is missing.\n");
    }
  }

  /** This function has no defined effect but each implementation is free to
   *  provide its own meaning of it. Thus this function can be used to compute or
   *  lookup very scheme implementation specific data.
   *  \param [in] elem An valid element
   *  \param [in] indata Pointer to input data
   *  \param [out] outdata Pointer to output data.
   *  For the correct usage of \a indata and \a outdata see the specific implementations
   *  of the scheme.
   *  For example the default scheme triangle and tetrahedron implementations use 
   *  this function to return the type of a tri/tet to the caller.
   */
  inline void
  element_general_function (const t8_element_t *elem, const void *indata, void *outdata) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /**
   * Indicates if an element is refinable. Possible reasons for being not refinable could be
   * that the element has reached its max level.
   * \param [in] elem   The element to check.
   * \return            True if the element is refinable.
   */
  inline bool
  element_is_refinable (const t8_element_t *elem) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;

    return TUnderlyingEclassScheme1::element_is_refinable ((t8_element_t *) &el->linear_element1)
           && TUnderlyingEclassScheme2::element_is_refinable ((t8_element_t *) &el->linear_element2);
  }

#ifdef T8_ENABLE_DEBUG
  /** Query whether a given element can be considered as 'valid' and it is
   *  safe to perform any of the above algorithms on it.
   *  For example this could mean that all coordinates are in valid ranges
   *  and other membervariables do have meaningful values.
   * \param [in]      elem  The element to be checked.
   * \return          True if \a elem is safe to use. False otherwise.
   * \note            An element that is constructed with \ref t8_element_new
   *                  must pass this test.
   * \note            An element for which \ref t8_element_init was called must pass
   *                  this test.
   * \note            This function is used for debugging to catch certain errors.
   *                  These can for example occur when an element points to a region
   *                  of memory which should not be interpreted as an element.
   * \note            We recommend to use the assertion T8_ASSERT (t8_element_is_valid (elem))
   *                  in the implementation of each of the functions in this file.
   */
  inline int
  element_is_valid (const t8_element_t *elem) const
  {
    const element_2_5D *el = (const element_2_5D *) elem;

    return TUnderlyingEclassScheme1::element_is_valid ((const t8_element_t *) &el->linear_element1)
           && TUnderlyingEclassScheme2::element_is_valid ((const t8_element_t *) &el->linear_element2);
  }

  /**
 * Print a given element. For a example for a triangle print the coordinates
 * and the level of the triangle. This function is only available in the
 * debugging configuration. 
 * 
 * \param [in]        elem  The element to print
 */
  inline void
  element_debug_print (const t8_element_t *elem) const
  {
    T8_ASSERT (element_is_valid (elem));

    const element_2_5D *el = (const element_2_5D *) elem;
    TUnderlyingEclassScheme1::element_debug_print ((t8_element_t *) &el->linear_element1);
    TUnderlyingEclassScheme2::element_debug_print ((t8_element_t *) &el->linear_element2);
  }

  /**
 * \brief Fill a string with readable information about the element
 * 
 * \param[in] elem The element to translate into human-readable information
 * \param[in, out] debug_string The string to fill. 
 */
  inline void
  element_to_string (const t8_element_t *elem, char *debug_string, const int string_size) const
  {
    SC_ABORT ("Not implemented for 2.5 dimensional scheme.\n");
  }
#endif

  /** Allocate memory for an array of elements of a given class and initialize them.
   * \param [in] length   The number of elements to be allocated.
   * \param [in,out] elems On input an array of \b length many unallocated
   *                      element pointers.
   *                      On output all these pointers will point to an allocated
   *                      and initialized element.
   * \note Not every element that is created in t8code will be created by a call
   * to this function. However, if an element is not created using \ref t8_element_new,
   * then it is guaranteed that \ref t8_element_init is called on it.
   * \note In debugging mode, an element that was created with \ref t8_element_new
   * must pass \ref t8_element_is_valid.
   * \note If an element was created by \ref t8_element_new then \ref t8_element_init
   * may not be called for it. Thus, \ref t8_element_new should initialize an element
   * in the same way as a call to \ref t8_element_init would.
   * \see t8_element_init
   * \see t8_element_is_valid
   */
  /* TODO: would it be better to directly allocate an array of elements,
   *       not element pointers? */
  inline void
  element_new (int length, t8_element_t **elem) const
  {
    /* allocate memory */
    t8_2_5D_mempool_alloc ((sc_mempool_t *) this->scheme_2_5D_pool, length, elem);

/* in debug mode, set sensible default values. */
#if T8_ENABLE_DEBUG
    {
      for (int i = 0; i < length; i++) {
        element_2_5D *el = (element_2_5D *) elem[i];
        TUnderlyingEclassScheme1::element_init (1, (t8_element_t *) &el->linear_element1);
        TUnderlyingEclassScheme2::element_init (1, (t8_element_t *) &el->linear_element2);
      }
    }
#endif
  }

  /** Initialize an array of allocated elements.
   * \param [in] length   The number of elements to be initialized.
   * \param [in,out] elems On input an array of \b length many allocated
   *                       elements.
   * \note In debugging mode, an element that was passed to \ref t8_element_init
   * must pass \ref t8_element_is_valid.
   * \note If an element was created by \ref t8_element_new then \ref t8_element_init
   * may not be called for it. Thus, \ref t8_element_init should initialize an element
   * in the same way as a call to \ref t8_element_new would.
   * \note Every call to \ref t8_element_init must be matched by a call to \ref t8_element_deinit
   * \see t8_element_deinit
   * \see t8_element_new
   * \see t8_element_is_valid
   */
  inline void
  element_init (int length, t8_element_t *elem) const
  {
    element_2_5D *el = (element_2_5D *) elem;
    /* Set all values to 0 */

    for (int i = 0; i < length; i++) {
      element_2_5D *elem_init = el + i;
      TUnderlyingEclassScheme1::element_init (1, (t8_element_t *) &elem_init->linear_element1);
      TUnderlyingEclassScheme2::element_init (1, (t8_element_t *) &elem_init->linear_element2);
    }
  }

  /** Deinitialize an array of allocated elements.
   * \param [in] length   The number of elements to be deinitialized.
   * \param [in,out] elems On input an array of \b length many allocated
   *                       and initialized elements, on output an array of
   *                       \b length many allocated, but not initialized elements.
   * \note Call this function if you called t8_element_init on the element pointers.
   * \see t8_element_init
   */
  inline void
  element_deinit (int length, t8_element_t *elem) const
  {
    for (int i = 0; i < length; i++) {
      element_2_5D *el = (element_2_5D *) elem + i;

      TUnderlyingEclassScheme1::element_deinit (1, (t8_element_t *) &el->linear_element1);
      TUnderlyingEclassScheme2::element_deinit (1, (t8_element_t *) &el->linear_element2);
    }
  }

  /** Deallocate an array of elements.
   * \param [in] length   The number of elements in the array.
   * \param [in,out] elems On input an array of \b length many allocated
   *                      element pointers.
   *                      On output all these pointers will be freed.
   *                      \b elem itself will not be freed by this function.
   */
  inline void
  element_destroy (int length, t8_element_t **elem) const
  {
    T8_ASSERT (this->scheme_2_5D_pool != NULL);
    T8_ASSERT (0 <= length);
    T8_ASSERT (elem != NULL);

    element_2_5D **el = (element_2_5D **) elem;
    for (int i = 0; i < length; ++i) {
      TUnderlyingEclassScheme1::element_deinit (1, (t8_element_t *) &el[i]->linear_element1);
      TUnderlyingEclassScheme2::element_deinit (1, (t8_element_t *) &el[i]->linear_element2);
      sc_mempool_free ((sc_mempool_t *) scheme_2_5D_pool, elem[i]);
    }
  }

  /** create the root element
   * \param [in,out] elem The element that is filled with the root
   */
  inline void
  set_to_root (t8_element_t *elem) const
  {
    element_2_5D *el = (element_2_5D *) elem;
    TUnderlyingEclassScheme1::set_to_root ((t8_element_t *) &el->linear_element1);
    TUnderlyingEclassScheme2::set_to_root ((t8_element_t *) &el->linear_element2);
  }

  /** Pack multiple elements into contiguous memory, so they can be sent via MPI.
   * \param [in] elements Array of elements that are to be packed
   * \param [in] count Number of elements to pack
   * \param [in,out] send_buffer Buffer in which to pack the elements
   * \param [in] buffer_size size of the buffer (in order to check that we don't access out of range)
   * \param [in, out] position the position of the first byte that is not already packed
   * \param [in] comm MPI Communicator
  */
  inline void
  element_MPI_Pack (t8_element_t **const elements, const unsigned int count, void *send_buffer, int buffer_size,
                    int *position, sc_MPI_Comm comm) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Determine an upper bound for the size of the packed message of \b count elements
   * \param [in] count Number of elements to pack
   * \param [in] comm MPI Communicator
   * \param [out] pack_size upper bound on the message size
  */
  inline void
  element_MPI_Pack_size (const unsigned int count, sc_MPI_Comm comm, int *pack_size) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }

  /** Unpack multiple elements from contiguous memory that was received via MPI.
   * \param [in] recvbuf Buffer from which to unpack the elements
   * \param [in] buffer_size size of the buffer (in order to check that we don't access out of range)
   * \param [in, out] position the position of the first byte that is not already packed
   * \param [in] elements Array of initialised elements that is to be filled from the message
   * \param [in] count Number of elements to unpack
   * \param [in] comm MPI Communicator
  */
  inline void
  element_MPI_Unpack (void *recvbuf, const int buffer_size, int *position, t8_element_t **elements,
                      const unsigned int count, sc_MPI_Comm comm) const
  {
    SC_ABORT ("This function is not implemented yet.\n");
  }
};

#endif /* !T8_2_5DIMENSION_ELEMENT_CXX_HXX */
