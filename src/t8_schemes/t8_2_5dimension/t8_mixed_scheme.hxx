/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2025 the developers

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

#ifndef T8_MIXED_SCHEMES_HXX
#define T8_MIXED_SCHEMES_HXX

#include <variant>
#include <memory>
#include <t8_schemes/t8_scheme.hxx>
#include <t8_schemes/t8_scheme_builder.hxx>
#include <t8_schemes/t8_2_5dimension/t8_2_5D.hxx>
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension.hxx>
#include <t8_schemes/t8_2_5dimension/t8_2_5dimension_element.hxx>

class line_class1;
class line_class2;

class invalid_scheme;

class t8_mixed_scheme: public t8_scheme {
  friend class t8_mixed_scheme_builder;

 public:
  t8_mixed_scheme ()
  {
    t8_refcount_init (&rc_mixed);
  };

  ~t8_mixed_scheme ()
  {
    if (sc_refcount_is_active (&rc_mixed)) {
      T8_ASSERT (t8_refcount_is_last (&rc_mixed));
      t8_refcount_unref (&rc_mixed);
    }
    t8_debugf ("Deleted the mixed scheme.\n");
    if (t8_scheme::rc.refcount > 1) {
      t8_scheme::unref ();
    }
  };
  /* clang-format off */

  /** Variant to hold an eclass scheme. */
  using scheme_mixed_var = std::variant<
    /* 2.5D scheme */
    invalid_scheme, 
    t8_2_5dimension_scheme<line_class1, t8_dline_t, line_class2, t8_dline_t>,
    t8_2_5dimension_scheme<t8_default_scheme_quad, t8_pquad_t, t8_default_scheme_line, t8_dline_t>,
    t8_2_5dimension_scheme<t8_default_scheme_tri, t8_dtri_t, t8_default_scheme_line, t8_dline_t>>;
  /* clang-format on */

  using scheme_mixed_container = std::vector<scheme_mixed_var>; /**< Container type for holding eclass schemes. */

 private:
  scheme_mixed_container eclass_schemes_mixed; /**< The container holding the eclass schemes. */

  mutable t8_refcount_t
    rc_mixed; /**< The reference count of the scheme. Mutable so that the class can be const and the ref counter is still mutable. TODO: Replace by shared_ptr when forest becomes a class. */

 public:
  /**
   * Increase the reference count of the scheme.
   */
  inline void
  ref_mixed () const
  {
    t8_refcount_ref (&rc_mixed);
  }

  /**
   * Decrease the reference count of the scheme.
   * If the reference count reaches zero, the scheme is deleted.
   * \return The remaining reference count. If 0 the scheme was deleted.
   */
  inline int
  unref_mixed () const
  {
    const int remaining = rc_mixed.refcount - 1;
    if (t8_refcount_unref (&rc_mixed)) {
      t8_debugf ("Deleting the mixed scheme.\n");
      delete this;
    }
    return remaining;
  }

