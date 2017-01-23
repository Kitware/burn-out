/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_TRACK_STATE_ATTRIBUTES_H_
#define _VIDTK_TRACK_STATE_ATTRIBUTES_H_

#include <vxl_config.h>

#include <set>
#include <string>

namespace vidtk
{

// ----------------------------------------------------------------
/** Track state attributes.
 *
 * Track state attributes are either individual attributes that can be
 * freely combined with other attributes or part of a group attribute
 * where only one attribute in the group can be active. Groups and
 * individual attributes can be freely mixed though.
 *
 * Additional attributes and groups can be added, just follow the
 * existing implementation pattern. The MASK reserves the bits for the
 * group, and must be handled explicitly in the associated methods.
 *
 * The access methods preserve the semantics of the attribute groups.
 */
class track_state_attributes
{
public:
  typedef vxl_int_64 raw_attrs_t;
  typedef std::set< std::string > strings_t;

  enum state_attr_t
  {
    _ATTR_NONE                   = 0x0, ///< Not to be used in API
    // Only one of these can be active at a time
    // ForeGround tracking
    ATTR_ASSOC_FG_SSD            = 0x0001, ///< Sum-squared difference based template matching
    ATTR_ASSOC_FG_CSURF          = 0x0002, ///< First algo from U of Missouri
    ATTR_ASSOC_FG_LOFT           = 0x0003, ///< Second algo from U of Missouri
    ATTR_ASSOC_FG_NXCORR         = 0x0004, ///< Normalized cross-correlated based template matching
    ATTR_ASSOC_FG_NDI            = 0x0005, ///< (normalized) disjoint information
    ATTR_ASSOC_FG_avail          = 0x0006, ///< available values (6..f)
    ATTR_ASSOC_FG_GROUP          = 0x000f, ///< Used for testing any FG assoc

    // Data Association tracking
    ATTR_ASSOC_DA_KINEMATIC      = 0x0010, ///< Kinematic linker
    ATTR_ASSOC_DA_MULTI_FEATURES = 0x0020, ///< Multi-feature linker
    ATTR_ASSOC_avail             = 0x0030, ///< available values (3..f)
    ATTR_ASSOC_DA_GROUP          = 0x00f0, ///< Used for testing any DA assoc
    _ATTR_ASSOC_MASK             = 0x00ff, ///< Not to be used in API

    // Only one of these can be active at a time
    ATTR_KALMAN_ESH              = 0x00100, ///< Extended Kalman filter with position, speed and heading state
    ATTR_KALMAN_LVEL             = 0x00200, ///< Linear Kalman filter with position and velocity
    ATTR_KALMAN_avail            = 0x00300, ///< available values (3..f)
    _ATTR_KALMAN_MASK            = 0x00f00, ///< Not to be used in API

    // Only one of these can be active at a time
    ATTR_INTERVAL_FORWARD        = 0x001000, ///< Set during forward tracking that happens after
                                             ///< both track initialization and back tracking
    ATTR_INTERVAL_BACK           = 0x002000, ///< Set during back tracking that happens after
                                             ///< track initialization but before forward tracking
    ATTR_INTERVAL_INIT           = 0x004000, ///< Set during track initialization interval
    _ATTR_INTERVAL_MASK          = 0x007000, ///< Not to be used in API

    // Linking set of attributes
    ATTR_LINKING_START           = 0x0010000, ///< Start of linked track (first state in track)
    ATTR_LINKING_END             = 0x0020000, ///< End of linked track (last state in track)
    _ATTR_LINKING_MASK           = 0x0030000, ///< Not to be used in API

