/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "frame_metadata_super_process.h"

#include <utilities/string_reader_process.h>
#include <utilities/split_string_process.h>
#include <video_io/frame_metadata_decoder_process.h>
#include <video_io/filename_frame_metadata_process.h>

#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/sync_pipeline.h>
#include <pipeline_framework/async_pipeline.h>

#ifdef USE_ANGEL_FIRE
#include <video_io/angel_fire_reader_source_process.h>
#endif

// initialize logger
#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_frame_metadata_super_process_txx__
VIDTK_LOGGER( "frame_metadata_super_process" );

namespace vidtk
{

template< class PixType >
class frame_metadata_super_process_impl
{
public:

  /////////////////////////////////////////////////////////////////////////////
  // Vars
  typedef super_process_pad_impl< vidtk::timestamp >          pad_out_timestamp_t;
  typedef super_process_pad_impl< vidtk::video_metadata >     pad_out_metadata_t;
  typedef super_process_pad_impl< vil_image_view< PixType > > pad_out_image_t;

  enum decoder_mode {
    DECODER_EXTERNAL_METADATA = 1, ///< Metadata from an external source
    DECODER_INTERNAL_METADATA,     ///< Metadata from the image source
    DECODER_ANGLE_FIRE             ///< Support for angel fire file format
  };

  process_smart_pointer< pad_out_timestamp_t > pad_out_timestamp;
  process_smart_pointer< pad_out_metadata_t >  pad_out_metadata;
  process_smart_pointer< pad_out_image_t >     pad_out_image;

  process_smart_pointer< string_reader_process >                      proc_string_reader;
  process_smart_pointer< frame_metadata_decoder_process< PixType > >  proc_frame_metadata_decoder;
  process_smart_pointer< filename_frame_metadata_process< PixType > > proc_filename_frame_metadata;
#ifdef USE_ANGEL_FIRE
  process_smart_pointer< angel_fire_reader_source_process< PixType > >  proc_angel_fire_reader;
#endif
  process_smart_pointer< split_string_process >                       proc_string_splitter;

  config_block config_;

  /////////////////////////////////////////////////////////////////////////////
  // Methods

  frame_metadata_super_process_impl()
  : pad_out_timestamp( NULL )
   ,pad_out_metadata( NULL )
   ,pad_out_image( NULL )
   ,proc_string_reader( NULL )
   ,proc_frame_metadata_decoder( NULL )
   ,proc_filename_frame_metadata( NULL )
#ifdef USE_ANGEL_FIRE
   ,proc_angel_fire_reader(NULL)
#endif
   ,proc_string_splitter( NULL )
  {
    // Initialize pipeline process nodes and pads
    pad_out_timestamp = new pad_out_timestamp_t( "timestamp_out_pad" );
    pad_out_metadata = new pad_out_metadata_t( "metadata_out_pad" );
    pad_out_image = new pad_out_image_t( "image_out_pad" );

    #ifdef USE_ANGEL_FIRE
    proc_angel_fire_reader = new angel_fire_reader_source_process< PixType >("angel_fire");
    #endif

    proc_string_reader = new string_reader_process( "list_reader" );
    proc_frame_metadata_decoder =
      new frame_metadata_decoder_process< PixType >( "frame_metadata_decoder" );
    proc_filename_frame_metadata =
      new filename_frame_metadata_process< PixType >( "filename_frame_metadata" );
    proc_string_splitter = new split_string_process( "string_splitter" );

    config_.add_parameter(
      "decoder_type", "frame_metadata_decoder",
      "Valid choices: frame_metadata_decoder, filename_frame_metadata, or angel_fire. "
      "Where frame_metadata_decoder - reads file names from a port, gets metadata from external file. "
      "filename_frame_metadata - reads file names from a port, decodes metadata from image data. "
      "Refer to specific process config for details."
      "angel_fire - reads angel fire input files and decodes its metadata.  "
);

    // Create initial config_ settings, pulling from created proc objects as
    // sub-blocks.
    config_.add_subblock( proc_string_reader->params(),
                          proc_string_reader->name() );
    config_.add_subblock( proc_frame_metadata_decoder->params(),
                          proc_frame_metadata_decoder->name() );
    config_.add_subblock( proc_filename_frame_metadata->params(),
                          proc_filename_frame_metadata->name() );
    config_.add_subblock( proc_string_splitter->params(),
                          proc_string_splitter->name() );

    #ifdef USE_ANGEL_FIRE
    config_.add_subblock( proc_angel_fire_reader->params(),
                          proc_angel_fire_reader->name() );
    #endif
  }


  virtual ~frame_metadata_super_process_impl()
  { }

