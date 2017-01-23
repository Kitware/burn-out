/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_reader_or_generator_process.h"

#include <cassert>
#include <fstream>
#include <vnl/vnl_double_3x3.h>

#include <sstream>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_homography_reader_or_generator_process_cxx__
VIDTK_LOGGER("homography_reader_or_generator_process_cxx");



namespace vidtk
{


homography_reader_or_generator_process
::homography_reader_or_generator_process( std::string const& _name )
  : process( _name, "homography_reader_or_generator_process" ),
    generate_ ( false ),
    image_needs_homog_( false ),
    number_to_generate_( 0 ),
    number_generated_(0)
{
  config_.add_parameter( "textfile", "", "The file from which to read the homography." );
  config_.add_parameter( "generate", "true", "Generates the same homography over and over again." );
  config_.add_parameter( "number_of_homographys", "0", "Number of homographies to generate." );
  config_.add_parameter( "input_homography", "1 0 0 0 1 0 0 0 1", "Homography to generate." );
}


homography_reader_or_generator_process
::~homography_reader_or_generator_process()
{
}


config_block
homography_reader_or_generator_process
::params() const
{
  return config_;
}


bool
homography_reader_or_generator_process
::set_params( config_block const& blk )
{
  try
  {
    input_filename_ = blk.get<std::string>( "textfile" );
    generate_ = blk.get<bool>( "generate" );
    number_to_generate_ = blk.get<unsigned>( "number_of_homographys" );
    std::string homg = blk.get<std::string>( "input_homography" );
    std::stringstream ss(homg);
    vnl_double_3x3 M;
    // TODO: Isn't reading from the config file as vnl_double_3x3 the same
    // thing? Or is it transposed?
    for( unsigned i = 0; i < 3; ++i )
    {
      for( unsigned j = 0; j < 3; ++j )
      {
        ss >> M(i,j);
      }
    }
    img_to_world_H_.set(M);
    world_to_img_H_ = img_to_world_H_.get_inverse();
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
homography_reader_or_generator_process
::initialize()
{
  if(!generate_)
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
  }
  return true;
}


bool
homography_reader_or_generator_process
::step()
{
  if(generate_)
  {
    number_generated_++;
    if(image_needs_homog_)
    {
      image_needs_homog_ = false;
      return true;
    }
    return number_generated_ < number_to_generate_;
  }
  else
  {
    if( input_filename_.empty() )
    {
      return true;
    }

    // Read in the next homography

    vnl_double_3x3 M;
    for( unsigned i = 0; i < 3; ++i )
    {
      for( unsigned j = 0; j < 3; ++j )
      {
        if( ! (homog_str_ >> M(i,j)) )
        {
          LOG_WARN( "Failed to read homography" );
          return false;
        }
      }
    }

    img_to_world_H_.set( M );
    world_to_img_H_ = img_to_world_H_.get_inverse();
  }
  return true;
}


vgl_h_matrix_2d<double>
homography_reader_or_generator_process
::image_to_world_homography() const
{
    return img_to_world_H_;
}


vgl_h_matrix_2d<double>
homography_reader_or_generator_process
::world_to_image_homography() const
{

    return world_to_img_H_;
}

void
homography_reader_or_generator_process
::signal_new_image(vidtk::timestamp)
{
  image_needs_homog_ = true;
}


} // end namespace vidtk
