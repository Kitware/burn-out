/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/track.h>
#include <tracking_data/amhi_data.h>
#include <tracking_data/tracking_keys.h>
#include <tracking_data/fg_matcher.h>

#include <utilities/object_cache.txx>

#include <logger/logger.h>

#include <vbl/vbl_smart_ptr.hxx>
#include <algorithm>
#include <sstream>

#include <logger/logger.h>

VIDTK_LOGGER("track");


namespace vidtk
{

track
::track()
  : id_( 0 ),
    history_(),
    data_(),
    amhi_datum_(),
    last_mod_match_( 0 ),
    false_alarm_likelihood_( 0.0 ),
    pvo_( NULL ),
    tracker_type_( TRACKER_TYPE_UNKNOWN ),
    tracker_instance_( 0 )
{
  amhi_datum_.ghost_count = 0;
}

track
::track( vidtk::uuid_t u )
  : uuid_able<track>( u ),
    id_( 0 ),
    history_(),
    data_(),
    amhi_datum_(),
    last_mod_match_( 0 ),
    false_alarm_likelihood_( 0.0 ),
    pvo_( NULL ),
    tracker_type_( TRACKER_TYPE_UNKNOWN ),
    tracker_instance_( 0 )
{
  amhi_datum_.ghost_count = 0;
}


track
::~track()
{
}


void
track
::add_state( track_state_sptr const& state )
{
  if(!history_.empty() && history_.back()->time_==state->time_)
  {
    return;
  }

  LOG_ASSERT( this->is_timestamp_valid( state ),
              "Could not append new state due to inconsistent timestamp." );

  history_.push_back ( state );
}


bool
track
::is_timestamp_valid( track_state_sptr const& state )
{
  if( history_.size() < 2 )
    return true;

  double h_diff_ts = history_.back()->time_.diff_in_secs(
                          history_[ history_.size() - 2 ]->time_ );
  double s_diff_ts = state->time_.diff_in_secs( history_.back()->time_ );

  if( (h_diff_ts > 0.0 && s_diff_ts > 0.0 ) ||
      (h_diff_ts < 0.0 && s_diff_ts < 0.0 ) )
  {
    return true;
  }
  else
  {
    return false;
  }
}

void
track
::set_amhi_datum( amhi_data const & datum)
{
  unsigned old_gcount = amhi_datum_.ghost_count;
  amhi_datum_.deep_copy( datum );
  amhi_datum_.ghost_count += old_gcount;
}


track_state_sptr const&
track
::last_state() const
{
  return history_.back();
}


track_state_sptr const&
track
::first_state() const
{
  return history_.front();
}


// ----------------------------------------------------------------
/** \brief Get track history.
 *
 * This method returns a read only copy of the whole track state
 * history.
 */
std::vector< track_state_sptr > const&
track
::history() const
{
  return history_;
}


// ----------------------------------------------------------------
/** \brief Reset track history.
 *
 * This method resets the current track history by replacing it with
 * the specified track history vector.  The old track state history is
 * dereferenced and left for the smart pointer to clean up.  The
 * specified list of state pointers is shallow copied into the history
 * vector.
 *
 * @param hist new track state history vector.
 */
void track::
reset_history (std::vector< track_state_sptr > const& hist)
{
  history_ = hist;
}


void
track
::set_id( unsigned _id )
{
  id_ = _id;
}

unsigned
track
::id() const
{
  return id_;
}

track_sptr
track
::clone() const
{
  track_sptr copy = this->shallow_clone();

  //Deep copy the history
  copy->history_.clear(); // first undo the shallow copy above.
  std::vector<track_state_sptr>::const_iterator iter;
  for(iter = this->history().begin(); iter != this->history().end(); ++iter)
  {
    copy->history_.push_back((*iter)->clone());
  }

  return copy;
}


track_sptr
track
::shallow_clone() const
{
  track_sptr copy = new track( *this );

  //Deep copy property map
  tracking_keys::deep_copy_property_map(this->data_, copy->data_);

  //Deep copy the amhi data
  copy->amhi_datum_.deep_copy( this->amhi_datum() );

  return copy;
}


void
track
::set_init_amhi_bbox(unsigned idx, vgl_box_2d<unsigned> const & bbox )
{
  history_[idx]->amhi_bbox_ = bbox;
}

image_object_sptr
track
::get_begin_object() const
{
  return get_object( 0 );
}

image_object_sptr
track
::get_end_object() const
{
  return get_object( history_.size() - 1 );
}

image_object_sptr
track
::get_object( unsigned ind ) const
{
  if ( ind >= history_.size() )
  {
    return NULL;
  }
  std::vector< image_object_sptr > objs;

  if( ! history_[ind]->data_.get( tracking_keys::img_objs, objs )
      || objs.empty() )
  {
    LOG_ERROR( "no MOD for track state: " << ind );
    return NULL;
  }

  return objs[0];
}

double
track
::false_alarm_likelihood() const
{
  return false_alarm_likelihood_;
}

void
track
::set_false_alarm_likelihood( double fa )
{
  false_alarm_likelihood_ = fa;
}

double
track
::last_mod_match() const
{
  return last_mod_match_;
}

void
track
::set_last_mod_match( double time )
{
  last_mod_match_ = time;
}

void
track
::append_track( track& in_track )
{
  append_history( in_track.history() );
}

void
track
::append_history(const std::vector<track_state_sptr> & in_history)
{

  for( std::vector<track_state_sptr>::const_iterator hiter = in_history.begin();
        hiter != in_history.end();
        hiter++ )
  {
    this->add_state( *hiter );
  }

}

double
track
::get_length_world() const
{
  double distance = 0;
  vnl_vector_fixed<double,3> prev_point;
  bool prev_set = false;
  for( std::vector<track_state_sptr>::const_iterator iter = history_.begin();
       iter != history_.end(); ++iter)
  {
    std::vector<vidtk::image_object_sptr> objs;
    if ( (*iter)->data_.get( vidtk::tracking_keys::img_objs, objs ) )
    {
      tracker_world_coord_type const& current = objs[0]->get_world_loc();
      if(prev_set)
      {
        distance += (current - prev_point).magnitude();
        prev_point = current;
      }
      else
      {
        prev_point = current;
        prev_set = true;
      }
    }
  }
  return distance;
}

double
track
::get_length_time_secs() const
{
  if(!history_.size())
  {
    return 0;
  }
  return this->last_state()->time_.diff_in_secs( this->first_state()->time_ );
}

bool
track
::get_pvo(pvo_probability& pvo) const
{
  if (pvo_.ptr() != NULL)
  {
    pvo = *pvo_;
    return true;
  }
  // else set to default pvo
  pvo = pvo_probability();
  return false;
}

void
track
::set_pvo( pvo_probability const & pvo )
{
  this->pvo_ = new pvo_probability( pvo );
}

image_histogram const &
track
::histogram() const
{
  return histogram_;
}

void
track
::set_histogram( image_histogram const & h )
{
  histogram_ = h;
}


track_sptr
track
::get_subtrack( timestamp const& begin, timestamp const& end ) const
{
  track_sptr result = get_subtrack_same_uuid( begin, end );
  result->regenerate_uuid();
  return result;
}

track_sptr
track
::get_subtrack_same_uuid( timestamp const& begin, timestamp const& end ) const
{
  track_sptr result = new track(this->get_uuid());
  std::vector< track_state_sptr >::const_iterator iter = this->history_.begin();
  for( ; iter != this->history_.end() && (*iter)->time_ < begin; ++iter) ;
  if(iter == this->history_.end())
  {
    return result;
  }

  for( ; iter != this->history_.end() && (*iter)->time_ <= end; ++iter)
  {
    result->add_state((*iter)->clone());
  }
  return result;
}

track_sptr
track
::get_subtrack_shallow( timestamp const& begin, timestamp const& end ) const
{
  track_sptr result = new track(this->get_uuid());
  result->set_id(this->id());
  std::vector< track_state_sptr >::const_iterator iter = this->history_.begin();
  for( ; iter != this->history_.end() && (*iter)->time_ < begin; ++iter) ;
  if(iter == this->history_.end())
  {
    return result;
  }

  for( ; iter != this->history_.end() && (*iter)->time_ <= end; ++iter)
  {
    result->add_state(*iter);
  }
  return result;
}


/*
 * get/set tracker attribute codes.
 *
 * Currently we are (mis)using the tracker subytpe to store the tracker
 * id.
 */
vidtk::track::tracker_type_code_t
track
::get_tracker_type() const
{
  return this->tracker_type_;
}


unsigned int
track
::get_tracker_id() const
{
  return  this->tracker_instance_;
}


void
track
::set_tracker_type (tracker_type_code_t type)
{
  this->tracker_type_ = type;
}


void
track
::set_tracker_id (unsigned int inst)
{
  this->tracker_instance_ = inst;
}


vidtk::track::tracker_type_code_t
track
::convert_tracker_type( std::string const& type)
{

  if ("person" == type)
  {
    return TRACKER_TYPE_PERSON;
  }

  if ("vehicle" == type)
  {
    return TRACKER_TYPE_VEHICLE;
  }

  if ("unknown" == type)
  {
    return TRACKER_TYPE_UNKNOWN;
  }

  LOG_ERROR ("Unrecognized tracker type name: '" << type << "'");
  return TRACKER_TYPE_UNKNOWN;
}


const char*
track
::convert_tracker_type( vidtk::track::tracker_type_code_t code)
{
  switch (code)
  {
  case TRACKER_TYPE_UNKNOWN:  return "unknown";
  case TRACKER_TYPE_PERSON:   return "person";
  case TRACKER_TYPE_VEHICLE:  return "vehicle";

  default:
    LOG_ERROR ("Unexpected tracker type: " << static_cast< unsigned >(code) );
    return "** unexpected tracker type **";
  }
}

std::string const&
track
::external_uuid() const
{
  return external_uuid_;
}

bool
track
::has_external_uuid() const
{
  return ( external_uuid_.size() > 0 );
}

void
track
::set_external_uuid (const std::string& uuid)
{
  external_uuid_ = uuid;
}

void
track
::add_note( std::string const& note )
{
  std::string& local_note = this->data().get_or_create_ref < std::string >( tracking_keys::note );
  if ( ! local_note.empty() )
  {
    local_note += "\n";
  }

  local_note += note;
}


void
track
::get_note (std::string& note) const
{
  note.clear();
  std::string const* ptr = this->data().get_if_avail < std::string >( tracking_keys::note );
  if (ptr != 0)
  {
    note = *ptr;
  }
}

bool
track
::get_latest_confidence(double & conf) const
{
  return last_state()->get_track_confidence(conf);
}

std::ostream& operator << ( std::ostream & str, const vidtk::track & obj )
{
  str << "track (id: " << obj.id();
  if( obj.history().empty() )
  {
    str << "   history empty";
  }
  else
  {
    str << "   history from " << obj.first_state()->time_
        << " to " << obj.last_state()->time_;
  }
  str << "  size: " << obj.history().size() << " states.";

  return ( str );
}


std::istream& operator >> ( std::istream & str, vidtk::track::tracker_type_code_t& code )
{
  int int_code;
  str >> int_code;
  code = static_cast<vidtk::track::tracker_type_code_t>(int_code);
  return str;
}


} // end namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::track );
VBL_SMART_PTR_INSTANTIATE( vidtk::track_state );
VIDTK_OBJECT_CACHE_INSTANTIATE( vidtk::track );
