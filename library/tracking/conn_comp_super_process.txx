/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "conn_comp_super_process.h"

#include <tracking/filter_image_process.h>
#include <tracking/world_morphology_process.h>
#include <tracking/world_connected_component_process.h>
#include <tracking/project_to_world_process.h>
#include <tracking/filter_image_objects_process.h>
#include <tracking/image_object_writer_process.h>
#include <tracking/transform_image_object_process.h>
#include <video/image_list_writer_process.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <pipeline/sync_pipeline.h>

namespace vidtk
{

template< class PixType >
class conn_comp_super_process_impl
{
public:
  // Processes
  process_smart_pointer< filter_image_process > proc_filter_image;
  process_smart_pointer< world_morphology_process > proc_morph1;
  process_smart_pointer< world_morphology_process > proc_morph;
  process_smart_pointer< world_connected_component_process > proc_conn_comp;
  process_smart_pointer< project_to_world_process > proc_project;
  process_smart_pointer< filter_image_objects_process< PixType > > proc_filter1;
  process_smart_pointer< image_object_writer_process > proc_output_objects;
  process_smart_pointer< image_list_writer_process<bool> > proc_morph_image_writer;
  process_smart_pointer< project_to_world_process > proc_crop_correct;
  process_smart_pointer< transform_image_object_process< PixType > > proc_add_chip;

  // I/O data
  vil_image_view<PixType> source_img;
  timestamp source_ts;
  image_to_image_homography H_src2ref;
  image_to_plane_homography H_src2wld;
  plane_to_image_homography H_wld2src;
  plane_to_utm_homography H_wld2utm;
  mutable vil_image_view< bool > copied_morph_image;
  vil_image_view< bool > fg_image;
  double world_units_per_pixel;

  // Config parameters
  config_block config;
  config_block default_config;

  conn_comp_super_process_impl()
    : proc_filter_image( NULL ),
      proc_morph1( NULL ),
      proc_morph( NULL ),
      proc_conn_comp( NULL ),
      proc_project( NULL ),
      proc_filter1( NULL ),
      proc_output_objects( NULL ),
      proc_morph_image_writer( NULL ),
      proc_crop_correct( NULL ),
      proc_add_chip(NULL),
      world_units_per_pixel( -1 )
  {}

  void create_process_configs()
  {
    proc_filter_image = new filter_image_process( "filter_image" );
    config.add_subblock( proc_filter_image->params(), proc_filter_image->name() );

    proc_crop_correct = new project_to_world_process( "correct_for_cropping" );
    config.add_subblock( proc_crop_correct->params(), proc_crop_correct->name() );

    proc_morph1 = new world_morphology_process( "morph1" );
    config.add_subblock( proc_morph1->params(), proc_morph1->name() );

    proc_morph  = new world_morphology_process( "morph2" );
    config.add_subblock( proc_morph->params(), proc_morph->name() );

    proc_conn_comp = new world_connected_component_process( "conn_comp" );
    config.add_subblock( proc_conn_comp->params(), proc_conn_comp->name() );

    proc_project = new project_to_world_process( "project" );
    config.add_subblock( proc_project->params(), proc_project->name() );

    proc_filter1 = new filter_image_objects_process< PixType >( "filter1" );
    config.add_subblock( proc_filter1->params(), proc_filter1->name() );

    proc_output_objects = new image_object_writer_process( "output_objects" );
    config.add_subblock( proc_output_objects->params(), proc_output_objects->name() );

    proc_morph_image_writer = new image_list_writer_process<bool>( "fg_image_writer" );
    config.add_subblock( proc_morph_image_writer->params(), proc_morph_image_writer->name() );

    proc_add_chip = new transform_image_object_process< PixType >( "add_image_chip",
                                                                   new add_image_clip_to_image_object< PixType >() );
    config.add_subblock( proc_add_chip->params(), proc_add_chip->name() );

    default_config.add( proc_morph_image_writer->name() + ":disabled", "true" );
    config.update( default_config );
  }

  void setup_pipeline( pipeline_sptr & pp )
  {
    sync_pipeline* p = new sync_pipeline;
    pp = p;

    p->add( proc_filter_image );
    p->add( proc_morph1 );
    p->connect( proc_filter_image->isgood_port(),
                proc_morph1->set_is_fg_good_port() );

    p->add( proc_morph );
    p->connect( proc_morph1->image_port(),
                proc_morph->set_source_image_port() );
    p->connect( proc_filter_image->isgood_port(),
                proc_morph->set_is_fg_good_port() );

    p->add( proc_conn_comp );
    p->connect( proc_filter_image->isgood_port(),
                proc_conn_comp->set_is_fg_good_port() );
    p->connect( proc_morph->image_port(),
                proc_conn_comp->set_fg_image_port() );

    p->add( proc_crop_correct );
    p->connect( proc_conn_comp->objects_port(),
                proc_crop_correct->set_source_objects_port() );

    p->add( proc_project );
    p->connect( proc_crop_correct->objects_port(),
                proc_project->set_source_objects_port() );

    p->add( proc_filter1 );
    p->connect( proc_project->objects_port(),
                proc_filter1->set_source_objects_port() );

    p->add( proc_morph_image_writer );
    p->connect( proc_morph->image_port(),
                proc_morph_image_writer->set_image_port() );

    p->add( proc_add_chip );
    p->connect( proc_filter1->objects_port(),
                proc_add_chip->set_objects_port() );

    p->add( proc_output_objects );
    p->connect( proc_add_chip->objects_port(),
                proc_output_objects->set_image_objects_port() );
  }
}; // conn_comp_super_process_impl

template< class PixType >
conn_comp_super_process<PixType>
::conn_comp_super_process( vcl_string const& name )
  : super_process( name, "conn_comp_super_process" ),
    impl_( new conn_comp_super_process_impl<PixType> )
{
  impl_->create_process_configs();
}

template< class PixType >
conn_comp_super_process<PixType>
::~conn_comp_super_process()
{
  delete impl_;
}


template< class PixType >
config_block
conn_comp_super_process<PixType>
::params() const
{
  return impl_->config;
}


template< class PixType >
bool
conn_comp_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    impl_->config.update( blk );

