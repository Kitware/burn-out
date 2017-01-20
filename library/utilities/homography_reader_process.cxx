/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_reader_process.h"

#include <cassert>
#include <fstream>
#include <vnl/vnl_double_3x3.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_homography_reader_process_cxx__
VIDTK_LOGGER("homography_reader_process_cxx");



namespace vidtk
{


homography_reader_process
::homography_reader_process( std::string const& _name )
  : process( _name, "homography_reader_process" ),
    version_( 2 )
{
  config_.add_parameter( "textfile",
    "",
    "The file from which to read the homography." );

  config_.add_parameter( "version",
    "2",
    "1 or 2. 1 reads in a file containing a 3x3 homography for each frame. "
    "2 reads in a file containing a frame number, a timestamp, and a 3x3 "
    "homography for each frame" );
}


homography_reader_process
::~homography_reader_process()
{
}


config_block
homography_reader_process
::params() const
{
  return config_;
}


bool
homography_reader_process
::set_params( config_block const& blk )
{
  try
  {
    input_filename_ = blk.get<std::string>( "textfile" );
    version_ = blk.get<unsigned>( "version" );
    if(version_ != 1 && version_ != 2)
    {
      throw config_block_parse_error("The version number is not 1 or 2");
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
homography_reader_process
::initialize()
{
  if( !input_filename_.empty()  )
  {
    homog_str_.open( input_filename_.c_str() );
    if( ! homog_str_ )
    {
      LOG_ERROR( "Couldn't open \"" << input_filename_
                 << "\" for reading" );
      return false;
    }
  }

  img_to_world_H_.set_identity();
  world_to_img_H_.set_identity();

  return true;
}


bool
homography_reader_process
::step()
{
  if( input_filename_.empty() )
  {
    return true;
  }

  // Read in the next homography
  if( version_ == 2 )
  {
    unsigned frame_num;
    double time_secs;
    homog_str_ >> frame_num;
    homog_str_ >> time_secs;
    time_.set_time(time_secs*1e6);
    time_.set_frame_number(frame_num);
  }

  vnl_double_3x3 M;
  for( unsigned i = 0; i < 3; ++i )
  {
    for( unsigned j = 0; j < 3; ++j )
    {
      if( ! (homog_str_ >> M(i,j)) )
      {
        LOG_ERROR( "Failed to read homography" );
        return false;
      }
    }
  }

  img_to_world_H_.set( M );

  if( M.is_identity( 1.0e-5 ) )
  {
    reference_ = time_;
  }

  world_to_img_H_ = img_to_world_H_.get_inverse();

  return true;
}

/// Input port(s)

void
homography_reader_process
::set_timestamp( timestamp const & ts )
{
  time_ = ts;
}

/// Output port(s)

vgl_h_matrix_2d<double>
homography_reader_process
::image_to_world_homography() const
{
  return img_to_world_H_;
}


vgl_h_matrix_2d<double>
homography_reader_process
::world_to_image_homography() const
{
  return world_to_img_H_;
}

image_to_plane_homography
homography_reader_process
::image_to_world_vidtk_homography_plane() const
{
  image_to_plane_homography itwh;
  itwh.set_transform(img_to_world_H_);
  itwh.set_source_reference(time_);
  return itwh;
}

plane_to_image_homography
homography_reader_process
::world_to_image_vidtk_homography_plane() const
{
  plane_to_image_homography wtih;
  wtih.set_transform(world_to_img_H_);
  wtih.set_dest_reference(time_);
  return wtih;
}

image_to_image_homography
homography_reader_process
::image_to_world_vidtk_homography_image() const
{
  image_to_image_homography itwh;
  itwh.set_transform(img_to_world_H_);
  itwh.set_source_reference(time_);
  itwh.set_dest_reference(reference_);
  return itwh;
}

image_to_image_homography
homography_reader_process
::world_to_image_vidtk_homography_image() const
{
  image_to_image_homography wtih;
  wtih.set_transform(world_to_img_H_);
  wtih.set_source_reference(reference_);
  wtih.set_dest_reference(time_);
  return wtih;
}

} // end namespace vidtk