  /** Return the level of a particular element.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem    The element whose level should be returned.
   * \return             The level of \a elem.
   */
  inline int
  element_get_level (const t8_eclass_t tree_class, const t8_element_t *elem, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_level (elem, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Compute the parent of a given element \a elem and store it in \a parent.
   *  \a parent needs to be an existing element. No memory is allocated by this function.
   *  \a elem and \a parent can point to the same element, then the entries of
   *  \a elem are overwritten by the ones of its parent.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem   The element whose parent will be computed.
   * \param [in,out] parent This element's entries will be overwritten by those
   *                    of \a elem's parent.
   *                    The storage for this element must exist
   *                    and match the element class of the parent.
   *                    For a pyramid, for example, it may be either a
   *                    tetrahedron or a pyramid depending on \a elem's childid.
   */
  inline void
  element_get_parent (const t8_eclass_t tree_class, const t8_element_t *elem, t8_element_t *parent, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_parent (elem, parent, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /*
  *TODO
  */
  inline void
  element_get_parent_2_5D (const t8_eclass_t tree_class, const t8_element_t *elem, t8_element_t *p[]) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_parent_2_5D (elem, p); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Compute the number of siblings of an element. That is the number of 
   * Children of its parent.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem The element.
   * \return          The number of siblings of \a element.
   * Note that this number is >= 1, since we count the element itself as a sibling.
   */
  inline int
  element_get_num_siblings (const t8_eclass_t tree_class, const t8_element_t *elem, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_num_siblings (elem, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Return the number of children of an element when it is refined.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem   The element whose number of children is returned.
   * \return            The number of children of \a elem if it is to be refined.
   */
  inline int
  element_get_num_children (const t8_eclass_t tree_class, const t8_element_t *elem, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_num_children (elem, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Construct all children of a given element.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem     This must be a valid element, bigger than maxlevel.
   * \param [in] length   The length of the output array \a c must match
   *                      the number of children.
   * \param [in,out] c    The storage for these \a length elements must exist.
   *                      On output, all children are valid.
   * It is valid to call this function with elem = c[0].
   * \see t8_element_num_children
   */
  inline void
  element_get_children (const t8_eclass_t tree_class, const t8_element_t *elem, const int length, t8_element_t *c[],
                        int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_children (elem, length, c, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Compute the child id of an element.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem     This must be a valid element.
   * \return              The child id of elem.
   */
  inline int
  element_get_child_id (const t8_eclass_t tree_class, const t8_element_t *elem, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_child_id (elem, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Query whether a given set of elements is a family or not.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] fam      An array of as many elements as an element of class
   *                      \a tree_class has siblings.
   * \return              Zero if \a fam is not a family, nonzero if it is.
   * \note level 0 elements do not form a family.
   */
  inline bool
  elements_are_family (const t8_eclass_t tree_class, t8_element_t *const *fam, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.elements_are_family (fam, dir); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Initialize the entries of an allocated element according to a
   * given linear id in a uniform refinement.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in,out] elem The element whose entries will be set.
   * \param [in] level    The level of the uniform refinement to consider.
   * \param [in] id       The linear id.
   *                      id must fulfil 0 <= id < 'number of leaves in the uniform refinement'
   */
  inline void
  element_set_linear_id (const t8_eclass_t tree_class, t8_element_t *elem, std::vector<int> &levels,
                         const t8_linearidx_t id) const  //const std::vector<int>& levelsgeht nicht @TODO
  {
    // const std::vector<int> levels = {level};

    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_set_linear_id (elem, levels, id); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Compute the linear id of a given element in a hypothetical uniform
   * refinement of a given level.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem     The element whose id we compute.
   * \param [in] level    The level of the uniform refinement to consider.
   * \return              The linear id of the element.
   */
  inline t8_linearidx_t
  element_get_linear_id (const t8_eclass_t tree_class, const t8_element_t *elem,
                         std::vector<int> &levels) const  //const std::vector<int>& levelsgeht nicht @TODO
  {
    // const std::vector<int> levels = {level};

    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.element_get_linear_id (elem, levels); },
                       eclass_schemes_mixed[tree_class]);
  };

  /** Compute the first descendant of a given element.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem     The element whose descendant is computed.
   * \param [out] desc    The first element in a uniform refinement of \a elem
   *                      of the given level.
   * \param [in] level    The level, at which the descendant is computed.
   */
  inline void
  element_get_first_descendant (const t8_eclass_t tree_class, const t8_element_t *elem, t8_element_t *desc,
                                std::vector<int> levels) const
  {
    return std::visit (
      [&] (auto &&scheme_mixed) { return scheme_mixed.element_get_first_descendant (elem, desc, levels); },
      eclass_schemes_mixed[tree_class]);
  };

  /** Compute the last descendant of a given element.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] elem     The element whose descendant is computed.
   * \param [out] desc    The last element in a uniform refinement of \a elem
   *                      of the given level.
   * \param [in] level    The level, at which the descendant is computed.
   */
  inline void
  element_get_last_descendant (const t8_eclass_t tree_class, const t8_element_t *elem, t8_element_t *desc,
                               std::vector<int> levels) const
  {
    return std::visit (
      [&] (auto &&scheme_mixed) { return scheme_mixed.element_get_last_descendant (elem, desc, levels); },
      eclass_schemes_mixed[tree_class]);
  };

  /** Count how many leaf descendants of a given uniform level the root element will produce.
   * \param [in] tree_class    The eclass of the current tree.
   * \param [in] level A refinement level.
   * \return The value of \ref t8_element_count_leaves if the input element
   *      is the root (level 0) element.
   *
   * This is a convenience function, and can be implemented via
   * \ref t8_element_count_leaves.
   */
  inline t8_gloidx_t
  count_leaves_from_root (const t8_eclass_t tree_class, const int level, int dir) const
  {
    return std::visit ([&] (auto &&scheme_mixed) { return scheme_mixed.count_leaves_from_root (level, dir); },
                       eclass_schemes_mixed[tree_class]);
  };
};

/** The mixed scheme builder adds a combination of two eclass schemes to a scheme container and returns it.
 * TODO: Make return value a reference.
 */
class t8_mixed_scheme_builder {  //}: public t8_scheme_builder {
 public:
  t8_mixed_scheme_builder (): scheme_mixed (new t8_mixed_scheme) {};
  ~t8_mixed_scheme_builder () {};

  using scheme_mixed_var = t8_mixed_scheme::scheme_mixed_var;

  /** Add a new element class scheme to the scheme.
   * \tparam TEclassScheme       The type of the element class scheme.
   * \tparam Args                 The types of the arguments to pass to the constructor of the element class scheme.
   * \param  [in] args            The arguments to pass to the constructor of the element class scheme.
   * \return                      The position of the added element class scheme in the scheme.
*/
  template <typename TEclassScheme, typename... _Args>
  size_t
  add_eclass_scheme_mixed (_Args &&...args)
  {
#if T8_ENABLE_DEBUG
    t8_debugf ("Registering scheme of type %s with position %li.\n", t8_debug_print_type<TEclassScheme> ().c_str (),
               scheme_mixed->eclass_schemes_mixed.size ());
#endif  // T8_ENABLE_DEBUG
    // scheme->eclass_schemes_mixed.emplace_back (eclass);
    scheme_mixed->eclass_schemes_mixed.emplace_back (std::in_place_type<TEclassScheme>, std::forward<_Args> (args)...);
    scheme_mixed->eclass_schemes.emplace_back (std::in_place_type<TEclassScheme>, std::forward<_Args> (args)...);
    return scheme_mixed->eclass_schemes_mixed.size ();
  }

  /** Build the scheme.
   * \return The built scheme.
   */
  const t8_scheme *
  // const t8_mixed_scheme *
  build_mixed_scheme () const
  {
    return (t8_scheme *) scheme_mixed;
    // return scheme;
  }

 private:
  t8_mixed_scheme *scheme_mixed;
};

#endif /* !T8_MIXED_SCHEMES_HXX */
