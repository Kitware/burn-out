/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_writer_process.h"

#include <vcl_algorithm.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vnl/vnl_double_2.h>
#include <vgl/vgl_area.h>

namespace vidtk
{

homography_writer_process
  ::homography_writer_process( vcl_string const& name )
  : process( name, "homography_writer_process" ),
    disabled_( true ),
    FrameNumber( 0 ),
    TimeStamp( 0.0 ),
    append_file_( true )
{
  config_.add_parameter( "disabled",
    "true",
    "Do not do anything. Will write to the output file when set to false" );

  config_.add_parameter( "output_filename",
    "",
    "Ouput homographies to a file" );

  config_.add_parameter( "append",
    "true",
    "Whether to append or over-write a pre-existing file." );
}

homography_writer_process
::~homography_writer_process()
{
  if (this->filestrm.is_open())
  {
    filestrm.close();
  }
}


config_block
homography_writer_process
::params() const
{
  return config_;
}


bool
homography_writer_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get("disabled",this->disabled_);
    if( !this->disabled_ )
    {
      blk.get("output_filename",this->filename);
      blk.get("append",this->append_file_);
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( name() << ": couldn't set parameters: "<< e.what() <<"\n" );

    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
homography_writer_process
::initialize()
{
  if( !this->disabled_ && !this->filename.empty() )
  {
    this->set_output_file(this->filename);
    if( this->filestrm.fail() )
    {
      log_error( "Could not open file: " + this->filename + "\n" );
      return false;
    }
  }

  this->filestrm << vcl_fixed;

  return true;
}

void 
homography_writer_process
::set_output_file(vcl_string file)
{
  if (filestrm.is_open())
  {
    filestrm.close();
  }

  if( this->append_file_ )
  {
    filestrm.open( file.c_str(), vcl_ofstream::app );
  }
  else
  {
    filestrm.open( file.c_str(), vcl_ofstream::out );
  }
}

bool
homography_writer_process
::step()
{
  if( this->disabled_ )
    return false;
  
  if (!this->filestrm.is_open())
  {
    return false;
  }

  this->filestrm << this->FrameNumber << vcl_endl;
  this->filestrm.precision(6);
  this->filestrm << this->TimeStamp << vcl_endl;
  this->filestrm.precision(20);
  this->filestrm << this->Homography << vcl_endl;
  
  return true;
}

void homography_writer_process::set_source_homography( vgl_h_matrix_2d<double> const& H )
{
  this->Homography = H.get_matrix();
}

void homography_writer_process::set_source_vidtk_homography( image_to_image_homography const& H )
{
  this->Homography = H.get_transform().get_matrix();
}

void homography_writer_process::set_source_timestamp( vidtk::timestamp const& ts )
{
  this->TimeStamp = ts.time_in_secs();
  this->FrameNumber = ts.frame_number();
}

} // end namespace vidtk