    // ----------------------------------------------------------------
  template< class Pipeline >
  void
  setup_pipeline( Pipeline *p )
  {
    vidtk::config_enum_option metadata_source;
    metadata_source
      .add( "frame_metadata_decoder",  DECODER_EXTERNAL_METADATA )
      .add( "filename_frame_metadata", DECODER_INTERNAL_METADATA )
      .add( "angel_fire", DECODER_ANGLE_FIRE );

    p->add( pad_out_timestamp );
    p->add( pad_out_metadata );
    p->add( pad_out_image );

    // add option to read filenames from a file
    p->add( proc_string_reader );

    int decoder_type;
    try
    {
      // Get option and validate against table
      decoder_type = config_.get< decoder_mode >( metadata_source, "decoder_type");
    }
    catch( config_block_parse_error const& e)
    {
      throw( config_block_parse_error( "Invalid parameter value decoder_type" ) );
    }


    // determine file name source. Instantiate either file string reader or TCP string reader.
    // It would be nice if they had the same base class.

    //-if( decoder_type == "frame_metadata_decoder" )
    if( decoder_type == DECODER_EXTERNAL_METADATA )
    {
      p->add( proc_frame_metadata_decoder );
      p->connect( proc_string_reader->str_port(),                proc_frame_metadata_decoder->set_input_image_file_port() );
      p->connect( proc_frame_metadata_decoder->timestamp_port(), pad_out_timestamp->set_value_port() );
      p->connect( proc_frame_metadata_decoder->metadata_port(),  pad_out_metadata->set_value_port() );
      p->connect( proc_frame_metadata_decoder->image_port(),     pad_out_image->set_value_port() );
    }
    else if(decoder_type == DECODER_ANGLE_FIRE)
    {
#ifdef USE_ANGEL_FIRE
      p->add( proc_angel_fire_reader );
      p->connect( proc_string_reader->str_port(),           proc_angel_fire_reader->set_filename_port() );
      p->connect( proc_angel_fire_reader->timestamp_port(), pad_out_timestamp->set_value_port() );
      p->connect( proc_angel_fire_reader->metadata_port(),  pad_out_metadata->set_value_port() );
      p->connect( proc_angel_fire_reader->image_port(),     pad_out_image->set_value_port() );
#else
      // There is no name() in _impl, proper identification should be done
      // by catching code.
      throw( config_block_parse_error( "Not built with angel fire support" ) );
#endif
    }
    else if( decoder_type == DECODER_INTERNAL_METADATA )
    {
      p->add( proc_filename_frame_metadata );

      std::string crop_mode = config_.get< std::string >(proc_filename_frame_metadata->name() + ":crop_mode");
      if(crop_mode == "pixel_input") //pixel string hook up
      {
        // splits string into filename and crop region
        // filename contains frame number
        p->add( proc_string_splitter );
        p->connect( proc_string_reader->str_port(),  proc_string_splitter->set_input_port() );
        p->connect( proc_string_splitter->group1_port(), proc_filename_frame_metadata->set_filename_port() );
        p->connect( proc_string_splitter->group2_port(), proc_filename_frame_metadata->set_pixel_roi_port() );
      }
      else //all other hook ups
      {
        p->connect( proc_string_reader->str_port(), proc_filename_frame_metadata->set_filename_port() );
      }

      p->connect( proc_filename_frame_metadata->timestamp_port(),  pad_out_timestamp->set_value_port() );
      p->connect( proc_filename_frame_metadata->metadata_port(),   pad_out_metadata->set_value_port() );
      p->connect( proc_filename_frame_metadata->image_port(),      pad_out_image->set_value_port() );
    }

  } // setup_pipeline()

}; // class frame_metadata_super_process_impl


template< class PixType >
frame_metadata_super_process< PixType >
::frame_metadata_super_process( std::string const& _name )
  : super_process( _name, "frame_metadata_super_process" )
  , impl_(NULL)
{
  this->impl_ = new frame_metadata_super_process_impl< PixType >();
}

template< class PixType >
frame_metadata_super_process< PixType >
::~frame_metadata_super_process()
{
  delete this->impl_;
}


template< class PixType >
config_block
frame_metadata_super_process< PixType >
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config_;
  return tmp_config;
}


template< class PixType >
bool
frame_metadata_super_process< PixType >
::set_params( config_block const& blk )
{
  this->impl_->config_.update( blk );

  // Only producing a sync pipeline
  sync_pipeline *p = new sync_pipeline();
  this->pipeline_ = p;
  try
  {
    this->impl_->setup_pipeline( p );
  }
  catch( config_block_parse_error const & e)
  {
    LOG_ERROR( name() << ": " << e.what() );
    return false;
  }

  if( ! this->pipeline_->set_params( impl_->config_ ) )
  {
    LOG_ERROR( this->name() << ": failed to set super_process pipeline "
               "parameters!" );
    return false;
  }

  return true;
}


template< class PixType >
process::step_status
frame_metadata_super_process< PixType >
::step2()
{
  return this->pipeline_->execute();
}


template< class PixType >
vidtk::timestamp
frame_metadata_super_process< PixType >
::timestamp() const
{
  return this->impl_->pad_out_timestamp->value();
}


template< class PixType >
vidtk::video_metadata
frame_metadata_super_process< PixType >
::metadata() const
{
  return this->impl_->pad_out_metadata->value();
}


template< class PixType >
vil_image_view< PixType >
frame_metadata_super_process< PixType >
::image() const
{
  return this->impl_->pad_out_image->value();
}


template< class PixType >
bool
frame_metadata_super_process< PixType >
::seek( unsigned frame_number )
{
  return this->impl_->proc_frame_metadata_decoder->seek( frame_number );
}


template< class PixType >
double
frame_metadata_super_process< PixType >
::frame_rate() const
{
  return this->impl_->proc_frame_metadata_decoder->frame_rate();
}


template< class PixType >
unsigned
frame_metadata_super_process< PixType >
::nframes() const
{
  return this->impl_->proc_frame_metadata_decoder->nframes();
}


template< class PixType >
unsigned
frame_metadata_super_process< PixType >
::ni() const
{
  return this->impl_->proc_frame_metadata_decoder->ni();
}


template< class PixType >
unsigned
frame_metadata_super_process< PixType >
::nj() const
{
  return this->impl_->proc_frame_metadata_decoder->nj();
}


}
