/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_reader_process.h"

#include <vnl/vnl_double_3x3.h>

#include <fstream>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_metadata_reader_process_cxx__
VIDTK_LOGGER("metadata_reader_process_cxx");


namespace vidtk
{

metadata_reader_process
::metadata_reader_process( std::string const& _name )
  : process( _name, "metadata_reader_process" ),
    input_filename_(),
    input_file_(),
    ts_()
{
  config_.add_parameter( "filename", "",
    "The input CSV file contain metadata. Could be produced from MAASProxy2Tool" );
}

metadata_reader_process
::~metadata_reader_process()
{
}

config_block
metadata_reader_process
::params() const
{
  return config_;
}

bool
metadata_reader_process
::set_params( config_block const& blk )
{
  try
  {
    input_filename_ = blk.get<std::string>( "filename" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() <<": set_params failed:"
      << e.what() );
    return false;
  }
  return true;
}

bool
metadata_reader_process
::initialize()
{
  if( !input_filename_.empty() )
  {
    input_file_.open( input_filename_.c_str() );
    if( !input_file_ )
    {
      LOG_ERROR( "Couldn't open file: " << input_filename_ );
      return false;
    }

    char hash;
    input_file_.get( hash );
    if( hash == '#' )
    {
      char comment_line[10000]; // FIXME: Static buffer.
      input_file_.getline( comment_line, 10000 );

      // TODO: comments are currently being ignored. Use them for reading in the metdata. 
    }
    else
    {
      input_file_.unget();
    }
  }

  H_img_to_wld_.set_identity();

  return true;
}

bool
metadata_reader_process
::step()
{
  if( input_filename_.empty() )
  {
    return true;
  }

  char comma;

  unsigned frame_number;
  input_file_ >> frame_number >> comma;
  ts_.set_frame_number( frame_number );

  long long ts_usecs;
  input_file_ >> ts_usecs >> comma;
  ts_.set_time( static_cast<double>(ts_usecs) );

  double UL[2], UR[2], LL[2], LR[2]; // Currently not used
  input_file_ >> UL[0] >> comma;
  input_file_ >> UL[1] >> comma;
  input_file_ >> UR[0] >> comma;
  input_file_ >> UR[1] >> comma;
  input_file_ >> LL[0] >> comma;
  input_file_ >> LL[1] >> comma;
  input_file_ >> LR[0] >> comma;
  input_file_ >> LR[1] >> comma;

  unsigned ref_frame_number;  // Currently not used
  input_file_ >> ref_frame_number >> comma;

  vnl_double_3x3 M;
  for( unsigned i = 0; i < 3; i++ )
    for( unsigned j = 0; j < 3; j++ )
      input_file_ >> M(i,j) >> comma;
  vgl_h_matrix_2d<double> H_img_to_ref( M ); // Currently not used

  for( unsigned i = 0; i < 3; i++ ){
    for( unsigned j = 0; j < 3; j++ ){
      input_file_ >> M(i,j);
      // Handling one last missing comma at the end of line.
      if( !(i == 2 && j == 2) )
      {
        input_file_ >> comma;
      }
    }
  }
  H_img_to_wld_.set( M );
  H_wld_to_img_ = H_img_to_wld_.get_inverse();

  return true;
}


vgl_h_matrix_2d< double >
metadata_reader_process
::image_to_world_homography() const
{
  return H_img_to_wld_;
}

vgl_h_matrix_2d< double >
metadata_reader_process
::world_to_image_homography() const
{
  return H_wld_to_img_;
}

timestamp
metadata_reader_process
::timestamp() const
{
  return ts_;
}

}// end namespace vidtk
