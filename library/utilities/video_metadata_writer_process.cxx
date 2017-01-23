/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_metadata_writer_process.h"

#include <sstream>
#include <fstream>
#include <vnl/vnl_double_2.h>

#include <utilities/video_metadata_util.h>

#include <logger/logger.h>

VIDTK_LOGGER( "video_metadata_writer_process" );

/// \todo This process has two largely independent modes of operation
/// depending on the "type" selected. If "vidtk" output, then it
/// writes one file per frame using the pattern. If "kml" output, then
/// it uses a different parameter to specify the output file. This
/// should operate as a more harmonious unit, rather than behave like
/// two different writers glued together.
///
/// Look at using the track writer design pattern to implement various
/// output formatters.

namespace vidtk
{

video_metadata_writer_process
::video_metadata_writer_process( std::string const& _name )
  : process( _name, "video_metadata_writer_process" ), mode_(MODE_VIDTK)
{
  config_.add_parameter( "pattern", "out%2$04d.vidtk_meta", "The output file name pattern" );
  config_.add_parameter( "disabled", "false", "whether or not to run the writer");
  config_.add_parameter( "type", "vidtk",
                         "What type of metadata output.  "
                         "Options are \n\t(vidtk): writes the vidtk format.  "
                         "\n\t(kml): writes all the corner points a single metadata file");
  config_.add_parameter("kml_fname", "out.kml", "The file where to write the kml file.");
  config_.add_parameter("image_size", "720 680", "Size of the image in pixels, used in kml output");
  config_.add_parameter("write_given_corner_points", "false", "For KML, when true, we write the "
                        "corner points in the metadata, not the points calculated for img to utm");
}

video_metadata_writer_process
::~video_metadata_writer_process()
{
}

config_block
video_metadata_writer_process
::params() const
{
  return config_;
}

bool
video_metadata_writer_process
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>("disabled");
    std::string pat = blk.get<std::string>( "pattern" );
    using namespace boost::io;
    pattern_ = boost::format( pat );
    pattern_.exceptions( all_error_bits & ~too_many_args_bit );
    std::string mode = blk.get<std::string>( "type" );
    if(mode == "vidtk")
    {
      mode_ = MODE_VIDTK;
    }
    else if( mode == "kml")
    {
      mode_ = MODE_KML;
    }
    else
    {
      throw config_block_parse_error( "Invalid type: " + mode );
    }
    kml_fname_ = blk.get<std::string>( "kml_fname" );
    vnl_double_2 size = blk.get<vnl_double_2>( "image_size" );
    kml_ni_ = static_cast<unsigned int>(size[0]);
    kml_nj_ = static_cast<unsigned int>(size[1]);
    kml_write_given_corner_points_ = blk.get<bool>( "write_given_corner_points" );
  }
  catch( config_block_parse_error & e )
  {
    LOG_ERROR( this->name() <<": set_params failed:" << e.what() );
    return false;
  }
  return true;
}

bool
video_metadata_writer_process
::initialize()
{
  step_count_ = 0;
  if(!disabled_)
  {
    switch(mode_)
    {
      case MODE_VIDTK: return true;
      case MODE_KML:
        if(!kml_writer_.open(kml_fname_))
        {
          LOG_ERROR( this->name() << ": Could not open: " << kml_fname_);
          return false;
        }
        return true;
    }
  }
  return true;
}

bool
video_metadata_writer_process
::step()
{
  if(disabled_)
  {
    return true;
  }

  if(mode_ == MODE_KML)
  {
    if(meta_.is_valid())
    {
      // Test for file being opened.
      /// \note This test is somewhat redundant in that the
      /// write_comp_transform_points() call will return false if the
      /// write fails for any reason including file not open.
      if ( ! kml_writer_.is_open() )
      {
        return false; // cant write if file not open
      }

      if(kml_write_given_corner_points_ && !kml_writer_.write_corner_point(time_, meta_))
      {
        LOG_ERROR(this->name() << ": Failed to write a valid metadata packet");
        reset_variables();
        return false;
      }
      else if(!kml_write_given_corner_points_ && !kml_writer_.write_comp_transform_points(time_,kml_ni_, kml_nj_, meta_))
      {
        LOG_ERROR( this->name() << ": Failed to write a valid metadata packet");
        reset_variables();
        return false;
      }
    }
  }
  else
  {
    if( ! time_.has_frame_number() )
    {
      time_.set_frame_number( step_count_ );
    }
    if( ! time_.has_time() )
    {
      time_.set_time( time_.frame_number() );
    }

    std::ostringstream filename;
    filename << ( this->pattern_ % time_.time() % time_.frame_number() % step_count_ );

    std::string fname(filename.str());
    std::ofstream fout(fname.c_str());
    if(!fout)
    {
      LOG_ERROR(this->name() << ": Could not open: " << fname);
      reset_variables();
      return false;
    }
    fout << time_.frame_number() << " " << static_cast<vxl_uint_64>(time_.time()) << " ";
    ascii_serialize(fout, meta_);
    fout << std::endl;

    ++step_count_;
  }
  reset_variables();
  return true;
}

void video_metadata_writer_process
::reset_variables()
{
  meta_ = video_metadata().is_valid(false);
  time_ = timestamp();
}

void
video_metadata_writer_process
::set_meta(video_metadata vm)
{
  meta_ = vm;
}

void video_metadata_writer_process
::set_timestamp(timestamp ts)
{
  time_ = ts;
}

}// end namespace vidtk
