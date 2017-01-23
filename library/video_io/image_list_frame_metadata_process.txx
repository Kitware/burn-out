/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_io/image_list_frame_metadata_process.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vul/vul_file_iterator.h>
#include <vul/vul_file.h>
#include <vul/vul_reg_exp.h>
#include <vil/vil_load.h>
#include <vxl_config.h>

#include <iomanip>

#include <video_io/frame_process.txx>
#include <utilities/video_metadata_util.h>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_image_list_frame_metadata_process_txx__
VIDTK_LOGGER("image_list_frame_metadata_process_txx");


namespace
{

void replace_backslash_with_slash( std::string& str )
{
  for( std::string::iterator sit = str.begin();
       sit != str.end(); ++sit )
  {
    if( *sit == '\\' )
    {
        *sit = '/';
    }
  }
}

} // end anonymous namespace


namespace vidtk
{

template <class PixType>
image_list_frame_metadata_process<PixType>
::image_list_frame_metadata_process( std::string const& _name )
  : frame_process<PixType>( _name, "image_list_frame_process" ),
    cur_idx_( 0 ),
    ni_( 0 ),
    nj_( 0 ),
    loop_forever_( false ),
    loop_count_( 0 ),
    frame_size_accessed_( false ),
    roi_string_("")
{
}


template<class PixType>
config_block
image_list_frame_metadata_process<PixType>
::params() const
{
  config_block blk;

  // file containing a list of image
  blk.add_optional( "file",
                    "file containing a list of image files and metadata files."
                    "for each pair: image file first, then metadata file"
                    " on seperate lines.");

  // Should the image source loop continuously
  blk.add_optional( "loop_forever", "Should the image source loop continuously" );

  //ROI configuration
  blk.add_optional( "roi", "ROI (region of interest) of the form WxH+X+Y, e.g --aoi 240x191+100+100."
                           "  This will result in only this part of the image to be loaded.  Please"
                           " note, you might want to make sure you pixel to world is correct if "
                           "an ROI is provided" );

  // Metadata format
  blk.add_parameter( "metadata_format", "klv", "Which format the metadata file is in."
                     " Currently supported formats are 'klv' and 'red_river'" );
  // FOV for Red River (does not exist in the metadata files)
  blk.add_parameter( "red_river_hfov", "28.304", "Horizontal FOV (degrees) for Red River.");
  blk.add_parameter( "red_river_vfov", "18.484", "Vertical FOV (degrees) for Red River.");

  return blk;
}

template<class PixType>
bool
image_list_frame_metadata_process<PixType>
::set_params( config_block const& blk )
{
  image_filenames_.clear();
  meta_filenames_.clear();

  if( blk.has( "file" ) )
  {
    file_ = blk.get<std::string>( "file" );
    std::ifstream istr( file_.c_str() );
    if( ! istr )
    {
      LOG_ERROR( this->name() << ": couldn't open \"" << file_ << "\" for reading" );
      return false;
    }
    while( istr )
    {
      std::string fn;
      if ( std::getline(istr, fn) && !fn.empty() )
      {
        image_filenames_.push_back( fn );
      }
      if ( std::getline(istr, fn) && !fn.empty() )
      {
        meta_filenames_.push_back( fn );
      }
    }
  }

  loop_forever_ = false;
  if (blk.has( "loop_forever" ))
  {
    loop_forever_ = blk.get<bool>( "loop_forever" );
  }

  if(blk.has( "roi" ))
  {
    roi_string_ = blk.get<std::string>("roi");
  }

  std::string md_format = blk.get<std::string>( "metadata_format" );
  if( md_format == "klv" )
  {
    metadata_format_ = KLV;
  }
  else if( md_format == "red_river" )
  {
    metadata_format_ = RED_RIVER;

    // Validate red river FOV values
    red_river_hfov_ = blk.get<double>( "red_river_hfov" );
    red_river_vfov_ = blk.get<double>( "red_river_vfov" );
    if( red_river_hfov_ <= 0 || red_river_hfov_ >= 180 )
    {
      LOG_ERROR( this->name() << ": red_river_hfov parameter out of theoretical range: " << red_river_hfov_ );
      return false;
    }
    if( red_river_vfov_ <= 0 || red_river_vfov_ >= 180 )
    {
      LOG_ERROR( this->name() << ": red_river_vfov parameter out of theoretical range: " << red_river_vfov_ );
      return false;
    }
  }
  else
  {
    LOG_ERROR( this->name() << ": invalid metadata format specified: " << md_format );
    return false;
  }

  return true;
}


template<class PixType>
bool
image_list_frame_metadata_process<PixType>
::initialize()
{
  cur_idx_ = unsigned(-1);

  if( image_filenames_.empty() || meta_filenames_.empty() )
  {
    LOG_WARN( this->name() << ": no images in list" );
    return true;
  }

  if( image_filenames_.size() != meta_filenames_.size() )
  {
    LOG_ERROR( this->name() << ": mismatched image/metadata filenames");
    return false;
  }

  bool all_exists = true;

  typedef std::vector<std::string>::iterator iter_type;

  for( iter_type it = image_filenames_.begin();
       it != image_filenames_.end(); ++it )
  {
    // Convert all \ to / for consistency
    replace_backslash_with_slash( *it );
    boost::filesystem::path const pth( *it );
    if( !pth.is_absolute() )
    {
      *it = vul_file::dirname( file_ ) + "/" + *it;
    }
    if( ! vul_file_exists( *it ) )
    {
      LOG_ERROR( this->name() << ": file \"" << *it << "\" does not exist\n"; );
      all_exists = false;
    }
  }

  for( iter_type it = meta_filenames_.begin();
       it != meta_filenames_.end(); ++it )
  {
    // Convert all \ to / for consistency
    replace_backslash_with_slash( *it );
    boost::filesystem::path const pth( *it );
    if( !pth.is_absolute() )
    {
      *it = vul_file::dirname( file_ ) + "/" + *it;
    }
    if( ! vul_file_exists( *it ) )
    {
      LOG_ERROR( this->name() << ": file \"" << *it << "\" does not exist\n"; );
      all_exists = false;
    }
  }

  if( ! all_exists )
  {
    return false;
  }

  timestamps_.clear();

  metadata_.resize(meta_filenames_.size());
  // load all the metadata files for later use
  // also, fill in the timestamps with the frame number and time
  for (unsigned i=0; i<meta_filenames_.size(); ++i)
  {
    std::ifstream in;
    in.open(meta_filenames_[i].c_str());
    unsigned long frame_number;
    vxl_uint_64 timecode;
    // Expecting Red River metadata filenames to be of the format Meta_000000.txt
    vul_reg_exp re("Meta_([0-9]+)\\.txt");

    switch(metadata_format_)
    {
      case KLV:
        in >> frame_number;
        in >> timecode;
        ascii_deserialize(in, metadata_[i]);
        break;
      case RED_RIVER:
        if( !re.find(meta_filenames_[i]) )
        {
          LOG_ERROR( this->name() << ": couldn't parse frame number from filename: " << meta_filenames_[i] );
          in.close();
          return false;
        }
        frame_number = boost::lexical_cast<unsigned long>( re.match(1) );
        ascii_deserialize_red_river(in, metadata_[i]);
        timecode = metadata_[i].timeUTC();

        // Values not included in Red River metadata
        metadata_[i].sensor_horiz_fov(red_river_hfov_);
        metadata_[i].sensor_vert_fov(red_river_vfov_);
        metadata_[i].slant_range(-1.0);
        break;
      default:
        LOG_ERROR( this->name() << ": no implementation for metadata format specified: " << metadata_format_ );
        in.close();
        return false;
    }

    in.close();

    vidtk::timestamp ts;
    ts.set_frame_number(frame_number);
    double t = static_cast< double>( timecode );
    ts.set_time(t);

    timestamps_.push_back( ts );
  }

  assert( timestamps_.size() == meta_filenames_.size() );

  if(roi_string_ != "")
  {
    if(!this->set_roi(roi_string_))
    {
      LOG_ERROR( this->name() << ": couldn't parse ROI " << roi_string_ );
      return false;
    }
  }

  //only open the image if needed to get size
  if( this->has_roi_ )
  {
    ni_ = this->roi_width_;
    nj_ = this->roi_height_;
  }
  else
  {
    vil_image_resource_sptr img = vil_load_image_resource( image_filenames_[0].c_str() );
    if( ! img )
    {
      LOG_ERROR( this->name() << ": couldn't load \"" << image_filenames_[0] );
      return false;
    }
    ni_ = img->ni();
    nj_ = img->nj();
  }

  return true;
}


template<class PixType>
bool
image_list_frame_metadata_process<PixType>
::step()
{
  ++cur_idx_;

  if ( (cur_idx_ == image_filenames_.size()) && (loop_forever_) )
  {
    ++loop_count_;
    cur_idx_ = 0;
  }

  if( cur_idx_ < image_filenames_.size() )
  {
    if(this->has_roi_)
    {
      this->load_image_roi(image_filenames_[cur_idx_]);
    }
    else
    {
      this->load_image(image_filenames_[cur_idx_]);
    }
    if( ! img_ )
    {
      LOG_ERROR( this->name() << ": couldn't load \""
                 << image_filenames_[cur_idx_] );
    }
  }
  else
  {
    // invalidate the image
    img_ = vil_image_view<PixType>();
  }

  if( frame_size_accessed_ )
  {
    if( img_ &&
        ( ( ni_ != 0 && ni_ != img_.ni() ) ||
          ( nj_ != 0 && nj_ != img_.nj() ) ) )
    {
      LOG_WARN( this->name() << ": ni() or nj() was used, but the value changed at index"
                   << cur_idx_ << " (\"" << image_filenames_[cur_idx_] << "\")" );
    }
  }

  // The step worked if we have an image.
  return img_;
}


template<class PixType>
bool
image_list_frame_metadata_process<PixType>
::seek( unsigned frame_number )
{
  if( frame_number < image_filenames_.size() )
  {
    cur_idx_ = frame_number-1;
    loop_count_ = 0;
    return this->step();
  }
  else
  {
    return false;
  }
}


template<class PixType>
unsigned
image_list_frame_metadata_process<PixType>
::nframes() const
{
  return static_cast< unsigned >( image_filenames_.size() );
}


template<class PixType>
double
image_list_frame_metadata_process<PixType>
::frame_rate() const
{
  return 0.0; // FIXME not implemented yet
}


template<class PixType>
timestamp
image_list_frame_metadata_process<PixType>
::timestamp() const
{
  //TEMP Debug print
  LOG_DEBUG( this->name() << ": Sending frame " << timestamps_[cur_idx_] );
  //

  // simple if we're not looping
  if ( ! loop_forever_ ) return timestamps_[cur_idx_];

  // more complicated if we are ... only return frame numbers
  unsigned n = (timestamps_.size() * loop_count_ ) + cur_idx_;
  vidtk::timestamp looped_timestamp;
  looped_timestamp.set_frame_number( n );
  return looped_timestamp;
}


template<class PixType>
vil_image_view<PixType>
image_list_frame_metadata_process<PixType>::
image() const
{
  return img_;
}


template<class PixType>
vidtk::video_metadata
image_list_frame_metadata_process<PixType>::
metadata() const
{
  return metadata_[cur_idx_];
}


template <class PixType>
unsigned
image_list_frame_metadata_process<PixType>
::ni() const
{
  frame_size_accessed_ = true;
  return ni_;
}


template <class PixType>
unsigned
image_list_frame_metadata_process<PixType>
::nj() const
{
  frame_size_accessed_ = true;
  return nj_;
}


template <class PixType>
bool
image_list_frame_metadata_process<PixType>
::load_image_roi(std::string & fname)
{
  vil_image_resource_sptr img = vil_load_image_resource( fname.c_str() );
  if ( ! img )
  {
    LOG_ERROR( "image_list_frame_process<PixType>: couldn't load \""
               << fname );
    img_ = vil_image_view<PixType>();
    return false;
  }

  img_ = img->get_view( this->roi_x_, this->roi_width_,
                        this->roi_y_, this->roi_height_ );
  return img_;
}


template <class PixType>
bool
image_list_frame_metadata_process<PixType>
::load_image(std::string & fname)
{
  img_ = vil_load( fname.c_str() );
  return img_;
}


} // end namespace vidtk