    impl_->setup_pipeline( this->pipeline_ );

    if( ! pipeline_->set_params( blk ) )
    {
      return false;
    }
  }
  catch( unchecked_return_value & e )
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );
    return false;
  }

  return true;
}


template< class PixType >
bool
conn_comp_super_process<PixType>
::initialize()
{
  return pipeline_->initialize();
}


// ----------------------------------------------------------------
/** Main process loop
 *
 *
 */
template< class PixType >
process::step_status
conn_comp_super_process<PixType>
::step2()
{
  log_info (name() << ": input timestamp " << impl_->source_ts << "\n");

  impl_->copied_morph_image = vil_image_view< bool >();

  return pipeline_->execute();
}


// ----------------------------------------------------------------
// -- INPUTS --
template< class PixType >
void
conn_comp_super_process<PixType>
::set_fg_image( vil_image_view<bool> const& img )
{
  impl_->fg_image = img;
  impl_->proc_filter_image->set_source_image( impl_->fg_image );
  impl_->proc_morph1->set_source_image( impl_->fg_image );
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->source_ts = ts;
  impl_->proc_output_objects->set_timestamp( impl_->source_ts );
  impl_->proc_morph_image_writer->set_timestamp( impl_->source_ts );
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_src_to_ref_homography( image_to_image_homography const& H )
{
  impl_->H_src2ref = H;
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_wld_to_src_homography( plane_to_image_homography const& H )
{
  impl_->H_wld2src = H;
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_wld_to_utm_homography( plane_to_utm_homography const& H )
{
  impl_->H_wld2utm = H;
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_src_to_wld_homography( image_to_plane_homography const& H )
{
  impl_->H_src2wld = H;
  impl_->proc_conn_comp->set_src_2_wld_homography( impl_->H_src2wld );
  impl_->proc_project->set_image_to_world_homography( impl_->H_src2wld.get_transform() );
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_world_units_per_pixel( double const& gsd )
{
  impl_->proc_morph->set_world_units_per_pixel( gsd );
  impl_->proc_morph1->set_world_units_per_pixel( gsd );
  impl_->proc_conn_comp->set_world_units_per_pixel( gsd );
  impl_->world_units_per_pixel = gsd;  // pass thru
}


// ----------------------------------------------------------------
// -- OUTPUTS --
template< class PixType >
vcl_vector< image_object_sptr > const&
conn_comp_super_process<PixType>
::output_objects() const
{
  return impl_->proc_add_chip->objects();
}

template< class PixType >
vcl_vector< image_object_sptr > const&
conn_comp_super_process<PixType>
::projected_objects() const
{
  return impl_->proc_project->objects();
}

template< class PixType >
vil_image_view<bool> const&
conn_comp_super_process<PixType>
::fg_image() const
{
  return impl_->fg_image;
}

template< class PixType >
vil_image_view<bool> const&
conn_comp_super_process<PixType>
::morph_image() const
{
  return impl_->proc_morph->image();
}

template< class PixType >
vil_image_view<bool> const&
conn_comp_super_process<PixType>
::copied_morph_image() const
{
  if( !impl_->copied_morph_image )
  {
    impl_->copied_morph_image.deep_copy( impl_->proc_morph->image() );
  }

  return impl_->copied_morph_image;
}

template< class PixType >
timestamp const&
conn_comp_super_process<PixType>
::source_timestamp() const
{
  return impl_->source_ts;
}

template< class PixType >
vil_image_view< PixType > const&
conn_comp_super_process<PixType>
::source_image() const
{
  return impl_->source_img;
}

template< class PixType >
void
conn_comp_super_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  impl_->source_img = img;
  impl_->proc_filter1->set_source_image( img );
  impl_->proc_add_chip->set_image(img);
}

template< class PixType >
image_to_image_homography const&
conn_comp_super_process<PixType>
::src_to_ref_homography() const
{
  return impl_->H_src2ref;
}

template< class PixType >
image_to_plane_homography const&
conn_comp_super_process<PixType>
::src_to_wld_homography() const
{
  return impl_->H_src2wld;
}

template< class PixType >
plane_to_image_homography const&
conn_comp_super_process<PixType>
::wld_to_src_homography() const
{
  return impl_->H_wld2src;
}

template< class PixType >
plane_to_utm_homography const&
conn_comp_super_process<PixType>
::wld_to_utm_homography() const
{
  return impl_->H_wld2utm;
}

template< class PixType >
double
conn_comp_super_process<PixType>
::world_units_per_pixel() const
{
  return impl_->world_units_per_pixel;
}

} // end namespace vidtk


// Local Variables:
// mode: c++
// fill-column: 70
// c-tab-width: 2
// c-basic-offset: 2
// c-basic-indent: 2
// c-indent-tabs-mode: nil
// end:
