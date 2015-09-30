/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "visualize_world_super_process.h"

#include <pipeline/sync_pipeline.h>
#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/transform_vidtk_homography_process.h>
#include <utilities/log.h>
#include <video/draw_on_image_process.h>
#include <video/warp_image_process.h>
#include <video/image_list_writer_process.h>
#include <vcl_sstream.h>

namespace vidtk
{

// ----------------------------------------------------------------
template< class PixType>
class visualize_world_super_process_impl
{
public:
  typedef transform_vidtk_homography_process< timestamp, plane_ref_t,
                                              timestamp, plane_ref_t >
          transform_gnd_ptype; 

  // Pipeline processes
  process_smart_pointer< transform_gnd_ptype > proc_trans_for_vis_wld;
  process_smart_pointer< warp_image_process< PixType > > proc_gen_vis_wld;
  process_smart_pointer< draw_on_image_process< PixType > > proc_frame_drawer;
  process_smart_pointer< image_list_writer_process< PixType > > proc_vis_wld_writer;

  // Configuration parameters
  config_block config;
  bool disabled;

  //------------------------------------------------------------------------

  visualize_world_super_process_impl()
  : proc_trans_for_vis_wld( NULL ),
    proc_gen_vis_wld( NULL ),
    proc_frame_drawer( NULL ),
    proc_vis_wld_writer( NULL ),
    disabled( true )
  { 
    this->create_process_configs();
  }

  //------------------------------------------------------------------------

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    proc_trans_for_vis_wld = new transform_gnd_ptype( "trans_for_vis_wld" );
    config.add_subblock( proc_trans_for_vis_wld->params(), 
                         proc_trans_for_vis_wld->name() );

    proc_gen_vis_wld = new warp_image_process< PixType >( "gen_vis_wld" );
    config.add_subblock( proc_gen_vis_wld->params(), 
                         proc_gen_vis_wld->name() );

    proc_frame_drawer = new draw_on_image_process< PixType >( "frame_drawer" );
    config.add_subblock( proc_frame_drawer->params(), 
                         proc_frame_drawer->name() );

    proc_vis_wld_writer = new image_list_writer_process< PixType >( "vis_wld_writer" );
    config.add_subblock( proc_vis_wld_writer->params(), 
                         proc_vis_wld_writer->name() );
  }

// ----------------------------------------------------------------
/** Conenct pipeline.
 *
 * This method creates a new pipeline and connects all the processes.
 *
 * @param[out] pp - pointer to the newly created pipeline.
 */
  void setup_pipeline( pipeline_sptr & pp )
  {
    sync_pipeline* p = new sync_pipeline();
    pp = p;

    p->add( proc_trans_for_vis_wld );
    // Gets input data from the sp input port(s). 

    p->add( proc_gen_vis_wld );
    p->connect( proc_trans_for_vis_wld->inv_bare_homography_port(),
                proc_gen_vis_wld->set_destination_to_source_homography_port() );

    p->add( proc_frame_drawer );
    p->connect( proc_gen_vis_wld->warped_image_port(),
                proc_frame_drawer->set_source_image_port() );
    // Gets input data from the sp input port(s).

    p->add( proc_vis_wld_writer );
    p->connect( proc_frame_drawer->image_port(),
                proc_vis_wld_writer->set_image_port() );
    // Gets input data from the sp input port(s). 

  } // setup_pipeline()

  //-----------------------------------------------------------------------

};


// ----------------------------------------------------------------
/** Costructor.
 *
 *
 */
template< class PixType>
visualize_world_super_process<PixType>
::visualize_world_super_process( vcl_string const& name )
  : super_process( name, "visualize_world_super_process" ),
    impl_( new visualize_world_super_process_impl<PixType> )
{
  impl_->config.add_parameter( "disabled", "true",
    "Whether to run this super process or not?" );
}


template< class PixType>
visualize_world_super_process<PixType>
::~visualize_world_super_process()
{
  delete impl_;
}


template< class PixType>
config_block
visualize_world_super_process<PixType>
::params() const
{
  return impl_->config;
}


template< class PixType>
bool
visualize_world_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", impl_->disabled );

    impl_->setup_pipeline( this->pipeline_ );

    impl_->config.update( blk );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value( " unable to set pipeline parameters." );
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );

    return false;
  }

  return true;
}

template< class PixType>
process::step_status
visualize_world_super_process<PixType>
::step2()
{
  if( impl_->disabled )
  {
    log_warning( this->name() << " is disabled and will stop running.\n" );
    return FAILURE;
  }

  return this->pipeline_->execute();
}

// ---------------------------------------------------------
// Process inputs
template< class PixType>
void
visualize_world_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->proc_vis_wld_writer->set_timestamp( ts );
}


template< class PixType>
void
visualize_world_super_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  impl_->proc_gen_vis_wld->set_source_image( img );
}


template< class PixType>
void
visualize_world_super_process< PixType >
::set_src_to_wld_homography(image_to_plane_homography const& H )
{
  impl_->proc_trans_for_vis_wld->set_source_homography( H );
}

template< class PixType>
void
visualize_world_super_process< PixType >
::set_world_units_per_pixel( double gsd )
{
  impl_->proc_frame_drawer->set_world_units_per_pixel( gsd );
}


} // end namespace vidtk
