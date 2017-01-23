/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_writer_process.h"

#include <algorithm>

#include <vnl/vnl_double_2.h>
#include <vgl/vgl_area.h>

#include <logger/logger.h>
VIDTK_LOGGER("homography_writer_process_cxx");


namespace vidtk
{

homography_writer_process
  ::homography_writer_process( std::string const& _name )
  : process( _name, "homography_writer_process" ),
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
    this->disabled_ = blk.get<bool>("disabled");
    if( !this->disabled_ )
    {
      this->filename = blk.get<std::string>("output_filename");
      this->append_file_ = blk.get<bool>("append");
    }
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
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
      LOG_ERROR( "Could not open file: " + this->filename + "" );
      return false;
    }
  }

  this->filestrm << std::fixed;

  return true;
}

void
homography_writer_process
::set_output_file(std::string file)
{
  if (filestrm.is_open())
  {
    filestrm.close();
  }

  if( this->append_file_ )
  {
    filestrm.open( file.c_str(), std::ofstream::app );
  }
  else
  {
    filestrm.open( file.c_str(), std::ofstream::out );
  }
}

bool
homography_writer_process
::step()
{
  if( this->disabled_ )
  {
    return false;
  }

  if( !this->filestrm.is_open() )
  {
    return false;
  }

  this->filestrm << this->FrameNumber << std::endl;
  this->filestrm.precision(6);
  this->filestrm << this->TimeStamp << std::endl;
  this->filestrm.precision(20);
  this->filestrm << this->Homography << std::endl;

  return true;
}

void homography_writer_process::set_source_homography( vgl_h_matrix_2d<double> const& H )
{
  this->Homography = H.get_matrix();
}

void homography_writer_process::set_source_vidtk_homography( vidtk::homography const& H )
{
  this->Homography = H.get_transform().get_matrix();
}

void homography_writer_process::set_source_timestamp( vidtk::timestamp const& ts )
{
  this->TimeStamp = ts.time_in_secs();
  this->FrameNumber = ts.frame_number();
}

} // end namespace vidtk