    /// Track scoring attributes: populated by scoring module for forensics
    /// Did this state match a state from the dominant match at the track level?
    ATTR_SCORING_DOMINANT        = 0x100000,
    /// Did this state match a state from a track that is not the dominant
    /// match for this track?
    ATTR_SCORING_NONDOMINANT     = 0x200000,
    /// Is the state source ground-truth (bit is set) or computed (bit is cleared)?
    ATTR_SCORING_SRC_IS_TRUTH    = 0x400000,
    /// Did the state get matched during scoring?  (If cleared, i.e. not-matched,
    /// this may be due to other reasons not enumerated yet: filtered-by-AOI,
    /// filtered-by-time-window, dropped-due-to-frame-rate-mismatch, straight-up
    /// false alarm, etc. etc.
    ATTR_SCORING_STATE_MATCHED   = 0x800000
  };

  track_state_attributes (track_state_attributes::raw_attrs_t attrs = 0);

  /** \brief Set attribute.
   *
   * This method sets the specified attribute into the attribute
   * field. Note that some attributes are in groups and only one
   * attribute in the group can be active.
   *
   * Even though you may have guessed that these attributes are
   * implemented as bit in a larger word, you can only pass a single
   * attribute to this method at a time. If you need to set multiple
   * attributes, you need to make multiple calls.
   *
   * @param attr - attribute symbol to set.
   */
  void set_attr (state_attr_t attr);

  /** \brief Set attributes.
   *
   * This method sets the entire attribute field at once.
   * Its primary use is when reading them from a file or database.
   * @param attribute - field to set.
   */
  void set_attrs (raw_attrs_t attr);

  /** \brief Set attributes.
   *
   * This method sets the entire attribute field at once. It is
   * equivalent to clearing all attributes, then calling set_attr()
   * for each attribute in the set of strings that can successfully be
   * converted to a state_attr_t by from_string().
   * @param attr - attributes represented as conanical strings.
   */
  void set_attrs (strings_t const& attr);

  /** \brief Get attributes.
   *
   * This method returns the entire attribute bitfield.
   */
  raw_attrs_t get_attrs() const;

  /** \brief Convenience conversion operator.
   *
   * This allows attributes to be implicitly converted to raw_attrs_t.
   */
  operator raw_attrs_t() const { return this->get_attrs(); }

  /** \brief Get attributes as set of strings.
   *
   * This method returns a std::set of strings where each string represents a
   * single attribute.
   */
  strings_t get_attrs_strings() const;

  /** \brief Clear attribute.
   *
   * This method clears the specified attribute from the attribute
   * field.  Note that some attributes are in groups so if one of those
   * is passed, the whole group is cleared.
   *
   * Even though you may have guessed that these attributes are
   * implemented as bit in a larger word, you can only pass a single
   * attribute to this method at a time. If you need to clear multiple
   * attributes, you need to make multiple calls.
   *
   * @param attr - attribute symbol to clear.
   */
  void clear_attr (state_attr_t attr);

  /** \brief Is attribute set.
   *
   * This method tests to see if the sepcified attribute symbol is
   * active in the attribute set.
   *
   * Even though you may have guessed that these attributes are
   * implemented as bit in a larger word, you can only pass a single
   * attribute to this method at a time. If you need to test multiple
   * attributes, you need to make multiple calls.
   *
   * @param attr - attribute to test.
   */
  bool has_attr (state_attr_t attr) const;

  /** \brief Get name for attributes.
   *
   * This method returns a string that contains the name or names of
   * the attributes contained
   */
  std::string get_attr_text () const;

  /** \brief Get machine-readable name for attribute.
   *
   * This method returns a string representation of the specified attribute
   * that is suitable for encodings that are meant to be read in software (i.e.
   * a mnemonic symbol rather than an English phrase). It is the inverse of
   * from_string().
   */
  static std::string to_string (state_attr_t);

  /** \brief Get attribute from machine-readable name.
   *
   * This method returns an attribute from its symbolic name, or _ATTR_NONE if
   * the name is not recognized. It is the inverse of to_string().
   */
  static state_attr_t from_string (std::string const&);

private:
  raw_attrs_t attributes_;


}; // end class track_state_attributes


}  // end namespace

#endif /* _VIDTK_TRACK_STATE_ATTRIBUTES_H_ */
