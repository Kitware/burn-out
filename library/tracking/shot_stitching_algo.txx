/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shot_stitching_algo.h"

#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>
#include <pipeline/sync_pipeline.h>

#include <video/greyscale_process.h>

#include <kwklt/klt_pyramid_process.h>
#include <kwklt/klt_tracking_process.h>

#include <tracking/homography_process.h>
#include <logger/logger.h>


//
// shot stitching is done by creating a small local KLT pipeline.
//

namespace vidtk
{

VIDTK_LOGGER ("shot_stitching_algo");

template< typename PixType >
class shot_stitching_algo_impl
{
public:

  shot_stitching_algo_impl();
  ~shot_stitching_algo_impl();

  config_block params() const { return cfg; }
  bool set_params( const config_block& blk )
  {
    try {
      blk.get( "threshold", this->stitch_threshold );
      cfg.update( blk );
    }
    catch (vidtk::unchecked_return_value& e )
    {
      LOG_ERROR( "shot_stitching_algo: param error: " << e.what() );
      return false;
    }

    return true;
  }

  image_to_image_homography stitch( const vil_image_view<PixType>& src_img,
                                    const vil_image_view<PixType>& dest_img );

private:

  void create_pipeline();

  sync_pipeline* p;
  config_block cfg;

  process_smart_pointer< greyscale_process< PixType > > proc_convert_to_grey;
  process_smart_pointer< klt_pyramid_process > proc_klt_pyramid;
  process_smart_pointer< klt_tracking_process > proc_klt_tracking;
  process_smart_pointer< homography_process > proc_compute_homog;


  float stitch_threshold;
};


template< typename PixType >
shot_stitching_algo_impl<PixType>
::shot_stitching_algo_impl()
  : p( 0 ),
    proc_convert_to_grey( new greyscale_process< PixType >( "ssp_rgb_to_grey" )),
    proc_klt_pyramid( new klt_pyramid_process( "ssp_klt_pyramid" )),
    proc_klt_tracking( new klt_tracking_process( "ssp_klt_tracking" )),
    proc_compute_homog( new homography_process( "ssp_compute_homog" ))
{
  cfg.add_subblock( proc_convert_to_grey->params(), proc_convert_to_grey->name() );
  cfg.add_subblock( proc_klt_pyramid->params(), proc_klt_pyramid->name() );
  cfg.add_subblock( proc_klt_tracking->params(), proc_klt_tracking->name() );
  cfg.add_subblock( proc_compute_homog->params(), proc_compute_homog->name() );

  cfg.add_parameter( "threshold", "0.5", "Limit for fraction of terminated tracks, above which"
                     " a stitch can not be made");
}


template< typename PixType >
shot_stitching_algo_impl<PixType>
::~shot_stitching_algo_impl()
{
  delete p;
}


template< typename PixType >
void
shot_stitching_algo_impl<PixType>
::create_pipeline()
{
  p = new sync_pipeline();

  p->add( proc_convert_to_grey );
  p->add( proc_klt_pyramid );
  p->add( proc_klt_tracking );
  p->add( proc_compute_homog );

  p->connect( proc_convert_to_grey->image_port(), proc_klt_pyramid->set_image_port() );

  p->connect( proc_klt_pyramid->image_pyramid_port(), proc_klt_tracking->set_image_pyramid_port() );
  p->connect( proc_klt_pyramid->image_pyramid_gradx_port(), proc_klt_tracking->set_image_pyramid_gradx_port() );
  p->connect( proc_klt_pyramid->image_pyramid_grady_port(), proc_klt_tracking->set_image_pyramid_grady_port() );

  p->connect( proc_klt_tracking->created_tracks_port(), proc_compute_homog->set_new_tracks_port() );
  p->connect( proc_klt_tracking->active_tracks_port(), proc_compute_homog->set_updated_tracks_port() );

  p->set_params( cfg );
}

template< typename PixType >
image_to_image_homography
shot_stitching_algo_impl<PixType>
::stitch( const vil_image_view< PixType >& src_img,
          const vil_image_view< PixType >& dest_img )
{
  image_to_image_homography h;

  if ( ! p )
  {
    create_pipeline();
  }

  // initialize rather than reset, because reset doesn't seem to work
  // with klt_tracking. Ben is (10may2011) working on this.
  if ( ! p->initialize() )
  {
    LOG_ERROR( "shot_stitching_algo: initialize failed" );
    return h;
  }

  // move the first image through at notional timestamp 1
  proc_convert_to_grey->set_image( src_img );
  proc_klt_tracking->set_timestamp( vidtk::timestamp( 1.0, 1 ) );
  process::step_status step_1_status = p->execute();
  if ( step_1_status != process::SUCCESS )
  {
    LOG_INFO( "shot_stitching_algo: stitch step 1 failed" );
    return h;
  }

  LOG_DEBUG ("shot_stitching_algo: (1) Created tracks: " << proc_klt_tracking->created_tracks().size()
            << "   terminated tracks: " << proc_klt_tracking->terminated_tracks().size()
            << "   active tracks: " << proc_klt_tracking->active_tracks().size() );


  // move the second image through at notional timestamp 2
  proc_convert_to_grey->set_image( dest_img );
  proc_klt_tracking->set_timestamp( vidtk::timestamp( 2.0, 2 ) );
  process::step_status step_2_status = p->execute();
  if (step_2_status != process::SUCCESS )
  {
    LOG_INFO( "shot_stitching_algo: stitch step 2 failed" );
    return h;
  }

  LOG_DEBUG ("shot_stitching_algo: (2) Created tracks: " << proc_klt_tracking->created_tracks().size()
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

  float terminated_count = proc_klt_tracking->terminated_tracks().size();
  float active_count     = proc_klt_tracking->active_tracks().size();

  if ( (active_count > 0)
       && (terminated_count / active_count) < stitch_threshold)
  {
    h = proc_compute_homog->world_to_image_vidtk_homography_image();
  }
  return h;
}


template< typename PixType >
shot_stitching_algo<PixType>
::shot_stitching_algo()
  : impl_( NULL )
{
  impl_ = new shot_stitching_algo_impl<PixType>();
}


template< typename PixType >
shot_stitching_algo<PixType>
::~shot_stitching_algo()
{
  delete impl_;
}


template< typename PixType >
image_to_image_homography
shot_stitching_algo<PixType>
::stitch( const vil_image_view<PixType>& src,
          const vil_image_view<PixType>& dest )
{
  return impl_->stitch( src, dest );
}


template< typename PixType >
config_block
shot_stitching_algo<PixType>
::params() const
{
  return impl_->params();
}


template< typename PixType >
bool
shot_stitching_algo<PixType>
::set_params( const config_block& blk )
{
  return impl_->set_params( blk );
}



} // namespace vidtk
