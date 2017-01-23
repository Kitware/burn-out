/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
\file
\brief
Method and field definitions for a track state.
*/

#ifndef vidtk_track_state_h_
#define vidtk_track_state_h_

#include <tracking_data/image_object.h>
#include <tracking_data/track_state_attributes.h>

#include <utilities/property_map.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/geo_UTM.h>

#include <vnl/vnl_double_2x2.h>
#include <vgl/vgl_box_2d.h>

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

#include <boost/optional.hpp>

namespace vidtk
{

namespace geo_coord {
  class geo_lat_lon;
}

struct track_state;

/// Creates name for type vbl_smart_ptr<track_state>
typedef vbl_smart_ptr<track_state> track_state_sptr;

// ----------------------------------------------------------------
/** \brief Structure containing information on a track's current state.
 *
 * @todo Convert to a class. Make data elements inaccessible. Add
 * accessors.
 */
struct track_state
  : public vbl_ref_count
{
  // -- TYPES --
  typedef std::vector< track_state_sptr > vector_t;

  /// Constructor.
  track_state()
  /// Track's location.
  : loc_(0,0,0),
    /// Track's velocity.
    vel_(0,0,0),
    /// Track's world velocity mapped to image coordinates.
    img_vel_(0,0),
    /// Pixel metrics have not been computed.
    pixel_metrics_computed_(false),
    /// Track's time.
    time_(),
    /// Aligned motion history image bounding box.
    amhi_bbox_(),
    /// Track data.
    data_(),
    /// State's geo_coordinate, lat_lon, UTM, etc
    geo_coord_( new geo_coord::geo_coordinate() )
  {
    /// Initialize the cov_matrix to identity
    cov_matrix_.set_identity();
  }


  /// Destructor
  virtual ~track_state() {}

  /// World location vector
  vnl_vector_fixed<double,3> loc_; // world location

  /// World velocity vector
  vnl_vector_fixed<double,3> vel_;

  /// World velocity mapped to image coordinates vector
  vnl_vector_fixed<double,2> img_vel_;

  /// Were image level measurements computed for this track state?
  bool pixel_metrics_computed_;

  /// Raw image location, if computed
  vnl_vector_fixed<double,2> pixel_loc_;

  /// Raw image velocity, if computed (note, this is not the same as
  /// the world velocity mapped to image coordinates)
  vnl_vector_fixed<double,2> pixel_vel_;

  /// Raw image location covariance matrix, if computed
  /// @todo: is this member different than the covariance matrix?
  vnl_vector_fixed<double,4> pixel_var_;

  /// Raw image location covariance matrix, if computed
  vnl_double_2x2 cov_matrix_;

  // Accessors for timestamp
  vidtk::timestamp const& get_timestamp() const { return time_; }
  track_state & set_timestamp(vidtk::timestamp const& v) { time_ = v; return *this; }

  /// Track timestamp
  timestamp time_;

  /// Aligned motion history image bounding box.
  vgl_box_2d<unsigned> amhi_bbox_;

  /// Property map of data
  mutable property_map data_; //Mutable because get_location will cache expensive computation results here.

  /// \brief Create a deep copy clone.
  ///
  /// This function calls the default copy constructor following by
  /// specialized deep copy operations for the individual fields.
  track_state_sptr clone();

  /// Store the supplied latitude and longitude in track_state.
  /// @deprecated Use call with geo_lat_lon
  void set_latitude_longitude( double latitude, double longitude );

  /** Store the supplied latitude and longitude in track_state.
   */
  void set_latitude_longitude( geo_coord::geo_coordinate const& lat_lon );

  /// \brief Retrieve latitude and longitude from track_state.
  ///
  /// Returns true if a latitude and longitude are available;
  /// false otherwise. If you need UTM or MGRS versions of the
  /// geographic data, then one can use vidtk::geographic::geo_coords
  /// for storage and conversions.
  /// @deprecated Use call with geo_lat_lon
  bool latitude_longitude( double & latitude, double & longitude ) const;

  /** Retrieve latitude and longitude from track_state. Returns true
   * if a latitude and longitude are available; false otherwise. If
   * you need UTM or MGRS versions of the geographic data, then one
   * can use vidtk::geo_coords::convert_to_{UTM,MGRS} for storage and
   * conversions.
   */
  bool latitude_longitude( geo_coord::geo_lat_lon& lat_lon ) const;
  const geo_coord::geo_lat_lon & latitude_longitude() const;

  /** Retrieve the smoothed location (related to loc_) in UTM.  If it is
   *  not available, geo will be set to invalid */
  void get_smoothed_loc_utm( geo_coord::geo_UTM & geo ) const;
  const geo_coord::geo_UTM & get_smoothed_loc_utm() const;

  /** Retrieve the raw location filed (related to img_obj world_loc).  If
   * it is not available, geo will be set to invalid. */
  void get_raw_loc_utm( geo_coord::geo_UTM & geo ) const;

  /** Retrieve the velocity (related to vel_) in UTM.  If it is not available,
   * geo returns false. */
  bool get_utm_vel( vnl_vector_fixed<double, 2> & vel ) const;

  /** Fills in the relevant utm based files .
   * @note This may be better as an operation on a state rather than a property of a state.
   */
  void fill_utm_based_fields( plane_to_utm_homography const & h );

  void set_smoothed_loc_utm( geo_coord::geo_UTM const& geo );
  void set_raw_loc_utm( geo_coord::geo_UTM const& geo );
  void set_utm_vel( vnl_vector_fixed<double, 2> const& vel );

  /// Type of coordinate to be returned by get_location()
  enum location_type
  {
    stabilized_plane,           ///< tracking plane coordinates
    unstabilized_world,         ///< UTM coordinates
    stabilized_world            ///< smoothed UTM coordimates
  };

  /** \brief Get object location.
   *
   * The object location is returned in the specified coordinate system.
   * @param[in] type - coordinate system to return
   * @param[out] loc - location of object
   * @return \c true if coordinates are valid; \c false if not valid
   */
  bool get_location( location_type type, vnl_vector_fixed<double,3> & loc ) const;
  geo_coord::geo_coordinate const& get_geo_location() const;

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

  /** Get image location.
   *
   * Get the location of the image object. Location is returned in
   * image coordinates.
   *
   * This could be the object centroid, the bottom-most point, or any
   * other description.  The choice is left up to the algorithm
   * detecting the objects.  Of course, related detections are
   * expected to use a consistent definition of this location.
   * @param[out] x - x coordinate of location
   * @param[out] y - y coordinate of location
   * @return true if image location is returned, false if location
   * can not be obtained.
   */
  bool get_image_loc( double& x, double& y ) const;


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
  vnl_double_2x2 const& location_covariance() const;


  bool add_img_vel(vgl_h_matrix_2d<double> const& wld2img_H);

  // attribute API. Refer to attribute class for details.
  void set_attr (track_state_attributes::state_attr_t attr) { this->attributes_.set_attr(attr); }
  void set_attrs (track_state_attributes::raw_attrs_t attr) { this->attributes_.set_attrs(attr); }
  track_state_attributes::raw_attrs_t get_attrs() const { return this->attributes_.get_attrs(); }
  void clear_attr (track_state_attributes::state_attr_t attr) { this->attributes_.clear_attr(attr); }
  bool has_attr (track_state_attributes::state_attr_t attr) { return this->attributes_.has_attr(attr); }
  std::string get_attr_text () const { return this->attributes_.get_attr_text(); }

  // Track confidence value [0,1] up to the current state (detection).
  // Retrun value: true if returning available value through the argument;
  //                 false otherwise with the argument unchanged
  bool get_track_confidence(double & conf) const;
  void set_track_confidence(double val) { this->track_confidence_ = val; }

private:
  track_state_attributes attributes_; ///< attributes for this track state

  /// Confidence level or quality for the track up to this
  /// time. Higher values indicate higher confidence that is a real
  /// object being tracked.  (i.e. 0 - low confidence; 1.0 - high
  /// confidence).  Always in the range [0,1]
  boost::optional<double> track_confidence_;

  geo_coord::geo_UTM utm_smoothed_loc_;
  geo_coord::geo_UTM utm_raw_loc_;

  // The following member is logically const.
  // It does however perform lazy conversions on the get methods.
  // Because of that, it must be mutable
  mutable geo_coord::geo_coordinate geo_coord_;
  vnl_vector_fixed<double, 2> utm_vel_;

};


// ----------------------------------------------------------------
/** Track state compare predicate.
 *
 * This struct implements a predicate that returns \c true if the
 * timestamp is greater than or equal to the target timestamp.  This
 * is used to locate specific states within a track history.  There is
 * an implicit assumption that the time increases as we advance
 * through the vector.
 */
struct track_state_ts_pred
{
  typedef track_state_sptr argument_type;
  track_state_ts_pred( timestamp const& ts)
  : ts_ (ts)
  { }


  bool operator()(track_state_sptr const& t) const
  {
    return (t->time_ >= ts_);
  }

private:
  timestamp ts_;
};

} // namespace vidtk

#endif
