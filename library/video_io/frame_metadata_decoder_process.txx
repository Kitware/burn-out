/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_io/frame_metadata_decoder_process.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vul/vul_file_iterator.h>
#include <vul/vul_file.h>
#include <vul/vul_sprintf.h>
#include <vul/vul_reg_exp.h>
#include <vil/vil_load.h>
#include <vxl_config.h>

#include <iomanip>

#include <video_io/frame_process.txx>
#include <utilities/video_metadata_util.h>

#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
#endif

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/concept_check.hpp>

// Set-up the logger for this file
#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_frame_metadata_decoder_process_txx__
VIDTK_LOGGER("frame_metadata_decoder_process_txx");


namespace vidtk
{

template <class PixType>
frame_metadata_decoder_process<PixType>
::frame_metadata_decoder_process( std::string const& _name )
  : frame_process<PixType>( _name, "frame_metadata_decoder_process" ),
    config_(),
    img_fn_re_string_(),
    meta_file_format_string_(),
    red_river_hfov_( 0 ),
    red_river_vfov_( 0 ),
    image_filename_(),
    meta_filename_(),
    timestamp_(),
    metadata_(),
    first_frame_(true),
    ni_( 0 ),
    nj_( 0 ),
    frame_size_accessed_( false ),
    roi_string_()
{
  // initialize parameter definitions:

  //ROI configuration
  config_.add_optional( "roi", "ROI (region of interest) of the form WxH+X+Y, e.g "
                        "240x191+100+100. This will result in only "
                        "this part of the image to be loaded.  Please note, "
                        "you might want to make sure your pixel to world "
                        "transformation is correct if an ROI is provided" );
  config_.add_parameter( "metadata_format", "klv", "Which format the metadata file "
                         "is in. Currently supported formats are 'klv' and "
                         "'red_river'." );
  config_.add_parameter( "img_frame_regex", "Data0_([0-9]+)\\.txt",
                         "Regular expression defining where to find the frame "
                         "number in an input image file. This should have exactly "
                         "one grouping where the frame number should be." );
  config_.add_parameter( "metadata_location", "./",
                         "Relative or absolute path to where the metadata "
                         "files are located. If this is a relative path, it "
                         "must be relative to the directory the image data is "
                         "located in." );
  config_.add_parameter( "metadata_filename_format", "Meta_%06d.txt",
                         "The format of the metadata file that is associated with "
                         "a given image file. This format *must* have one "
                         "format replacement of the form \"%d\" for the image "
                         "frame number it is matched to. Regular expression "
                         "syntax is allowed (the containing directory will be "
                         "iterated for matches; if none or multile matches "
                         "are found, step will fail)." );

  // FOV for Red River (does not exist in the metadata files)
  config_.add_parameter( "red_river_aspect_box", "6600.0 4400.0 17500.0",
                         "width height depth of aspect box for red river." );
}


template<class PixType>
config_block
frame_metadata_decoder_process<PixType>
::params() const
{
  return this->config_;
}

template<class PixType>
bool
frame_metadata_decoder_process<PixType>
::set_params( config_block const& blk )
{
  // General Metadata stuff
  vidtk::config_enum_option metadata_format_param;
  metadata_format_param
    .add( "klv", KLV )
    .add( "red_river", RED_RIVER );

  try
  {
    if(blk.has( "roi" ))
    {
      roi_string_ = blk.get< std::string >( "roi" );
    }

    metadata_format_ = blk.get< metadata_format >
      ( metadata_format_param, "metadata_format" );

    img_fn_re_string_ = blk.get< std::string >( "img_frame_regex" );
    meta_location_ = blk.get< std::string >( "metadata_location" );
    meta_file_format_string_ = blk.get< std::string >( "metadata_filename_format" );

    if( metadata_format_ == RED_RIVER )
    {
      // Validate red river FOV values
      std::string fov_angles = blk.get<std::string>( "red_river_aspect_box" );
      std::istringstream str(fov_angles);
      double width, height, depth;
      str >> width >> height >> depth;
      red_river_hfov_ = (2.0 * atan( width / (2.0 * depth))) * 57.2957795;
      red_river_vfov_ = (2.0 * atan( height / (2.0 * depth))) * 57.2957795;

      if( red_river_hfov_ <= 0 || red_river_hfov_ >= 180 )
      {
        throw config_block_parse_error(vul_sprintf("red_river_hfov parameter out of theoretical range: %f", red_river_hfov_));
      }
      if( red_river_vfov_ <= 0 || red_river_vfov_ >= 180 )
      {
        throw config_block_parse_error(vul_sprintf("red_river_vfov parameter out of theoretical range: %f", red_river_vfov_));
      }


    }
  }
  catch ( const config_block_parse_error &e )
  {
    LOG_ERROR( this->name() + ": Unable to set parameters: " << e.what() );
    return false;
  }

  // update internal config_block object with the new parameters
  config_.update( blk );

  return true;
}


template<class PixType>
bool
frame_metadata_decoder_process<PixType>
::initialize()
{
  // Initialize ROI settings if we have been manually provided one
  if(roi_string_ != "")
  {
    if(!this->set_roi(roi_string_))
    {
      LOG_ERROR( this->name() << ": couldn't parse ROI " << roi_string_ );
      return false;
    }

    // set the ni_/nj_ dimensions
    ni_ = this->roi_width_;
    nj_ = this->roi_height_;
  }

  return true;
}


template<class PixType>
bool
frame_metadata_decoder_process<PixType>
::step()
{
  // for various bfs::path -> string conversions for printing log messages
  std::string _tmp;

  // check that current image_filename_ is absolute and actually exists,
  // else return failure.
  boost::filesystem::path const img_pth( image_filename_ );
  image_filename_= boost::filesystem::absolute( img_pth ).string();
  if( !vul_file_exists( image_filename_ ) )
  {
    LOG_ERROR( this->name() << ": input image file \"" << image_filename_
               << "\" does not exist!" );
    return false;
  }

  LOG_INFO( this->name() << ": Decoding image file -> " << image_filename_ );

  // Get frame number from image file name
  unsigned long frame_number;
  vul_reg_exp img_frame_number_re( img_fn_re_string_.c_str() );
  if( ! img_frame_number_re.find( image_filename_ ) )
  {
    LOG_ERROR( this->name() << ": Couldn't parse the frame number from the "
               "image file name: \"" << image_filename_ << "\". Check your "
               "configuration file?" );
    return false;
  }

  frame_number = boost::lexical_cast<unsigned long>( img_frame_number_re.match(1) );
  LOG_DEBUG( this->name() << ": Image frame # -> " << frame_number );


  // Figure out the directory path of the paired metadata files
  boost::filesystem::path meta_location_pth( meta_location_ );
  if( ! meta_location_pth.is_absolute() )
  {
    LOG_DEBUG( this->name() << ": creating absolute path from configured relative "
               << "metadata location path" );
    meta_location_pth = ( img_pth.parent_path() / meta_location_pth );
  }

  _tmp = std::string( meta_location_pth.native().begin(), meta_location_pth.native().end() );
  LOG_DEBUG( this->name() << ": Looking for metadata files in -> " << _tmp );

  if( ! boost::filesystem::is_directory( boost::filesystem::status(meta_location_pth) ) )
  {
    LOG_ERROR( this->name() + ": The generated metadata directory is not a "
               "directory on the system." );
    return false;
  }


  // Figure out the specifically paired metadata file based on the image
  // file's frame number and the configured metadata file format.
  std::string _metadata_file_re_str = vul_sprintf( meta_file_format_string_.c_str(), frame_number );
  LOG_DEBUG( this->name() << ": Generated meta filename re -> " << _metadata_file_re_str );
  vul_reg_exp meta_file_re( _metadata_file_re_str.c_str() );

  std::vector<boost::filesystem::path> meta_pth_matches;
  boost::filesystem::directory_iterator dit( meta_location_pth );

  for( ; dit != boost::filesystem::directory_iterator(); ++dit )
  {
    // String manipulation to make windows happy
    boost::filesystem::path::string_type fname_native = (*dit).path().filename().native();
    std::string _filename = std::string(fname_native.begin(), fname_native.end());

    if( meta_file_re.find( _filename ) )
    {
      meta_pth_matches.push_back( (*dit).path() );
      LOG_DEBUG( this->name() << ": -- found \"" << (*dit).path().string()
                 << "\"" );
    }
  }
  if( meta_pth_matches.size() == 0 || meta_pth_matches.size() > 1 )
  {
    LOG_ERROR( this->name() << ": Found none or more than one metadata file "
               "based on the configured format for the current image file. "
               "Found " << meta_pth_matches.size() << ". Meta file pattern "
               "looked for: \"" << _metadata_file_re_str << "\"" );
    return false;
  }
  meta_filename_ = meta_pth_matches[0].string();
  LOG_INFO( this->name() << ": Associated metadata file -> " <<
            meta_filename_ );

  // Load the metadata from the current meta_filename_ file
  metadata_ = vidtk::video_metadata(); // reset metadata object to clear slate
  std::ifstream in;
  in.open( meta_filename_.c_str() );
  vxl_uint_64 timecode;

  switch( metadata_format_ )
  {
    case KLV:
      in >> frame_number;
      in >> timecode;
      ascii_deserialize( in, metadata_ );
      break;

    case RED_RIVER:
      ascii_deserialize_red_river( in, metadata_ );
      timecode = metadata_.timeUTC();

      // Values not included in Red River metadata
      metadata_.sensor_horiz_fov( red_river_hfov_ );
      metadata_.sensor_vert_fov( red_river_vfov_ );
      metadata_.slant_range( -1.0 );
      break;

    default:
      LOG_ERROR( this->name() << ": no implementation for metadata format "
                 "specified: " << metadata_format_ );
      in.close();
      return false;
  }
  in.close();


  // Set the timestamp for this step
  LOG_DEBUG( this->name() << ": FN->" << frame_number );
  LOG_DEBUG( this->name() << ": TS->" << timecode );
  timestamp_ = vidtk::timestamp( static_cast<double>(timecode), frame_number );


  // Load the image, or part of the image if an ROI is specified.
  LOG_DEBUG( this->name() << ": loading image -> '" << image_filename_ << "'" );
  bool load_success = this->generic_load_image( image_filename_ );
  LOG_DEBUG( this->name() << ": -- Done" );

  // warn if something asked about ni_/nj_ and the image size is changing
  if( frame_size_accessed_ )
  {
    if( img_ &&
        ( ( ni_ != 0 && ni_ != img_.ni() ) ||
          ( nj_ != 0 && nj_ != img_.nj() ) ) )
    {
      LOG_WARN( this->name() << ": ni() or nj() was used, but the width/height "
                "changed in \"" << image_filename_ << "\")" );
    }
  }

  // The step worked if we correctly loaded an image.
  return load_success;
}


template<class PixType>
void
frame_metadata_decoder_process<PixType>
::set_input_image_file( std::string const& image_file_path)
{
  this->image_filename_ = image_file_path;
  std::replace( this->image_filename_.begin(), this->image_filename_.end(),
               '\\', '/' );
  LOG_DEBUG( this->name() << "::[set_input_image_file] : storing "
             "image_filename_ -> " << image_filename_ );
}


template<class PixType>
timestamp
frame_metadata_decoder_process<PixType>
::timestamp() const
{
  return timestamp_;
}

template<class PixType>
vil_image_view<PixType>
frame_metadata_decoder_process<PixType>
::image() const
{
  return img_;
}

template<class PixType>
vidtk::video_metadata
frame_metadata_decoder_process<PixType>
::metadata() const
{
  return metadata_;
}

template <class PixType>
unsigned
frame_metadata_decoder_process<PixType>
::ni() const
{
  frame_size_accessed_ = true;
  return ni_;
}


template <class PixType>
unsigned
frame_metadata_decoder_process<PixType>
::nj() const
{
  frame_size_accessed_ = true;
  return nj_;
}

template <class PixType>
bool
frame_metadata_decoder_process< PixType >
::generic_load_image( std::string & fname )
{
  vil_image_resource_sptr img = vil_load_image_resource( fname.c_str() );
  if( img )
  {
    if( this->has_roi_ )
    {
      img_ = img->get_view( this->roi_x_, this->roi_width_,
                            this->roi_y_, this->roi_height_ );
    }
    else // no ROI -> full view
    {
      img_ = img->get_view();

      if( first_frame_ )
      {
        ni_ = img->ni();
        nj_ = img->nj();
        LOG_INFO( this->name() << ": Recording first frame image W/H "
                  "( " << ni_ << " x " << nj_ << " )" );
      }
    }
    first_frame_ = false;
  }
  else
  {
    LOG_ERROR( this->name() << ": couldn't load image \"" << fname << "\"" );
    img_ = vil_image_view<PixType>();
  }
  return img;
}

} // end namespace vidtk
