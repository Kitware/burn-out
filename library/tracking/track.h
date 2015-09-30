/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_h_
#define vidtk_track_h_

/**
\file
\brief
Method and field definitions of a track.
*/


#include <vcl_vector.h>
#include <vbl/vbl_smart_ptr.h>

#include <kwklt/klt_track.h>

#include <tracking/image_object.h>
#include <tracking/amhi_data.h>
#include <tracking/track_state.h>
#include <tracking/track_sptr.h>

#include <utilities/image_histogram.h>
#include <utilities/property_map.h>
#include <utilities/timestamp.h>
#include <utilities/uuid_able.h>
#include <utilities/homography.h>

#include <vgl/vgl_box_2d.h>

namespace vidtk
{

class pvo_probability;
///Parent class of all tracks.
class track
  : public uuid_able<track>
{
public:
  typedef image_histogram < vxl_byte, float > image_histogram_t;

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
  enum track_attr_t
  {
    // Only one of these can be active at a time
    ATTR_TRACKER_PERSON            = 0x0001, ///< Person tracker
    ATTR_TRACKER_VEHICLE           = 0x0002, ///< vehicle tracker
    ATTR_TRACKER_avail             = 0x0003, // available values (3..f)
    _ATTR_TRACKER_MASK             = 0x000f,

    // Single bit attributes
    ATTR_AMHI = 0x00010,
  };


  ///Constructor.
  track();
  track( uuid_t u);

  ///Destructor.
  ~track();

  ///Add a state.
  void add_state( track_state_sptr const& state );

  ///The last state.
  track_state_sptr const& last_state() const;

  ///The first state.
  track_state_sptr const& first_state() const;

  ///Contains a history of states.
  vcl_vector< track_state_sptr > const& history() const;

  ///Set the id of a track.
  void set_id( unsigned id );

  ///Id of track.
  unsigned id() const;

  ///\brief Creates a deep copy of the weight image, source image
  /// and histogram matrix.
  ///
  ///Creates a deep copy of the image chip. It will only be used to
  ///update the appearance model through correlation/ssd. We could
  ///avoid keeping deep copy if we can guarantee that the last
  ///observation of the track would not be older than the image_buffer
  ///length.
  void set_amhi_datum( amhi_data const &);

  ///Returns a copy of the weight image, source image and histogram matrix.
  amhi_data const & amhi_datum() const { return amhi_datum_; }

  //TODO: complete the from amhi.
  ///\brief Set the initial aligned motion history image bounding box.
  void set_init_amhi_bbox(unsigned idx, vgl_box_2d<unsigned> const & bbox );
  ///Return the last time there was a match.
  double last_mod_match() const;
  ///Set the last time a match was found.
  void set_last_mod_match( double time );

  /// An estimate of the likelihood that this track is a false alarm and
  /// should, for example, be ignored by downstream processing.
  double false_alarm_likelihood() const;
  ///Set the likelihood threshold for a false alarm.
  void set_false_alarm_likelihood( double fa );

  /// \brief Creates a clone instance of this track.
  ///
  /// This function creates deep copy of the full track, i.e. all the
  /// track_state, image_object, etc. objects.
  track_sptr clone() const;
  track_sptr shallow_clone() const;

  ///Returns the property map.
  property_map& data()
  {
    return data_;
  }

  ///Returns the property map.
  property_map const& data() const
  {
    return data_;
  }

  ///Returns the image object at the begin.
  image_object_sptr get_begin_object() const;

  ///Returns the image object at the end.
  image_object_sptr get_end_object() const;

  ///Returns an object with the given id.
  image_object_sptr get_object( unsigned ind ) const;

  // Append another tracks to this track. Supports track linking.
  void append_track( track& );
  ///Returns the length of the world.
  double get_length_world() const;
  ///Returns the length in time.
  double get_length_time_secs() const;
  ///Return true if has person, vehicle other probabilities set; otherwise false.
  bool has_pvo() const;

  ///Return the probability values for person, vehicle other.
  pvo_probability get_pvo() const;

  ///Creates a copy.
  void set_pvo( pvo_probability const * pvo );
  ///Set the probability of person, vehicle other.
  void set_pvo( double person, double vehicle, double other);

  /// \brief Reverse the order of track history.
  ///
  /// This merely reverse the order detections/states in the history buffer.
  /// Other track fields outside of the history buffer
  /// (e.g. tracking_keys::most_recent_clip ) should be handled externally
  /// based on the application's needs.
  void reverse_history();

  /// \brief Erases history after the specified time.
  ///
  /// Removes track history during [ts+1, end) interval.
  /// Returns false in case failure.
  bool truncate_history( timestamp const& ts);

  void reset_history(vcl_vector< track_state_sptr > const& hist);

  ///Get the spacial bounds of track
  const vgl_box_2d< double > get_spatial_bounds( );
  const vgl_box_2d< double > get_spatial_bounds( timestamp const& begin,
                                                 timestamp const& end );

  /// Track that shares the states between begin and end.
  /// Copies over the track state pointer (the states are shared), has the same id, and has the same uuid.
  track_sptr get_subtrack_shallow( timestamp const& begin, timestamp const& end ) const;
  /// Track that shares the states between begin and end.
  /// Different uuid.
  /// Creates a copy of each track state (new instance of the each track state).
  track_sptr get_subtrack( timestamp const& begin, timestamp const& end ) const;
  /// Track that shares the states between begin and end.
  /// Has the same uuid.
  /// Creates a copy of each track state (new instance of the each track state).
  track_sptr get_subtrack_same_uuid( timestamp const& begin, timestamp const& end ) const;

  /// \brief Return histogram representing the latest track model.
  ///
  /// See documentation of histogram_ field for details.
  image_histogram<vxl_byte, float> const & histogram() const;

  /// \brief Update the latest histogram model of the track.
  ///
  /// Makes a deep copy of the underlying histgoram matrix.
  void set_histogram( image_histogram<vxl_byte, float> const & );

  /// \brief Add lat/long world positions in the property map of all
  /// track_state's
  void update_latitudes_longitudes( plane_to_utm_homography const& );

  /// \brief Add lat/long world positions in the property map of the latest
  /// track_state.
  void update_latest_latitude_longitude( plane_to_utm_homography const& );

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
  bool has_attr (track_attr_t attr);


protected:
  ///Appends the track to the history.
  void append_history( vcl_vector< track_state_sptr > const& );

private:
  /// \brief Checks the temporal ordering of the new state.
  ///
  /// Checking whether the timestamp of the latest state to be inserted into
  /// history is consistent with the current trend. We need at least two prior
  /// detections to decide that. This funciton allows back tracking, as long as
  /// the states are still added through history_.push_back().
  bool is_timestamp_valid( track_state_sptr const& state );

  unsigned id_;

  vcl_vector< track_state_sptr > history_;

  property_map data_;

  amhi_data amhi_datum_;

  double last_mod_match_;

  double false_alarm_likelihood_;

  pvo_probability * pvo_;//Person Vehicle Other probability

  /// \brief Image chip (color/grayscale) histogram.
  ///
  /// Image (color or grayscale) histgoram corresponding to the
  /// latest state (detection) in the track. In future, this could be
  /// replaced by a representative histogram model that is computed
  /// over a few frames and updated in a controlled fashion.
  image_histogram<vxl_byte, float>  histogram_;

  vxl_int_64 attributes_; ///< attributes for this track

};

vcl_ostream& operator << ( vcl_ostream & str, const vidtk::track & obj );

// Utility predicates

// return true if the track id matches.
struct track_id_pred
{
  track_id_pred(unsigned id)
  {
    id_ = id;
  }

  bool operator()(track_sptr const& t)
  {
    return t->id() == id_;
  }

  unsigned id_;
};

vidtk::track_sptr convert_from_klt_track(klt_track_ptr trk);

} // end namespace vidtk


#endif // vidtk_track_h_
