/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_TRACK_ATTRIBUTES_H_
#define _VIDTK_TRACK_ATTRIBUTES_H_

#include <vxl_config.h>

#include <string>

namespace vidtk
{

/** Track attributes.
 *
 * Track attributes are either individual attributes that can be
 * freely combined with other attributes or part of a group attribute
 * where only one attribute in the group can be active. Groups and
 * individual attributes can be freely mixed though.
 *
 * Additional attributes and groups can be added, just follow the
 * existing implementation pattern. The MASK reserves the bits for the
 * group, and must be handled explicitly in the associated methods.
 */
class track_attributes
{
public:
  typedef vxl_int_64 raw_attrs_t;

  enum track_attr_t
  {
    // bits 0x0000000f are available

    // Single bit attributes

    // Is Aligned Motion History Image (AMHI) model produced for the track?
    ATTR_AMHI = 0x0010,

    // Track scoring attributes: populated by scoring module for forensics
    // Is this a truth or computed (default) track?
    ATTR_SCORING_TRUTH   = 0x0020,

    // Did this track match another one during track scoring?
    // This provides unique information for the truth and computed
    // tracks.
    ATTR_SCORING_MATCHED = 0x0030
  };

  track_attributes();

  /** Set attribute.
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
   * @param[in] attr - attribute symbol to set.
   */
  void set_attr (track_attr_t attr);

  /** Set attributes.
   *
   * This method sets the entire attribute field at once.
   * Its primary use is when reading them from a file or database.
   * @param[in] attribute field to set.
   */
  void set_attrs (raw_attrs_t attribute);

  /** Get attributes.
   *
   * This method returns the entire attribute bitfield.
   *
   */
  raw_attrs_t get_attrs() const;

  /** Clear attribute.
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
   * @param[in] attr - attribute symbol to clear.
   */
  void clear_attr (track_attr_t attr);


  /** Is attribute set.
   *
   * This method tests to see if the sepcified attribute symbol is
   * active in the attribute set.
   *
   * Even though you may have guessed that these attributes are
   * implemented as bit in a larger word, you can only pass a single
   * attribute to this method at a time. If you need to test multiple
   * attributes, you need to make multiple calls.
   *
   * @param[in] attr - attribute to test.
   */
  bool has_attr (track_attr_t attr) const;


  /** Get name for attributes.
   *
   * This method returns a string that conatiains the name or names of
   * the attributes contained
   */
  std::string get_attr_text () const;


private:
  raw_attrs_t attributes_; ///< attributes for this track
};

} // end namespace

#endif /* _VIDTK_TRACK_ATTRIBUTES_H_ */
