/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_shot_stitching_algo.h"

#include <utilities/config_block.h>
#include <pipeline_framework/sync_pipeline.h>

#include <kwklt/klt_pyramid_process.h>
#include <kwklt/klt_tracking_process.h>

#include <tracking/homography_process.h>
#include <logger/logger.h>


//
// shot stitching is done by creating a small local KLT pipeline.
//

namespace vidtk
{

VIDTK_LOGGER ("klt_shot_stitching_algo");

template< typename PixType >
class klt_shot_stitching_algo_impl
{
public:

  klt_shot_stitching_algo_impl();
  ~klt_shot_stitching_algo_impl();

  config_block params() const { return cfg; }
  bool set_params( const config_block& blk )
  {
    try
    {
      this->stitch_threshold = blk.get<float>( "threshold" );
      if( this->stitch_threshold < 0 || this->stitch_threshold > 1.0 )
      {
        throw config_block_parse_error( "Only expect the 'threshold' to be within"
          " [0,1] range." );
      }

      feature_count = blk.get<unsigned>( proc_klt_tracking->name()
        + ":feature_count" );
      if( feature_count == 0 )
      {
        throw config_block_parse_error( "Do not expect '0' for feature_count." );
      }

      cfg.update( blk );
    }
    catch (config_block_parse_error const& e )
    {
      LOG_ERROR( "klt_shot_stitching_algo: param error: " << e.what() );
      return false;
    }

    return true;
  }

  image_to_image_homography stitch( const vil_image_view < PixType >& src,
                                    const vil_image_view < bool >& src_mask,
                                    const vil_image_view < PixType >& dest,
                                    const vil_image_view < bool >& dest_mask);

private:

  bool create_pipeline();

  sync_pipeline* p;
  config_block cfg;

  process_smart_pointer< klt_pyramid_process< PixType > > proc_klt_pyramid;
  process_smart_pointer< klt_tracking_process > proc_klt_tracking;
  process_smart_pointer< homography_process > proc_compute_homog;

