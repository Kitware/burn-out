
/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
\file
\brief
Method and field definitions for a track state.
*/


#ifndef INCL_TRACK_STATE_H
#define INCL_TRACK_STATE_H

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <utilities/property_map.h>
#include <utilities/timestamp.h>
#include <vnl/vnl_double_2x2.h>
#include <vgl/vgl_box_2d.h>
#include <tracking/image_object.h>
#include <utilities/homography.h>


namespace vidtk
{

struct track_state;
///Creates name for type vbl_smart_ptr<track_state>
typedef vbl_smart_ptr<track_state> track_state_sptr;

///Structure containing information on a track's current state.
struct track_state
  : public vbl_ref_count
{
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
 */
  enum state_attr_t
  {
    // Only one of these can be active at a time
    // ForeGround tracking
    ATTR_ASSOC_FG_SSD            = 0x0001,
    ATTR_ASSOC_FG_CSURF          = 0x0002,
    ATTR_ASSOC_FG_avail          = 0x0003, // available values (3..f)
    ATTR_ASSOC_FG_GROUP          = 0x000f, // Used for testing any FG assoc

    // Data Association tracking
    ATTR_ASSOC_DA_KINEMATIC      = 0x0010,
    ATTR_ASSOC_DA_MULTI_FEATURES = 0x0020,
    ATTR_ASSOC_avail             = 0x0030, // available values (3..f)
    ATTR_ASSOC_DA_GROUP          = 0x00f0, // Used for testing any DA assoc
    _ATTR_ASSOC_MASK             = 0x00ff, // Not to be used in API


    // Only one of these can be active at a time
    ATTR_KALMAN_ESH              = 0x00100, ///< Extended Kalman filter with position, speed and heading state
    ATTR_KALMAN_LVEL             = 0x00200, ///< Linear Kalman filter with position and velocity
    ATTR_KALMAN_avail            = 0x00300, // available values (3..f)
    _ATTR_KALMAN_MASK            = 0x00f00, // Not to be used in API


    // Only one of these can be active at a time
    ATTR_INTERVAL_FORWARD        = 0x001000,
    ATTR_INTERVAL_BACK           = 0x002000,
    ATTR_INTERVAL_INIT           = 0x004000,
    _ATTR_INTERVAL_MASK          = 0x007000, // Not to be used in API


    // Linking set of attributes
    ATTR_LINKING_START           = 0x0010000, ///< Start of linked track (first state in track)
    ATTR_LINKING_END             = 0x0020000, ///< End of linked track (last state in track)
    _ATTR_LINKING_MASK           = 0x0030000, // Not to be used in API
  };


  ///Constructor.
track_state()
  ///Track's location.
  : loc_(),
    ///Track's velocity.
    vel_(),
    ///Track's image velocity.
    img_vel_(0,0),
    ///Track's time
    time_(),
    ///Aligned motion history image bounding box
    amhi_bbox_(),
    ///Track data
    data_(),
    attributes_(0)
    {}

  ///Destructor
  virtual ~track_state() { }
  ///Vector of locations
  vnl_vector_fixed<double,3> loc_;
  ///Vector of velocities
  vnl_vector_fixed<double,3> vel_;
  ///Vector of velocities in image
  vnl_vector_fixed<double,2> img_vel_;
  ///Track timestamp
  timestamp time_;
  /// Aligned motion history image bounding box.
  vgl_box_2d<unsigned> amhi_bbox_;
  ///Property map of data
  property_map data_;

  /// \brief Create a deep copy clone.
  ///
  /// This function calls the default copy constructor following by
  /// specialized deep copy operations for the individual fields.
  track_state_sptr clone();

  /// Store the supplied latitude and longitude in track_state.
  void set_latitude_longitude( double latitude, double longitude );

  /// \brief Use the stored world location to store corresponding lat/long
  ///
  /// A transformation from the coordinates of (tracking) world location
  /// (track_state::loc_) to UTM coordinates will be used to find the
  /// lat/long points, which will be stored in the data map.
  void set_latitude_longitude( plane_to_utm_homography const& H_wld2utm );

  /// \brief Retrieve latitude and longitude from track_state.
  ///
  /// Returns true if a latitude and longitude are available;
  /// false otherwise. If you need UTM or MGRS versions of the
  /// geographic data, then one can use vidtk::geographic::geo_coords
  /// for storage and conversions.
  bool latitude_longitude( double & latitude, double & longitude ) const;

  /// Store the supplied Ground Scale Distance (GSD) in track_state.
  void set_gsd( float gsd );

  /// \brief Retrieve Ground Scale Distance (GSD) from track_state.
  ///
  /// Returns true if a GSD is available; false otherwise.
  bool gsd( float & gsd ) const;

  /// Store the supplied image_object (MOD) in track_state.
  void set_image_object( image_object_sptr & obj );

  /// \brief Retrieve image_object (MOD) from track_state.
  ///
  /// Returns true if a image_object (MOD) is available;
  /// false otherwise.
  bool image_object( image_object_sptr & image_object ) const;

  /// \brief Store the supplied bounding bounding box in track_state.
  ///
  /// Note that bounding box is stored inside image_object (MOD).
  /// Returns true if an MOD is found and box is set; false otherwise.
  bool set_bbox( vgl_box_2d<unsigned> const & box );

  /// \brief Retrieve bounding box from track_state.
  ///
  /// Returns true if a image_object (MOD) containing the bbox
  /// is available; false otherwise.
  bool bbox( vgl_box_2d<unsigned> & box ) const;

  /// \brief Store the supplied location covariance in track_state.
  ///
  /// Covariance of the loc_ field.
  void set_location_covariance( vnl_double_2x2 const & cov );

  /// \brief Retrieve location covariance from track_state.
  ///
  /// Returns true if a location covariance is available; false otherwise.
  bool location_covariance( vnl_double_2x2 & cov ) const;


  bool add_img_vel(vgl_h_matrix_2d<double> const& wld2img_H);
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
  void set_attr (state_attr_t attr);


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
  void clear_attr (state_attr_t attr);


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
  bool has_attr (state_attr_t attr);


private:
  vxl_int_64 attributes_; ///< attributes for this track state

};


// return true if the timestamp matches.
struct track_state_ts_pred
{
  track_state_ts_pred( timestamp const& ts)
  {
    ts_ = ts;
  }

  bool operator()(track_state_sptr const& t)
  {
    return (t->time_ == ts_);
  }

private:
  timestamp ts_;
};

} // namespace vidtk

#endif
