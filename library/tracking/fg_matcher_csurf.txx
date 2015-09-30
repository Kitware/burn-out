/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "fg_matcher_csurf.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <boost/any.hpp>

namespace vidtk
{

// Static members

template < class PixType >
process_smart_pointer< external_proxy_process >
fg_matcher_csurf< PixType >::bridge_proc_( new external_proxy_process( "matlab_bridge" ) );

template < class PixType >
fg_matcher_csurf_params fg_matcher_csurf< PixType >::params_;

/********************************************************************/

template < class PixType >
fg_matcher_csurf< PixType >
::fg_matcher_csurf()
  : matched_index_( 0 )
{
}

/********************************************************************/

template < class PixType >
fg_matcher_csurf< PixType >
::~fg_matcher_csurf()
{
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_csurf< PixType >
::cleanup_model( track_sptr track )
{
  external_proxy_process::data_map_type data;
  data["function"] = vcl_string( "close_track" );
  data["track"] = track;
  bridge_proc_->set_inputs( data );

  if( !bridge_proc_->step() )
  {
    log_error( "Could not close external entry.\n" );
    return false;
  }

  return true;
}

/********************************************************************/

template < class PixType >
config_block
fg_matcher_csurf< PixType >
::params()
{
  config_block blk = params_.config_;
  
  blk.add_subblock( bridge_proc_->params(), bridge_proc_->name() );

  return blk;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_csurf< PixType >
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "padding_factor", params_.padding_factor_ );

    bool dflag = blk.get<bool>("dflag");
    params_.mc_.dflag = dflag ? 1.0 : 0.0;

    bool show = blk.get<bool>("show");
    params_.mc_.show = show ? 1.0 : 0.0;

    params_.mc_.step = blk.get<double>("step");
    params_.mc_.scale = blk.get<double>("scale");
    params_.mc_.matchratio = blk.get<double>("matchratio");
    params_.mc_.predictionlength = blk.get<double>("predictionlength");
    params_.mc_.lostnumdescriptors = blk.get<double>("lostnumdescriptors");
    params_.mc_.lostlowthreshold = blk.get<double>("lostlowthreshold");
    params_.mc_.losthighthreshold = blk.get<double>("losthighthreshold");
    params_.mc_.updatethreshold = blk.get<double>("updatethreshold");
    params_.mc_.stretch = blk.get<double>("stretch");
  }
  catch( const vidtk::config_block_parse_error &e )
  {
    log_error( "fg_matcher_csurf: Unable to set parameters: "
               << e.what() << vcl_endl );
    return false;
  }

  // Pass in all the config values to the external library
  if( !bridge_proc_->set_params( blk.subblock( bridge_proc_->name() ) ) )
    return false;

  params_.config_.update(blk);

  return true;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_csurf< PixType >
::initialize()
{
  if( !bridge_proc_->initialize() )
  {
    log_error( "Could not initialize matalb_bridge process.\n" );
    return false;
  }

  return true;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_csurf< PixType >
::create_model( track_sptr track,
                typename fg_matcher<PixType>::image_buffer_t const& img_buff,
                timestamp const & curr_ts )
{
  timestamp const& ts = track->last_state()->time_;
  bool found;
  vil_image_view< PixType > const& im = img_buff.find_datum( ts, found, true );
  if( !found )
  {
    log_error( "Could not find image for last track state in the buffer.\n" );
    return false;
  }

  // Construct map to call the external function.
  external_proxy_process::data_map_type data;
  data["function"] = vcl_string( "init_model" );
  data["track"] = track;
  data["full_frame"] = im;
  data["matlab_config"] = params_.mc_;

  bridge_proc_->set_inputs( data );

  if( !bridge_proc_->step() )
    return false;

  // No output to process.

  return true;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_csurf< PixType >
::find_matches( track_sptr track,
                vgl_box_2d<unsigned> const & predicted_bbox,
                vil_image_view<PixType> const & curr_im,
                vcl_vector< vgl_box_2d<unsigned> > &out_bboxes ) const
{
  bool match_found = false;

  // Make sure that car chip is padded with 0's when predicted point is not
  // in the center.

  // Construct map to call the external function.
  external_proxy_process::data_map_type data;
  data["function"] = vcl_string( "find_matches" );
  data["track"] = track;
  data["predicted_box"] = predicted_bbox;
  data["full_frame"] = curr_im;

  bridge_proc_->set_inputs( data );

  if( !bridge_proc_->step() )
    return false;

  // Processing output(s)
  external_proxy_process::data_map_type & out_data = bridge_proc_->outputs();
  external_proxy_process::data_map_type::iterator data_it = out_data.find( "match_found" );
  if( data_it == out_data.end() )
  {
    log_error( "Unable to retrieve match_found from the output map"
               << vcl_endl );
    return false;
  }
  match_found = boost::any_cast< bool >( data_it->second );

  if( match_found )
  {
    data_it = out_data.find( "boxes" );
    if( data_it == out_data.end() )
    {
      log_error( "Unable to retrieve boxes from the output map"
                 << vcl_endl );
      return false;
    }
    out_bboxes = boost::any_cast< vcl_vector< vgl_box_2d<unsigned> > >( data_it->second );

    data_it = out_data.find( "scores" );
    if( data_it == out_data.end() )
    {
      log_error( "Unable to retrieve scores from the output map"
                 << vcl_endl );
      return false;
    }
    vcl_vector< double > out_scores = boost::any_cast< vcl_vector< double > >( data_it->second );

    data_it = out_data.find( "locations" );
    if( data_it == out_data.end() )
    {
      log_error( "Unable to retrieve locations from the output map"
                 << vcl_endl );
      return false;
    }
    vcl_vector< vgl_point_2d< unsigned > > out_locations = boost::any_cast<
      vcl_vector< vgl_point_2d<unsigned> > >( data_it->second );

    // TODO: Pass locations and scores to the right place.
  }

  return match_found;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_csurf< PixType >
::update_model( track_sptr track,
                typename fg_matcher<PixType>::image_buffer_t const & img_buff,
                timestamp const & curr_ts  )
{  
  // TODO: Add a condition that only call update on the frame where
  // find_matches() was called last and not on the following frames.
  if( curr_ts != track->last_state()->time_ )
  {
    return false;
  }

  // Construct map to call the external function.
  external_proxy_process::data_map_type data;
  data["function"] = vcl_string( "update_model" );
  data["track"] = track;

  // TODO: Update matched_index_ in da_tracker_process.
  data["matched_index"] = matched_index_;

  bridge_proc_->set_inputs( data );

  if( !bridge_proc_->step() )
    return false;

  // No output to process.

  return true;
}

/********************************************************************/

template < class PixType >
typename fg_matcher< PixType >::sptr_t
fg_matcher_csurf< PixType >
::deep_copy()
{
  typename fg_matcher< PixType >::sptr_t cp =
    typename fg_matcher< PixType >::sptr_t( new fg_matcher_csurf< PixType > );

  fg_matcher_csurf< PixType > * csurf_ptr = dynamic_cast< fg_matcher_csurf< PixType > * >( cp.get() );
  *csurf_ptr = *this;

  return cp;
}

/********************************************************************/

} // namespace vidtk
