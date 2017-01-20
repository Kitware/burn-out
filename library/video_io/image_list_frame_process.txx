/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_list_frame_process.h"

#include <vul/vul_file_iterator.h>
#include <vul/vul_file.h>
#include <vul/vul_reg_exp.h>
#include <vil/vil_load.h>
#include <vil/vil_convert.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <video_io/frame_process.txx>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_image_list_frame_process_txx__
VIDTK_LOGGER( "image_list_frame_process_txx" );


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

/// @todo: This warning is being suppressed because, at the moment, there's nothing that can easily be done.
/// The sscanf code should ideally get replaced with regex to perform the same task of frame data extraction.
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

template <class PixType>
image_list_frame_process<PixType>
::image_list_frame_process( std::string const& _name )
  : frame_process<PixType>( _name, "image_list_frame_process" ),
    cur_idx_( 0 ),
    base_frame_number_(0),
    ni_( 0 ),
    nj_( 0 ),
    loop_forever_( false ),
    loop_count_( 0 ),
    frame_size_accessed_( false ),
    roi_string_( "" )
{
}


template<class PixType>
config_block
image_list_frame_process<PixType>
::params() const
{
  config_block blk;

  blk.add_optional( "glob", "The set of images provided in glob form.  i.e. images/image*.png" );

  // a list of image file names
  blk.add_optional( "list", "a list of image file names" );

  // file containing a list of image
  blk.add_optional( "file", "file containing a list of image" );

  // This is a scanf string that will be applied to the filename.  It
  // is expected to parse out one and only one %u.  It should not
  // parse out any other values.  Use %* to parse but not store
  // additional fields.
  blk.add_parameter( "parse_frame_number", "",
                     "This is a scanf string that will be applied to the filename.  It"
                     " is expected to parse out one and only one %u.  It should not"
                     " parse out any other values.  Use %* to parse but not store"
                     " additional fields." );

  // This is what frame number is for the first frame in the list.
  // Please note, if parse_frame_number is set, this parameter will
  // be ignored.
  blk.add_parameter( "base_frame_number", "0",
                     "This is what frame number is for the first frame in the list.  "
                     "Please note, if parse_frame_number is set, this parameter will "
                     "be ignored.");

  // Similar to above, but requires one and only one %lf.
  blk.add_parameter( "parse_timestamp",
                     "",
                     "Similar to parse_frame_number, but requires one and only one %lf." );

  // Should the image source loop continuously
  blk.add_optional( "loop_forever",
                    "Should the image source loop continuously" );

  // ROI configuration
  blk.add_optional( "roi",
                    "ROI (region of interest) of the form WxH+X+Y, e.g --aoi 240x191+100+100."
                    "  This will result in only this part of the image to be loaded.  Please"
                    " note, you might want to make sure you pixel to world is correct if "
                    "an ROI is provided" );

  // Optional exclusion string
  blk.add_optional( "ignore_substring",
                    "If non-empty, any files with this string in it will not be read. This "
                    "is useful if globbing a directory that has certain files that should "
                    "be ignored." );

  return blk;
}

