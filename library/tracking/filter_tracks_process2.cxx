/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_tracks_process2.h"

#include <vcl_algorithm.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vnl/vnl_double_2.h>
#include <vgl/vgl_area.h>

#define DEBUG

namespace vidtk
{

filter_tracks_process2
::filter_tracks_process2( vcl_string const& name )
  : process( name, "filter_tracks_process2" ),
  config_(),
  min_distance_covered_sqr_( 0.0 ),
  min_arc_length_sqr_( 0.0 ),
  max_speed_sqr_( 0.0 ),
  min_speed_sqr_( 0.0 ),
  min_track_length_( 0.0 ),
  samples_( 0 ),
  max_median_area_( 0 ),
  max_median_area_amhi_( 0 ),
  max_frac_ghost_detections_( 0.0 ),
  max_aspect_ratio_amhi_( 0.0 ),
  min_occupied_bbox_amhi_( 0.0 ),
  disabled_(false),
  src_trks_( NULL ),
  out_trks_()
{
  //TODO: min_arc_length to be replaced by min_distance_covered
  config_.add( "min_arc_length", "-1" );
  config_.add( "min_distance_covered", "-1" );
  config_.add( "max_speed", "-1" );
  config_.add( "min_speed", "-1" );
  config_.add( "min_track_length", "-1" );
  config_.add( "samples", "10" );
  config_.add( "max_median_area", "-1" );
  config_.add( "amhi:max_median_area", "-1" );
  config_.add( "max_frac_ghost_detections", "-1" );
  config_.add( "amhi:max_aspect_ratio", "-1" );
  config_.add( "amhi:min_occupied_bbox", "-1" );
  config_.add( "disabled", "false" );
}

filter_tracks_process2
::~filter_tracks_process2()
{
}


config_block
filter_tracks_process2
::params() const
{
  return config_;
}


bool
filter_tracks_process2
::set_params( config_block const& blk )
{
  try
  {
    double v;

    blk.get( "disabled", disabled_ );

    blk.get( "min_arc_length", v );
    min_arc_length_sqr_ = v >= 0.0 ? v*v : -1;

    blk.get( "min_distance_covered", v );
    min_distance_covered_sqr_ = v >= 0.0 ? v*v : -1;

    blk.get( "max_speed", v );
    max_speed_sqr_ = v >= 0.0 ? v*v : -1;

    blk.get( "min_speed", v );
    min_speed_sqr_ = v >= 0.0 ? v*v : -1;

    blk.get( "min_track_length", v );
    min_track_length_ = v >= 0.0 ? v : -1;

    blk.get( "samples", samples_ );

    blk.get( "max_median_area", max_median_area_ );

    blk.get( "amhi:max_median_area", max_median_area_amhi_ );

    blk.get( "max_frac_ghost_detections",
              max_frac_ghost_detections_ );

    blk.get( "amhi:max_aspect_ratio", max_aspect_ratio_amhi_ );

    blk.get( "amhi:min_occupied_bbox", min_occupied_bbox_amhi_ );

  }
  catch( unchecked_return_value& )
  {
    log_error( name() << ": couldn't set parameters" );
    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
filter_tracks_process2
::initialize()
{
  return true;
}

bool
filter_tracks_process2
::step()
{
  if( disabled_ ) return true;
  log_assert( src_trks_ != NULL, "source tracks not provided" );

  filter_these( *src_trks_);

  src_trks_ = NULL;

  return true; //Currently no failure case.
}

vcl_vector< track_sptr >
filter_tracks_process2
::filter_these( vcl_vector< track_sptr > const & in_trks )
{
  out_trks_ = in_trks;
  vcl_vector< track_sptr >::iterator begin = out_trks_.begin();
  vcl_vector< track_sptr >::iterator end = out_trks_.end();


  if(  min_arc_length_sqr_ >= 0.0
    || max_speed_sqr_ >= 0.0
    || min_speed_sqr_ >= 0.0
    || min_distance_covered_sqr_ >= 0.0
    || min_track_length_ >= 0.0
    || max_median_area_ >= 0.0
    || max_median_area_amhi_ >= 0.0
    || max_aspect_ratio_amhi_ >= 0.0
    || min_occupied_bbox_amhi_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
          boost::bind( &self_type::filter_out_multi_params, this, _1 ) );
  }


  // hack for communicating false alarm likelihoods out of the system
  // (examined in, for example, virat::compute_descriptor_modules)
  for( vcl_vector< track_sptr >::iterator it = end;
       it != out_trks_.end(); ++it )
  {
    (*it)->set_false_alarm_likelihood( 1.0 );
  }

  // Actually remove the filtered out elements.
  out_trks_.erase( end, out_trks_.end() );

  vcl_cout << "Track filtering: " << in_trks.size() << " --> "
           << out_trks_.size() << vcl_endl;

  return out_trks_; //returning for 'flushing' tracks at the end of stream
}

void
filter_tracks_process2
::set_source_tracks( vcl_vector< track_sptr > const& trks )
{
  src_trks_ = &trks;
}


vcl_vector< track_sptr > const&
filter_tracks_process2
::tracks() const
{
  return out_trks_;
}


bool
filter_tracks_process2
::filter_out_multi_params( track_sptr const& trk ) const
{
  vcl_vector< track_state_sptr > const & trkHist = trk->history();
  unsigned trk_length = trkHist.size();

  //3rd condition: remove short (in time) tracks,
  // Track length < min_threshold
  // TODO: make min_threshold a function of time.
  if( min_track_length_ >= 0.0 )
  {
    if( trk_length < min_track_length_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (min_track_length_) : "
               << trk_length
               << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  if( max_frac_ghost_detections_ >= 0.0 )
  {
    if( double(trk->amhi_datum().ghost_count) / trk_length
        > max_frac_ghost_detections_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (max_frac_ghost_detections_) : "
               << double(trk->amhi_datum().ghost_count) / trk_length
               << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  double xVelSum = 0, yVelSum = 0;

  typedef vcl_vector< track_state_sptr >::const_iterator iter_type;
  vcl_vector< double > speeds( trk_length );
  vcl_vector< double > areas( trk_length );
  vcl_vector< double > areas_amhi( trk_length );
  vcl_vector< double > aspect_ratios_amhi( trk_length );
  vcl_vector< double > bbox_occupancy_amhi( trk_length );

  image_object_sptr obj;

  for( unsigned i = 0; i < trk_length; i++ )
  {
    speeds.push_back( trkHist[i]->vel_.squared_magnitude() );

    obj = trk->get_object( i );

    areas.push_back( obj->area_ );

    areas_amhi.push_back( trkHist[i]->amhi_bbox_.area() );

    double aspect;
    if( trkHist[i]->amhi_bbox_.width() > 0 &&
        trkHist[i]->amhi_bbox_.height() > 0 )
    {
      aspect =
        vcl_min( trkHist[i]->amhi_bbox_.width(), trkHist[i]->amhi_bbox_.height() )
      / vcl_max( trkHist[i]->amhi_bbox_.width(), trkHist[i]->amhi_bbox_.height() );
    }
    else
    {
      aspect = 0;
    }
    aspect_ratios_amhi.push_back( aspect );

    double occupancy;
    if( trkHist[i]->amhi_bbox_.area() > 0 )
    {
      occupancy = obj->area_ / trkHist[i]->amhi_bbox_.area();
    }
    else
    {
      occupancy = 0;
    }
    bbox_occupancy_amhi.push_back( occupancy );

    xVelSum += trkHist[i]->vel_[0];
    yVelSum += trkHist[i]->vel_[1];
  }

  //1st condition: Distance traveled by object < min_threshold
  if( min_arc_length_sqr_ >= 0.0 )
  {
    double arc_length_sqr = xVelSum * xVelSum + yVelSum * yVelSum;

    if( arc_length_sqr < min_arc_length_sqr_ )
    {

#ifdef DEBUG
      vcl_cout << "Filtering (min_arc_length_sqr_) : "
               << arc_length_sqr
               << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  //2nd condition: median speed > max_threshold
  if( min_speed_sqr_ >= 0.0 || max_speed_sqr_ >= 0.0 )
  {
    //Compute median speed for the track
    vcl_vector< double >::iterator medianSpeedIter = speeds.begin()
                                                    + speeds.size()/2;
    vcl_nth_element( speeds.begin(), medianSpeedIter, speeds.end() );

    if( max_speed_sqr_ >= 0.0 )
    {
      if( *medianSpeedIter > max_speed_sqr_ )
      {
#ifdef DEBUG
        vcl_cout << "Filtering (max_speed_sqr_) : "
          << *medianSpeedIter
          << ", Track # " << trk->id() << vcl_endl;
#endif
        return true;
      }
    }

    if( min_speed_sqr_ >= 0.0 )
    {
      if( *medianSpeedIter < min_speed_sqr_ )
      {
#ifdef DEBUG
        vcl_cout << "Filtering (min_speed_sqr_) : "
                 << *medianSpeedIter
                 << ", Track # " << trk->id() << vcl_endl;
#endif
        return true;
      }
    }
  }


  //4th condition: remove short (in space) tracks
  // Implementation based on track sub-sampling.
  // Will replace min_arc_length.
  if( min_distance_covered_sqr_ >= 0.0 )
  {
    double d = 0;
    unsigned jump = (unsigned) vcl_ceil( double( trk_length )
                                            / samples_ - 0.5 ); // round()
    if( jump == 1 )
    {
      d = ( trkHist[0]->loc_
          - trkHist[trk_length - 1]->loc_ ).squared_magnitude();
    }
    else
    {
      vnl_vector_fixed<double, 3> * last_pos = &((*trkHist.begin())->loc_);

      for( unsigned i = jump; i < trk_length;  i += jump )
      {
        d += ( trkHist[i]->loc_ - *last_pos ).squared_magnitude();
        last_pos = &(trkHist[i]->loc_);
      }

      d += ( trkHist[trk_length-1]->loc_ - *last_pos ).squared_magnitude();
    }

    if( d < min_distance_covered_sqr_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (min_distance_covered_sqr_) : "
               << d << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  //5th condition: remove tracks with too large 'median' area
  if( max_median_area_ >= 0.0 )
  {
    //Compute median speed for the track
    vcl_vector< double >::iterator areasIter = areas.begin()
                                              + areas.size()/2;
    vcl_nth_element( areas.begin(), areasIter, areas.end() );

    if( *areasIter > max_median_area_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (max_median_area_) : "
               << *areasIter << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  //6th condition: remove tracks with too large 'median' amhi bboxes
  if( max_median_area_amhi_ >= 0.0 )
  {
    //Compute median speed for the track
    vcl_vector< double >::iterator areasIter = areas_amhi.begin()
                                              + areas_amhi.size()/2;
    vcl_nth_element( areas_amhi.begin(), areasIter, areas_amhi.end() );

    // We have to 'project' the median amhi area into world_units because
    // max_median_area_amhi_ is in world units. We get the image to
    // world 'scale factor' from the area of image_object.

    // ASSUMPTION: image_object.area_ is already in world_units.
    obj = trk->get_end_object();
    double scale_factor = obj->area_ / vgl_area( obj->boundary_ ); // m/pix
    double median_area_world = *areasIter * scale_factor * scale_factor;

    if(  median_area_world > max_median_area_amhi_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (max_median_area_amhi_) : "
               << median_area_world << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  if( max_aspect_ratio_amhi_ >= 0.0 )
  {
    vcl_vector< double >::iterator medianIter = aspect_ratios_amhi.begin()
                                              + aspect_ratios_amhi.size()/2;
    vcl_nth_element( aspect_ratios_amhi.begin(),
                     medianIter, aspect_ratios_amhi.end() );
    if( *medianIter > max_aspect_ratio_amhi_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (max_aspect_ratio_amhi_) : "
               << *medianIter << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }

  if( min_occupied_bbox_amhi_ >= 0.0 )
  {
    vcl_vector< double >::iterator medianIter = bbox_occupancy_amhi.begin()
                                              + bbox_occupancy_amhi.size()/2;
    vcl_nth_element( bbox_occupancy_amhi.begin(),
                     medianIter, bbox_occupancy_amhi.end() );
    if( *medianIter > min_occupied_bbox_amhi_ )
    {
#ifdef DEBUG
      vcl_cout << "Filtering (min_occupied_bbox_amhi_) : "
        << *medianIter << ", Track # " << trk->id() << vcl_endl;
#endif
      return true;
    }
  }
  return false;
}

} // end namespace vidtk
