/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_reader_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vcl_cassert.h>
#include <vcl_fstream.h>
#include <vnl/vnl_double_3x3.h>


namespace vidtk
{


homography_reader_process
::homography_reader_process( vcl_string const& name )
  : process( name, "homography_reader_process" ),
    version_( 1 )
{
  config_.add_parameter( "textfile", 
    "",
    "The file from which to read the homography." );

  config_.add_parameter( "version", 
    "1",
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
    blk.get( "textfile", input_filename_ );
    blk.get( "version", version_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
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
      log_error( "Couldn't open \"" << input_filename_
                 << "\" for reading\n" );
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
        log_error( "Failed to read homography\n" );
        return false;
      }
    }
  }

  img_to_world_H_.set( M );
  world_to_img_H_ = img_to_world_H_.get_inverse();

  return true;
}

/// Input port(s)

void 
homography_reader_process
::set_timestamp( timestamp const & /*ts*/ )
{
  // Currently don't need to do anything. This port is merely used to add 
  // execution dependency in an async pipeline.
}

/// Output port(s)

vgl_h_matrix_2d<double> const&
homography_reader_process
::image_to_world_homography() const
{
  return img_to_world_H_;
}


vgl_h_matrix_2d<double> const&
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
  wtih.set_transform(img_to_world_H_);
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
  return itwh;
}

image_to_image_homography
homography_reader_process
::world_to_image_vidtk_homography_image() const
{
  image_to_image_homography wtih;
  wtih.set_transform(img_to_world_H_);
  wtih.set_dest_reference(time_);
  return wtih;
}

} // end namespace vidtk