template<class PixType>
bool
image_list_frame_process<PixType>
::set_params( config_block const& blk )
{
  filenames_.clear();

  try
  {

    if( blk.has( "glob" ) )
    {
      for( vul_file_iterator fn=blk.get< std::string >( "glob" ); fn; ++fn )
      {
        if( !boost::filesystem::is_directory( boost::filesystem::path( fn() ) ) )
        {
          filenames_.push_back( fn() );
        }
      }

      std::sort( filenames_.begin(), filenames_.end() );
    }

    if( blk.has( "list" ) )
    {
      std::istringstream istr( blk.get< std::string >( "list" ) );
      std::string fn;
      while( std::getline( istr, fn ) )
      {
        if(!fn.empty())
        {
          filenames_.push_back( fn );
        }
      }
    }

    if( blk.has( "file" ) )
    {
      file_ = blk.get< std::string >( "file" );
      std::ifstream istr( file_.c_str() );
      if( ! istr )
      {
        LOG_ERROR( this->name() << ": couldn't open \"" << file_ << "\" for reading." );
        return false;
      }
      std::string fn;
      while( std::getline( istr, fn ) )
      {
        if(!fn.empty())
        {
          filenames_.push_back( fn );
        }
      }
    }

    parse_frame_number_ = blk.get< std::string >( "parse_frame_number" );

    if( parse_frame_number_.empty() )
    {
      this->base_frame_number_ = blk.get< unsigned int >( "base_frame_number" );
    }
    else
    {
      LOG_WARN( this->name() << ": This functionality will be ported over to using regex rather than scanf eventually" );
    }

    parse_timestamp_ = blk.get< std::string >( "parse_timestamp" );

    loop_forever_ = false;

    if( blk.has( "loop_forever" ) )
    {
      loop_forever_ = blk.get< bool >( "loop_forever" );
    }

    if( blk.has( "roi" ) )
    {
      roi_string_ = blk.get< std::string >("roi");
    }

    if( blk.has( "ignore_substring" ) )
    {
      ignore_substring_ = blk.get< std::string >( "ignore_substring" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }


  return true;
}


template<class PixType>
bool
image_list_frame_process<PixType>
::initialize()
{
  cur_idx_ = unsigned(-1);

  if( filenames_.empty() )
  {
    LOG_WARN( this->name() << ": no images in list" );
    return true;
  }

  bool all_exists = true;

  typedef std::vector<std::string>::iterator iter_type;

  for( iter_type it = filenames_.begin();
       it != filenames_.end(); ++it )
  {
    // Convert all \ to / for consistency
    replace_backslash_with_slash( *it );
    boost::filesystem::path const pth( *it );
    if( ! pth.is_absolute() )
    {
      *it = vul_file::dirname( file_ ) + "/" + *it;
    }
    if( ! vul_file_exists( *it ) )
    {
      LOG_ERROR( this->name() << ": file \"" << *it << "\" does not exist." );
      all_exists = false;
    }
  }
  if( ! all_exists )
  {
    return false;
  }

  // Load the first image to get the frame size.
  if( roi_string_ != "" )
  {
    if( ! this->set_roi(roi_string_) )
    {
      LOG_ERROR( this->name() << ": couldn't parse ROI " << roi_string_ );
      return false;
    }
  }

  if( this->has_roi_ )
  {
    ni_ = this->roi_width_;
    nj_ = this->roi_height_;
  }
  else
  {
    vil_image_resource_sptr img = vil_load_image_resource( filenames_[0].c_str() );
    if( ! img )
    {
      LOG_ERROR( this->name() << ": couldn't load \"" << filenames_[0] );
      return false;
    }
    ni_ = img->ni();
    nj_ = img->nj();
  }


  // Figure out the timestamps for each of the files.  If we don't
  // have a frame number, set a monotonically increasing sequence.
  for( unsigned i = 0; i < filenames_.size(); ++i )
  {
    vidtk::timestamp ts;

    // default to just the image index
    ts.set_frame_number( i+base_frame_number_ );

    // if requested, parse the frame number out of the filename
    if( ! parse_frame_number_.empty() )
    {
      unsigned f;
      // TODO: Use something other than sscanf for safety.
      int ret = std::sscanf( filenames_[i].c_str(), parse_frame_number_.c_str(), &f );
      if( ret != 1 )
      {
        LOG_ERROR( this->name() << ": couldn't parse frame number from " << filenames_[i] );
        return false;
      }
      ts.set_frame_number( f );
    }

    if( ! parse_timestamp_.empty() )
    {
      double t;
      // TODO: Use something safer than scanf.
      int ret = std::sscanf( filenames_[i].c_str(), parse_timestamp_.c_str(), &t );
      if( ret != 1 )
      {
        LOG_ERROR( this->name() << ": couldn't parse timestamp from " << filenames_[i] );
        return false;
      }
      ts.set_time( t );
    }

    timestamps_.push_back( ts );
  }

  assert( timestamps_.size() == filenames_.size() );

  return true;
}


template<class PixType>
bool
image_list_frame_process<PixType>
::step()
{
  ++cur_idx_;

  if ( ( cur_idx_ == filenames_.size() ) && ( loop_forever_ ) )
  {
    ++loop_count_;
    cur_idx_ = 0;
  }

  if( cur_idx_ < filenames_.size() )
  {
    // Skip frame if it contains the ignore string
    if( !ignore_substring_.empty() )
    {
      boost::filesystem::path pth( filenames_[cur_idx_] );

      if( pth.filename().string().find( ignore_substring_ ) != std::string::npos )
      {
        return step();
      }
    }

    // Load frame using appropriate method
    if( this->has_roi_ )
    {
      this->load_image_roi( filenames_[cur_idx_] );
    }
    else
    {
      this->load_image( filenames_[cur_idx_] );
    }
    if( ! img_ )
    {
      LOG_ERROR( "image_list_frame_process<PixType>: couldn't load \""
                 << filenames_[cur_idx_] );
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
      LOG_WARN( "image_list_frame_process<PixType>: ni() or nj() was used, but the value changed at index"
                   << cur_idx_ << " (\"" << filenames_[cur_idx_] << "\")" );
    }
  }

  // The step worked if we have an image.
  return img_;
}


template<class PixType>
bool
image_list_frame_process<PixType>
::seek( unsigned frame_number )
{
  frame_number = frame_number-base_frame_number_;

  if( frame_number < filenames_.size() )
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
image_list_frame_process<PixType>
::nframes() const
{
  return static_cast< unsigned >( filenames_.size() );
}


template<class PixType>
double
image_list_frame_process<PixType>
::frame_rate() const
{
  return 0.0; // FIXME not implemented yet
}


template<class PixType>
timestamp
image_list_frame_process<PixType>
::timestamp() const
{
  // simple if we're not looping
  if ( ! loop_forever_ ) return timestamps_[cur_idx_];

  // more complicated if we are ... only return frame numbers
  unsigned n = (timestamps_.size() * loop_count_ ) + cur_idx_+base_frame_number_;
  vidtk::timestamp looped_timestamp;
  looped_timestamp.set_frame_number( n );
  return looped_timestamp;
}


template<class PixType>
vil_image_view<PixType>
image_list_frame_process<PixType>::
image() const
{
  return img_;
}


template <class PixType>
unsigned
image_list_frame_process<PixType>
::ni() const
{
  frame_size_accessed_ = true;
  return ni_;
}


template <class PixType>
unsigned
image_list_frame_process<PixType>
::nj() const
{
  frame_size_accessed_ = true;
  return nj_;
}

template <class PixType>
bool
image_list_frame_process<PixType>
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

  img_ = vil_convert_cast(PixType(),
                          img->get_view( this->roi_x_, this->roi_width_,
                                         this->roi_y_, this->roi_height_ ) );
  return img_;
}

template <class PixType>
bool
image_list_frame_process<PixType>
::load_image(std::string & fname)
{
  vil_image_resource_sptr img = vil_load_image_resource( fname.c_str() );
  if ( ! img )
  {
    LOG_ERROR( "image_list_frame_process<PixType>: couldn't load \""
               << fname );
    img_ = vil_image_view<PixType>();
    return false;
  }

  img_ = vil_convert_cast(PixType(), img->get_view());
  return img_;
}


} // end namespace vidtk