  unsigned feature_count;
  float stitch_threshold;
};


template< typename PixType >
klt_shot_stitching_algo_impl<PixType>
::klt_shot_stitching_algo_impl()
  : p( 0 ),
    proc_klt_pyramid( new klt_pyramid_process< PixType >( "ssp_klt_pyramid" )),
    proc_klt_tracking( new klt_tracking_process( "ssp_klt_tracking" )),
    proc_compute_homog( new homography_process( "ssp_compute_homog" )),
    stitch_threshold( -1.0f )
{
  cfg.add_subblock( proc_klt_pyramid->params(), proc_klt_pyramid->name() );
  cfg.add_subblock( proc_klt_tracking->params(), proc_klt_tracking->name() );
  cfg.add_subblock( proc_compute_homog->params(), proc_compute_homog->name() );

  cfg.add_parameter( "threshold", "0.5", "Limit for fraction of terminated tracks, above which"
                     " a stitch can not be made. A value between 0 and 1.");
}


template< typename PixType >
klt_shot_stitching_algo_impl<PixType>
::~klt_shot_stitching_algo_impl()
{
  delete p;
}


// ----------------------------------------------------------------
template< typename PixType >
bool
klt_shot_stitching_algo_impl<PixType>
::create_pipeline()
{
  p = new sync_pipeline();

  p->add( proc_klt_pyramid );
  p->add( proc_klt_tracking );
  p->add( proc_compute_homog );

  p->connect( proc_klt_pyramid->image_pyramid_port(), proc_klt_tracking->set_image_pyramid_port() );
  p->connect( proc_klt_pyramid->image_pyramid_gradx_port(), proc_klt_tracking->set_image_pyramid_gradx_port() );
  p->connect( proc_klt_pyramid->image_pyramid_grady_port(), proc_klt_tracking->set_image_pyramid_grady_port() );

  p->connect( proc_klt_tracking->created_tracks_port(), proc_compute_homog->set_new_tracks_port() );
  p->connect( proc_klt_tracking->active_tracks_port(), proc_compute_homog->set_updated_tracks_port() );

  return p->set_params( cfg ) && p->initialize();
}


// ----------------------------------------------------------------
template< typename PixType >
image_to_image_homography
klt_shot_stitching_algo_impl<PixType>
::stitch( const vil_image_view< PixType >& src_img,
          const vil_image_view< bool >& src_mask_img,
          const vil_image_view< PixType >& dest_img,
          const vil_image_view< bool >& dest_mask_img )
{
  image_to_image_homography H_src2dst;

  if( this->stitch_threshold == -1.0f )
  {
    // It is assumed that the set_params() has been called once before
    // this function is being called. Trying to copy the default parameter
    // values into the parameters.
    if( ! this->set_params( this->params() ) )
    {
      LOG_ERROR( "klt_shot_stitching_algo: set_params() failed with the default"
        " values. Please consider setting the valid parameters before stitch()." );
      return H_src2dst;
    }
  }

  if ( ! p )
  {
    if( ! create_pipeline() )
    {
      LOG_ERROR( "klt_shot_stitching_algo: could not setup pipeline." );
      return H_src2dst;
    }
  }
  else if( ! p->reset() )
  {
    LOG_ERROR( "klt_shot_stitching_algo: pipeline reset failed" );
    return H_src2dst;
  }

  // move the first image through at notional timestamp 1
  proc_klt_pyramid->set_image( src_img );
  proc_klt_tracking->set_timestamp( vidtk::timestamp( 1.0, 1 ) );
  proc_compute_homog->set_mask_image(src_mask_img);

  process::step_status step_1_status = p->execute();
  if ( step_1_status != process::SUCCESS )
  {
    LOG_INFO( "klt_shot_stitching_algo: stitch step 1 failed" );
    return H_src2dst;
  }

  LOG_DEBUG("klt_shot_stitching_algo: (1) Created tracks: " << proc_klt_tracking->created_tracks().size()
            << "   terminated tracks: " << proc_klt_tracking->terminated_tracks().size()
            << "   active tracks: " << proc_klt_tracking->active_tracks().size() );

  size_t first_active_count = proc_klt_tracking->active_tracks().size();

  // move the second image through at notional timestamp 2
  proc_klt_pyramid->set_image( dest_img );
  proc_klt_tracking->set_timestamp( vidtk::timestamp( 2.0, 2 ) );
  proc_compute_homog->set_mask_image(dest_mask_img);

  process::step_status step_2_status = p->execute();
  if (step_2_status != process::SUCCESS )
  {
    LOG_INFO( "klt_shot_stitching_algo: stitch step 2 failed" );
    return H_src2dst;
  }

  LOG_DEBUG("klt_shot_stitching_algo: (2) Created tracks: " << proc_klt_tracking->created_tracks().size()
            << "   terminated tracks: " << proc_klt_tracking->terminated_tracks().size()
            << "   active tracks: " << proc_klt_tracking->active_tracks().size() );

  // if we have tracks, return whatever homography we can get.
  // FIXME: Arslan's code review comments:
  //
  // This seems to be "the condition" of whether the stitching was
  // successful or not. It is very likely that we'd need stronger
  // conditions to make sure we have low false positive stitch. It
  // should probably be delayed until after this topic branch. Maybe
  // the empirical results already show that we don't need stronger
  // tests?
  //
  // The test being the ones similar to those in shot_generator.cxx.
  //

  size_t terminated_count = proc_klt_tracking->terminated_tracks().size();

  // Percentage of feature points detected on the source image
  // feature_count: number of requested points to be detected.
  // first_active_count: number of points actually found.
  double pcent_detected = static_cast<double>(first_active_count) / feature_count;

  // Percentage of feature points for which we didn't find a match on the
  // destination frame. terminated_count is from the second step and
  // first_active_count is from the first step.
  double pcent_lost = ( first_active_count > 0 ) ?
    (static_cast<double>(terminated_count) / first_active_count) : 1.1;

  // Condition to decide whether the quality of stitch is acceptible or not
  //  (In case of 'first_active_count==0' pcent_lost will never be checked,
  //   because 'pcent_detected==0'. Using a single threshold for both conditions
  //   here in an effort not to create too many thresholds unnecessarily.)
  if ( pcent_detected > stitch_threshold && pcent_lost < stitch_threshold )
  {
    H_src2dst = proc_compute_homog->world_to_image_vidtk_homography_image();
    LOG_DEBUG( "klt_shot_stitching_algo: Succcessfully stitched the image pair." << H_src2dst );
  }
  else
  {
    LOG_DEBUG( "klt_shot_stitching_algo: Unable to stitch the image pair." );
  }

  return H_src2dst;
}


template< typename PixType >
klt_shot_stitching_algo<PixType>
::klt_shot_stitching_algo()
  : impl_( NULL )
{
  impl_ = new klt_shot_stitching_algo_impl<PixType>();
}


template< typename PixType >
klt_shot_stitching_algo<PixType>
::~klt_shot_stitching_algo()
{
  delete impl_;
}


template< typename PixType >
image_to_image_homography
klt_shot_stitching_algo<PixType>
::stitch( const vil_image_view< PixType >& src_img,
          const vil_image_view< bool >& src_mask_img,
          const vil_image_view< PixType >& dest_img,
          const vil_image_view< bool >& dest_mask_img )
{
  // forward to impl
  return impl_->stitch( src_img, src_mask_img,
                        dest_img, dest_mask_img );
}


template< typename PixType >
config_block
klt_shot_stitching_algo<PixType>
::params() const
{
  return impl_->params();
}


template< typename PixType >
bool
klt_shot_stitching_algo<PixType>
::set_params( config_block const &blk )
{
  return impl_->set_params( blk );
}


} // namespace vidtk
