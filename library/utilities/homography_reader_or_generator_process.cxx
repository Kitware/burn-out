/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_reader_or_generator_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vcl_cassert.h>
#include <vcl_fstream.h>
#include <vnl/vnl_double_3x3.h>

#include <vcl_sstream.h>


namespace vidtk
{


homography_reader_or_generator_process
::homography_reader_or_generator_process( vcl_string const& name )
  : process( name, "homography_reader_or_generator_process" ),
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
    blk.get( "textfile", input_filename_ );
    blk.get( "generate", generate_ );
    blk.get( "number_of_homographys", number_to_generate_ );
    vcl_string homg;
    blk.get( "input_homography", homg );
    vcl_stringstream ss(homg);
    vnl_double_3x3 M;
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
        log_error( "Couldn't open \"" << input_filename_
                  << "\" for reading\n" );
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
          log_warning( "Failed to read homography\n" );
          return false;
        }
      }
    }

    img_to_world_H_.set( M );
    world_to_img_H_ = img_to_world_H_.get_inverse();
  }
  return true;
}


vgl_h_matrix_2d<double> const&
homography_reader_or_generator_process
::image_to_world_homography() const
{
    return img_to_world_H_;
}


vgl_h_matrix_2d<double> const&
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
