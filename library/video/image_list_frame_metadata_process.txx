/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video/image_list_frame_metadata_process.h>

#include <vcl_algorithm.h>
#include <vcl_cstdio.h>
#include <vcl_fstream.h>
#include <vcl_sstream.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_file.h>
#include <vul/vul_reg_exp.h>
#include <vil/vil_load.h>
#include <vxl_config.h>

#include <iomanip>

#include <utilities/log.h>

#include <video/frame_process.txx>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem/path.hpp>

namespace
{

void replace_backslash_with_slash( vcl_string& str )
{
  for( vcl_string::iterator sit = str.begin();
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
::image_list_frame_metadata_process( vcl_string const& name )
  : frame_process<PixType>( name, "image_list_frame_process" ),
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
		    "for each pair: image file first, then metadata file");

  // Should the image source loop continuously
  blk.add_optional( "loop_forever", "Should the image source loop continuously" );

  //ROI configuration
  blk.add_optional( "roi", "ROI (region of interest) of the form WxH+X+Y, e.g --aoi 240x191+100+100."
                           "  This will result in only this part of the image to be loaded.  Please"
                           " note, you might want to make sure you pixel to world is correct if "
                           "an ROI is provided" );

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
    file_ = blk.get<vcl_string>( "file" );
    vcl_ifstream istr( file_.c_str() );
    if( ! istr )
    {
      log_error( this->name() << ": couldn't open \"" << blk.get<vcl_string>( "file" ) << "\" for reading\n" );
      return false;
    }
    vcl_string fn;
    while( istr >> fn )
    {
      image_filenames_.push_back( fn );
      if (! (istr >> fn) )
      {
	log_error( this->name() << ": mismatched image/metadata filenames in list");
      }
      meta_filenames_.push_back( fn );
    }
  }

  loop_forever_ = false;
  if (blk.has( "loop_forever" ))
  {
    blk.get( "loop_forever", loop_forever_ );
  }

  if(blk.has( "roi" ))
  {
    blk.get("roi", roi_string_);
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
    log_warning( this->name() << ": no images in list\n" );
    return true;
  }

  bool all_exists = true;

  typedef vcl_vector<vcl_string>::iterator iter_type;

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
      log_error( this->name() << ": file \"" << *it << "\" does not exist\n"; );
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
      log_error( this->name() << ": file \"" << *it << "\" does not exist\n"; );
      all_exists = false;
    }
  }

  if( ! all_exists )
  {
    return false;
  }

  // Load the first image to get the frame size.
  vil_image_view<PixType> img = vil_load( image_filenames_[0].c_str() );
  if( ! img )
  {
    log_error( this->name() << ": couldn't load \"" << image_filenames_[0] << "\n" );
    return false;
  }

  metadata_.resize(meta_filenames_.size());
  // load all the metadata files for later use
  // also, fill in the timestamps with the frame number and time
  for (unsigned i=0; i<meta_filenames_.size(); ++i)
  {
    vcl_ifstream in;
    in.open(meta_filenames_[i].c_str());
    unsigned long frame_number;
    in >> frame_number;
    vxl_uint_64 timecode;
    in >> timecode;
    metadata_[i].ascii_deserialize(in);
    in.close();

    vidtk::timestamp ts;
    ts.set_frame_number(frame_number);
    double t = static_cast< double>( timecode );
    ts.set_time(t);

    timestamps_.push_back( ts );
  }

  assert( timestamps_.size() == meta_filenames_.size() );

  ni_ = img.ni();
  nj_ = img.nj();

  if(roi_string_ != "")
  {
    if(!this->set_roi(roi_string_))
    {
      log_error( this->name() << ": couldn't parse ROI " << roi_string_ << "\n" );
      return false;
    }
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
      log_error( this->name() << ": couldn't load \""
                 << image_filenames_[cur_idx_] << "\n" );
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
      log_warning( this->name() << ": ni() or nj() was used, but the value changed at index"
                   << cur_idx_ << " (\"" << image_filenames_[cur_idx_] << "\")\n" );
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
timestamp
image_list_frame_metadata_process<PixType>
::timestamp() const
{
  //TEMP Debug print
  log_debug( this->name() << ": Sending frame " << timestamps_[cur_idx_] << "\n" );
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
vil_image_view<PixType> const&
image_list_frame_metadata_process<PixType>::
image() const
{
  return img_;
}

template<class PixType>
vidtk::video_metadata const&
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
::load_image_roi(vcl_string & fname)
{
  vil_image_resource_sptr img = vil_load_image_resource( fname.c_str() );
  if ( ! img )
  {
    log_error( "image_list_frame_process<PixType>: couldn't load \""
               << fname << "\n" );
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
::load_image(vcl_string & fname)
{
  img_ = vil_load( fname.c_str() );
  return img_;
}


} // end namespace vidtk
