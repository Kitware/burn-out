/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/track.h>
#include <tracking/amhi.h>
#include <tracking/amhi_data.h>
#include <tracking/tracking_keys.h>
#include <tracking/pvo_probability.h>
#include <tracking/fg_matcher.h>
#include <utilities/object_cache.txx>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vbl/vbl_smart_ptr.txx>
#include <vcl_algorithm.h>

namespace vidtk
{

track
::track()
  : id_( 0 ),
    history_(),
    data_(),
    amhi_datum_(),
    last_mod_match_(0),
    false_alarm_likelihood_( 0.0 ),
    pvo_(NULL),
    attributes_(0)

{
  amhi_datum_.ghost_count = 0;
}

track
::track( uuid_t u)
  :
  uuid_able<track>(u),
  id_( 0 ),
  history_(),
  data_(),
  amhi_datum_(),
  last_mod_match_(0),
  false_alarm_likelihood_( 0.0 ),
  pvo_(NULL),
  attributes_(0)
{
  amhi_datum_.ghost_count = 0;
}


track
::~track()
{
  if(pvo_)
  {
    delete pvo_;
  }
}


void
track
::add_state( track_state_sptr const& state )
{
  if(!history_.empty() && history_.back()->time_==state->time_)
  {
    return;
  }

  log_assert( this->is_timestamp_valid( state ),
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
/** Get track history.
 *
 * This method returns a read only copy of the whole track state
 * history.
 */
vcl_vector< track_state_sptr > const&
track
::history() const
{
  return history_;
}


// ----------------------------------------------------------------
/** Reset track history.
 *
 * This method resets the current track history by replacing it with
 * the specified track history vector.  The old track state history is
 * dereferenced and left for the smart pointer to clean up.  The
 * specified list of state pointers is shallow copied into the history
 * vector.
 *
 * @param[in] hist - new track state history vector.
 */
void track::
reset_history (vcl_vector< track_state_sptr > const& hist)
{
  history_ = hist;
}


void
track
::set_id( unsigned id )
{
  id_ = id;
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
  vcl_vector<track_state_sptr>::const_iterator iter;
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

  // Deep copy
  if( this->pvo_ )
  {
    copy->pvo_ = new pvo_probability( *(this->pvo_) );
  }

  //Deep copy property map
  tracking_keys::deep_copy_property_map(this->data_, copy->data_);
  fg_matcher< vxl_byte >::sptr_t fgm;

  if( copy->data_.has( tracking_keys::foreground_model ) )
  {
    copy->data_.get(tracking_keys::foreground_model, fgm);
  }

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
  vcl_vector< image_object_sptr > objs;

  if( ! history_[ind]->data_.get( tracking_keys::img_objs, objs )
      || objs.empty() )
  {
    log_error( "no MOD for track state: " << ind  << "\n" );
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
  for( vcl_vector<track_state_sptr>::const_iterator iter = history_.begin();
       iter != history_.end(); ++iter)
  {
    vcl_vector<vidtk::image_object_sptr> objs;
    if ( (*iter)->data_.get( vidtk::tracking_keys::img_objs, objs ) )
    {
      vnl_vector_fixed<double,3> current = objs[0]->world_loc_;
      if(prev_set)
      {
        distance += (current-prev_point).magnitude();
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
::has_pvo() const
{
  return pvo_ != NULL;
}

pvo_probability
track
::get_pvo() const
{
  if(this->has_pvo())
  {
    return *pvo_;
  }
  return pvo_probability();
}

void
track
::set_pvo( pvo_probability const * pvo ) //creates a copy
{
  this->set_pvo( pvo->get_probability_person(),
                 pvo->get_probability_vehicle(),
                 pvo->get_probability_other() );
}

void
track
::set_pvo( double person, double vehicle, double other)
{
  if(pvo_)
  {
    delete pvo_;
  }
    pvo_ = new pvo_probability( person, vehicle, other );
  }

void
track
::reverse_history()
{
  vcl_reverse( history_.begin(), history_.end() );
}



// Removes track history in [ts+1, end) interval. Returns false in case
// failure.
bool
track
::truncate_history( timestamp const& qts )
{
  vcl_vector< track_state_sptr >::iterator it = vcl_find_if(
    history_.begin(), history_.end(), track_state_ts_pred( qts ) );

  if( it == history_.end() )
  {
    return false;
  }

  history_.erase( it+1, history_.end() );

  return true;
}

image_histogram<vxl_byte, float> const &
track
::histogram() const
{
  return histogram_;
}

void
track
::set_histogram( image_histogram<vxl_byte, float> const & h )
{
  histogram_ = h;
}

const
vgl_box_2d< double >
track
::get_spatial_bounds( )
{
  return this->get_spatial_bounds( this->first_state()->time_,
                                   this->last_state()->time_ );
}

const
vgl_box_2d< double >
track
::get_spatial_bounds( timestamp const& begin,
                      timestamp const& end )
{
  vgl_box_2d< double > result;
  vcl_vector< track_state_sptr >::iterator iter = this->history_.begin();
  for( ; iter != this->history_.end() && (*iter)->time_ < begin; ++iter);
  if(iter == this->history_.end())
    return result;
  for( ; iter != this->history_.end() && (*iter)->time_ <= end; ++iter)
  {
    result.add(vgl_point_2d<double>((*iter)->loc_[0],(*iter)->loc_[1]));
  }
  return result;
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
  vcl_vector< track_state_sptr >::const_iterator iter = this->history_.begin();
  for( ; iter != this->history_.end() && (*iter)->time_ < begin; ++iter);
  if(iter == this->history_.end())
    return result;
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
  vcl_vector< track_state_sptr >::const_iterator iter = this->history_.begin();
  for( ; iter != this->history_.end() && (*iter)->time_ < begin; ++iter);
  if(iter == this->history_.end())
    return result;
  for( ; iter != this->history_.end() && (*iter)->time_ <= end; ++iter)
  {
    result->add_state(*iter);
  }
  return result;
}

void
track
::update_latitudes_longitudes( plane_to_utm_homography const& H_wld2utm )
{
  vcl_vector< track_state_sptr >::iterator iter = this->history_.begin();
  for( ; iter != this->history_.end(); ++iter)
  {
    (*iter)->set_latitude_longitude( H_wld2utm );
  }
}

void
track
::update_latest_latitude_longitude( plane_to_utm_homography const& H_wld2utm )
{
  this->last_state()->set_latitude_longitude( H_wld2utm );
}


// ----------------------------------------------------------------
// Expand these with groups as needed. See track_state.cxx for sample
// code.
// ----------------------------------------------------------------
void
track
::set_attr (track_attr_t attr)
{
  if (attr & _ATTR_TRACKER_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_TRACKER_MASK) | attr;
  }
  else
  {
    this->attributes_ |= attr;
  }
}


// ----------------------------------------------------------------
void
track
::clear_attr (track_attr_t attr)
{
  if (attr & _ATTR_TRACKER_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_TRACKER_MASK);
  }
  else
  {
    this->attributes_ &= ~attr;
  }
}


// ----------------------------------------------------------------
bool
track
::has_attr (track_attr_t attr)
{
  if (attr & _ATTR_TRACKER_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_TRACKER_MASK) == attr;
  }
  else
  {
    return (this->attributes_ & attr) == attr;
  }
}




vcl_ostream& operator << ( vcl_ostream & str, const vidtk::track & obj )
{
  str << "track (id: " << obj.id()
  << "   history from " << obj.first_state()->time_
  << " to " << obj.last_state()->time_
  << "  size: " << obj.history().size() << " states.";

  return ( str );
}


track_sptr convert_from_klt_track(klt_track_ptr trk)
{
  track_sptr vidtk_track(new track);

  std::vector<track_state_sptr> states;

  klt_track_ptr cur_point = trk;

  while (cur_point)
  {
    track_state_sptr state(new track_state);
    vidtk::timestamp ts;

    ts.set_frame_number(cur_point->point().frame);

    state->loc_[0] = cur_point->point().x;
    state->loc_[1] = cur_point->point().y;
    state->loc_[2] = 0;
    state->vel_[0] = 0;
    state->vel_[1] = 0;
    state->vel_[2] = 0;
    state->time_ = ts;

    states.push_back(state);

    cur_point = cur_point->tail();
  }

  vidtk_track->reset_history(states);
  vidtk_track->reverse_history();

  return vidtk_track;
}


} // end namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::track );
VBL_SMART_PTR_INSTANTIATE( vidtk::track_state );
VIDTK_OBJECT_CACHE_INSTANTIATE( vidtk::track );
