/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
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


#include <vector>
#include <vbl/vbl_smart_ptr.h>

#include <tracking_data/image_object.h>
#include <tracking_data/amhi_data.h>
#include <tracking_data/pvo_probability.h>
#include <tracking_data/track_state.h>
#include <tracking_data/track_sptr.h>
#include <tracking_data/track_attributes.h>

#include <utilities/image_histogram.h>
#include <utilities/property_map.h>
#include <utilities/timestamp.h>
#include <utilities/uuid_able.h>
#include <utilities/homography.h>

#include <vgl/vgl_box_2d.h>

namespace vidtk
{

// ----------------------------------------------------------------
/** \brief Parent class of all tracks.
 *
 * This class represents the spatio-temporal track of an object.
 *
 * The vector of states specifies the path of the object with one
 * entry for each detection.
 */
class track
  : public uuid_able<track>
{
public:

  // -- TYPES --
  typedef image_histogram image_histogram_t;
  typedef std::vector < track_sptr > vector_t;
  typedef vxl_int_64 raw_attrs_t;
  typedef unsigned track_id_t;

  /// Constructor.
  track();

  /// Create a new track with specific UUID
  track( vidtk::uuid_t u );

  /// Destructor.
  ~track();

  /** \brief Add state to track.
   *
   * The supplied state is added to the end of
   * the history vector.
   * @param[in] state - new track state to add
   */
  void add_state( track_state_sptr const& state );

  /// The last (newest) state.
  track_state_sptr const& last_state() const;

  /// The first (oldest) state.
  track_state_sptr const& first_state() const;

  /// \brief Returns a history of states.
  ///
  /// Vector is imutable, contents of states are mutable.
  std::vector< track_state_sptr > const& history() const;

  /// Set the id of a track.
  void set_id( unsigned id );

  /** \brief Get id of this track.
   *
   * A tracker assigns a unique number for each
   * track created. Note that if tracks are collected from multiple
   * trackers, the track id may not be unique.
   */
  unsigned id() const;

  /// \brief Creates a deep copy of the weight image, source image
  /// and histogram matrix.
  ///
  /// Creates a deep copy of the image chip. It will only be used to
  /// update the appearance model through correlation/ssd. We could
  /// avoid keeping deep copy if we can guarantee that the last
  /// observation of the track would not be older than the image_buffer
  /// length.
  void set_amhi_datum( amhi_data const &);

  /// Returns a copy of the weight image, source image and histogram matrix.
  amhi_data const & amhi_datum() const { return amhi_datum_; }

  ///@todo complete the from amhi. (I don't know what this means!)
  /// \brief Set the initial aligned motion history image bounding box.
  void set_init_amhi_bbox( unsigned idx, vgl_box_2d<unsigned> const & bbox );

  /// Return the last time there was a match.
  double last_mod_match() const;

  /// Set the last time a match was found.
  void set_last_mod_match( double time );

  /// An estimate of the likelihood that this track is a false alarm and
  /// should, for example, be ignored by downstream processing.
  double false_alarm_likelihood() const;

  /// Set the likelihood threshold for a false alarm.
  void set_false_alarm_likelihood( double fa );

  /// \brief Creates a clone instance of this track.
  ///
  /// This function creates and returns deep copy of the full track, i.e. all the
  /// track_state, image_object, etc. objects.
  track_sptr clone() const;
  track_sptr shallow_clone() const;

  /// Returns the property map.
  property_map& data()
  {
    return data_;
  }

  /// Returns the property map.
  property_map const& data() const
  {
    return data_;
  }

  /// Returns the image object at the begin.
  image_object_sptr get_begin_object() const;

  /// Returns the image object at the end.
  image_object_sptr get_end_object() const;

  /// Returns an object with the given id.
  image_object_sptr get_object( unsigned ind ) const;

  // Append another tracks to this track. Supports track linking.
  void append_track( track& );

  /// Returns the length of the world.
  double get_length_world() const;

  /// Returns the length in time.
  double get_length_time_secs() const;

  /// Return the probability values for person, vehicle other.
  bool get_pvo(pvo_probability& pvo) const;

  /// Set the object classification probabilities
  void set_pvo( pvo_probability const & pvo );

 /** \brief Reset track history.
 *
 * This method resets the current track history by replacing it with
 * the specified track history vector.  The old track state history is
 * dereferenced and left for the smart pointer to clean up.  The
 * specified list of state pointers is shallow copied into the history
 * vector.
 *
 * @param[in] hist - new track state history vector.
 */
 void reset_history(std::vector< track_state_sptr > const& hist);

  /// Track that shares the states between begin and end.
  /// Copies over the track state pointer (the states are shared), has the same id, and has the same uuid.
  /// @note This operation may be better represented as an operation
  /// on a track rather than a property of a track.
  track_sptr get_subtrack_shallow( timestamp const& begin, timestamp const& end ) const;

  /// Track that shares the states between begin and end.
  /// Different uuid.
  /// Creates a copy of each track state (new instance of the each track state).
  /// @note This operation may be better represented as an operation
  /// on a track rather than a property of a track.
  track_sptr get_subtrack( timestamp const& begin, timestamp const& end ) const;

  /// Track that shares the states between begin and end.
  /// Has the same uuid.
  /// Creates a copy of each track state (new instance of the each track state).
  /// @note This operation may be better represented as an operation
  /// on a track rather than a property of a track.
  track_sptr get_subtrack_same_uuid( timestamp const& begin, timestamp const& end ) const;

  /// \brief Return histogram representing the latest track model.
  ///
  /// See documentation of histogram_ field for details.
  image_histogram const & histogram() const;

  /// \brief Update the latest histogram model of the track.
  ///
  /// Makes a deep copy of the underlying histgoram matrix.
  void set_histogram( image_histogram const & );

  ///@{
  /** Tracker type and ID.
   *
   * The tracker ID and type are designed to be used in multi-tracker
   * environments to provide additional details, in addition to
   * track-id, on the source of a track. This allows tracks to be
   * unique when a key of (track-id, tracker-id, tracker-type) is
   * used.
   *
   * In a multi-tiled tracking system, the tracker-id can be used to
   * indicate which tile the track came from.  The tracker-type field
   * can be used to differentiate PERSON trackers from VEHICLE
   * trackers, for example.  Using a more expansive track key
   * eliminates the need to renumber tracks when sets from different
   * trackers need to be combined.
   *
   * If a track id/type has not been set and it is retrieved with a
   * get_* method, a value of zero will be returned, so don't use zero
   * as a valid type/id code.
   *
   * These accessor methods have been added to better encapsulate the
   * arcane property map access code needed to directly access these
   * values.
   *
   */
  unsigned int get_tracker_id() const;
  void set_tracker_id (unsigned int id);

  /// Valid types for trackers.  Type 'unknown' is the default value
  /// and indicates that the tracker type was not set (susally a
  /// programming error).  Type 'other' can be used for testing new
  /// tracker types before adding a specific tracker type.
  enum tracker_type_code_t
  {
    TRACKER_TYPE_UNKNOWN = 0, ///< default value, probably an error
    TRACKER_TYPE_PERSON,      ///< person tracker
    TRACKER_TYPE_VEHICLE,     ///< vehicle tracker
    TRACKER_TYPE_OTHER,       ///< unspecified tracker, useful for experiments
  };

  track::tracker_type_code_t get_tracker_type() const;
  void set_tracker_type (track::tracker_type_code_t type);

///@{
/** Access track text notes.
 *
 */
  void add_note (std::string const& note );
  void get_note (std::string& note ) const;
///@}

///@{
  /// Convert canonical string to tracker type code
  static track::tracker_type_code_t convert_tracker_type( std::string const& type );
  static const char* convert_tracker_type( track::tracker_type_code_t code);
///@}

///@{
  /// attibute API. Refer to attribute class for details.
  void set_attr (track_attributes::track_attr_t attr) { this->attributes_.set_attr(attr); }
  void set_attrs (track_attributes::raw_attrs_t attr) { this->attributes_.set_attrs(attr); }
  track_attributes::raw_attrs_t get_attrs() const { return this->attributes_.get_attrs(); }
  void clear_attr (track_attributes::track_attr_t attr) { this->attributes_.clear_attr(attr); }
  bool has_attr (track_attributes::track_attr_t attr) { return this->attributes_.has_attr(attr); }
  std::string get_attr_text () const { return this->attributes_.get_attr_text(); }
///@}

  /// Get confidence / quality of this track up to the lastest state.
  // Value in [0,1] from the latest state (detection).
  // Retrun value: true if returning available value through the argument;
  //                 false otherwise with the argument unchanged
  bool get_latest_confidence(double & conf) const;

  void set_latest_confidence(double c)
  {
    if(!history_.empty())
    {
      history_[history_.size()-1]->set_track_confidence(c);
    }
  }

  /// Get the external UUID for this track. This identifier was given
  /// by some GUI or other external interface for this track, and may
  /// or may not be equivalent to the internal UUID.
  std::string const& external_uuid() const;
  bool has_external_uuid() const;
  void set_external_uuid( const std::string& uuid );

protected:
  /// Appends the track to the history.
  void append_history( std::vector< track_state_sptr > const& );

private:
  /// \brief Checks the temporal ordering of the new state.
  ///
  /// Checking whether the timestamp of the latest state to be inserted into
  /// history is consistent with the current trend. We need at least two prior
  /// detections to decide that. This funciton allows back tracking, as long as
  /// the states are still added through history_.push_back().
  bool is_timestamp_valid( track_state_sptr const& state );

  unsigned id_;

  std::vector< track_state_sptr > history_;

  property_map data_;

  amhi_data amhi_datum_;

  double last_mod_match_;

  double false_alarm_likelihood_;

  pvo_probability_sptr pvo_;//Person Vehicle Other probability

  /// \brief Image chip (color/grayscale) histogram.
  ///
  /// Image (color or grayscale) histgoram corresponding to the
  /// latest state (detection) in the track. In future, this could be
  /// replaced by a representative histogram model that is computed
  /// over a few frames and updated in a controlled fashion.
  image_histogram  histogram_;

  track_attributes attributes_; ///< attributes for this track

  std::string external_uuid_;

  track::tracker_type_code_t tracker_type_; ///< tracker type code
  unsigned int tracker_instance_; ///< tracker id or instance

};

std::ostream& operator << ( std::ostream & str, const vidtk::track & obj );
std::istream& operator >> ( std::istream & str, vidtk::track::tracker_type_code_t& type );

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

} // end namespace vidtk


#endif // vidtk_track_h_
