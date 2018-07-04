/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filename_frame_metadata_process.h"

#include <vil_plugins/vil_plugins_config.h>
#include <vil_plugins/vil_plugin_loader.h>
#ifdef HAS_LTIDSDK
#include <vil_plugins/file_formats/vil_lti_base_image.h>
#endif
#if defined(HAS_LTI_NITF2) || defined(HAS_VIL_GDAL)
#include <vil_plugins/file_formats/vil_nitf_engrda.h>
#endif
#include <geographic/geo_coords.h>
#include <utilities/geo_lat_lon.h>

#include <ctime>
#include <fstream>
#include <iomanip>

#include <vpl/vpl.h>
#include <vul/vul_reg_exp.h>
#include <vul/vul_timer.h>
#include <vil/vil_smart_ptr.h>
#include <vil/vil_stream_core.h>
#include <vil/vil_stream_fstream.h>
#include <vil/vil_load.h>
#include <vil/vil_crop.h>
#include <vil/vil_save.h>
#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_polygon.h>
#include <vgl/vgl_intersection.h>
#include <vgl/vgl_homg_point_2d.h>
#include <vgl/vgl_area.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/algo/vgl_h_matrix_2d_compute_4point.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_filename_frame_metadata_process_txx__
VIDTK_LOGGER("filename_frame_metadata_process_txx");


#define UINT_INVALID static_cast<unsigned int>(-1)

namespace vidtk
{

using namespace geographic;

struct filename_frame_metadata_process_impl
{
  unsigned int metadata_mode;
  std::string framenum_regex_str;
  unsigned int cache_mode;
  boost::filesystem::path cache_mode_2_path;
  bool cache_mode_2_delete;
  bool use_latlon_for_xfm;
  unsigned int max_skip_count;
  unsigned int retry_count;
  unsigned int retry_sleep;
  unsigned int num_decode_threads;
  bool oob_skip;

  boost::regex framenum_regex;
  boost::regex roi_regex;

  enum {GEOGRAPHIC_CONFIG_MODE, PIXEL_INPUT_STRING_MODE} crop_mode;

  //GEO MODE
  /// \todo Convert to using internal geo coordinates
  std::vector<geo_coords> roi_corners_geo;
  unsigned int roi_w;
  unsigned int roi_h;

  vgl_box_2d<int> roi_pixel;
  geo_coords roi_geo[4];

  unsigned int current_skip_count;
  vgl_point_2d<unsigned int> full_size;
  geo_coords full_geo[4];
  vgl_h_matrix_2d<double> xfm_f2w;
  vgl_h_matrix_2d<double> xfm_w2f;

  timestamp prev_timestamp;
};


enum metadata_mode
{
  metadata_mode_0_NONE = 0,
  metadata_mode_1_NITF = 1
};

enum cache_mode
{
  cache_mode_0_NONE = 0,
  cache_mode_1_RAM  = 1,
  cache_mode_2_DISK = 2
};

template<class PixType>
filename_frame_metadata_process<PixType>
::filename_frame_metadata_process(std::string const& _name)
: frame_process<PixType>(_name, "filename_frame_metadata_process"),
  impl_(new filename_frame_metadata_process_impl)
{
  this->config_.add_parameter("metadata_mode", "NONE",
    "Format of the image metadata; NONE: No metadata; NITF: NITF embedded metadata");
  this->config_.add_parameter("framenum_regex", "",
    "Regular expression that when applied to the filename, the first match group is the the frame number.  "
    "Ex: for a filename like 20120213161322-01000312-VIS.ntf where the frame number is 000312, you would use \'[0-9]+-[0-9][0-9]([0-9]+)-VIS\'");
  this->config_.add_parameter("crop_mode", "pixel_input",
                              "Controls how to get the crop string.  "
                              "(pixel_input) looks for the pixel crop string on the input string.  "
                              "(geographic_config) creates the crop string from a geo_roi");
  this->config_.add_parameter("geo_roi", "",
                              "Only used when crop_mode is set to geo.  "
                              "The roi in geo coordinates in the format \"UL UR LR LL\", \"UL LR\", or \"CP +radius\".  "
                              "The coordinates can be expressed as 2 part lat/long or 3 part UTM coordinates");
  this->config_.add_parameter("cache_mode", "NONE",
    "NONE: No caching, directly read the image file; RAM: Cache file to memory before decoding; DISK: Cache to disk");
  this->config_.add_parameter("cache_mode_2_path", "./",
    "Directory output path for file cache");
  this->config_.add_parameter("cache_mode_2_delete", "true",
    "Delete the cached file after decoding");
  this->config_.add_parameter("use_latlon_for_xfm", "false",
    "Use lat/lon coordinates for linear transformations.  Default is to use UTM");

  this->config_.add_parameter("max_skip_count", "0",
    "Max number of bad frames to skip before failing");
  this->config_.add_parameter("retry_count", "10",
    "Max number of attempts to retry opening the image file");
  this->config_.add_parameter("retry_sleep", "100",
    "Time in ms in between retries");

  this->config_.add_parameter("num_decode_threads", "1",
    "Number of threads to use for image decoding; Only valid for NITF and J2K images.");
  this->config_.add_parameter("oob_skip", "false", "true: Skip when the requested roi is entirely out of bounds, false: Do not skip, just process a black image");
}


template<class PixType>
filename_frame_metadata_process<PixType>
::~filename_frame_metadata_process(void)
{
  delete this->impl_;
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::set_params(config_block const& blk)
{
  try
  {
    std::string metadata_mode_str = blk.get<std::string>("metadata_mode");
    if(metadata_mode_str == "NONE")
    {
      this->impl_->metadata_mode = metadata_mode_0_NONE;
    }
    else if(metadata_mode_str == "NITF")
    {
      #if ! defined (HAS_LTI_NITF2) && ! defined(HAS_VIL_GDAL)
      throw config_block_parse_error("NITF metadata mode requires LizardTech or GDAL support");
      #endif
      this->impl_->metadata_mode = metadata_mode_1_NITF;
    }
    else
    {
      throw config_block_parse_error("Unknown metadata mode");
    }

    this->impl_->framenum_regex_str = blk.get<std::string>("framenum_regex");
    if(!this->impl_->framenum_regex_str.empty())
    {
      LOG_DEBUG( this->name() << ": Assigning framenum regex: \""
                 << this->impl_->framenum_regex_str << "\"");
      this->impl_->framenum_regex.assign(this->impl_->framenum_regex_str);
    }

    // the roi regex is pretty standard and static (used in crop_mode in_str)
    this->impl_->roi_regex.assign("^([0-9]+)x([0-9]+)([+-][0-9]+)([+-][0-9]+)$");

    std::string crop_mode_str = blk.get<std::string>("crop_mode");
    if(crop_mode_str == "pixel_input")
    {
      this->impl_->crop_mode = filename_frame_metadata_process_impl::PIXEL_INPUT_STRING_MODE;
    }
    else if(crop_mode_str == "geographic_config")
    {
      if(this->impl_->metadata_mode == metadata_mode_0_NONE)
      {
        throw config_block_parse_error("Need to have metadata for this geo crop mode");
      }
      this->impl_->crop_mode = filename_frame_metadata_process_impl::GEOGRAPHIC_CONFIG_MODE;
      std::string geo_roi = blk.get<std::string>("geo_roi");
      if(!this->parse_roi_geo(geo_roi))
      {
        throw config_block_parse_error("Unable to parse roi coordinates");
      }
    }
    else
    {
      throw config_block_parse_error(std::string("Unknown crop mode: ") + crop_mode_str + " options are pixel_input and geographic_config");
    }

    std::string cache_mode_str = blk.get<std::string>("cache_mode");
    if(cache_mode_str == "NONE")
    {
      this->impl_->cache_mode = cache_mode_0_NONE;
    }
    else if(cache_mode_str == "RAM")
    {
      this->impl_->cache_mode = cache_mode_1_RAM;
    }
    else if(cache_mode_str == "DISK")
    {
      this->impl_->cache_mode = cache_mode_2_DISK;
      this->impl_->cache_mode_2_path = blk.get<std::string>("cache_mode_2_path");
      this->impl_->cache_mode_2_delete = blk.get<bool>("cache_mode_2_delete");
    }
    else
    {
      throw config_block_parse_error("Unknown cache mode");
    }

    this->impl_->use_latlon_for_xfm = blk.get<bool>("use_latlon_for_xfm");

    this->impl_->max_skip_count = blk.get<unsigned int>("max_skip_count");
    this->impl_->retry_count = blk.get<unsigned int>("retry_count");
    this->impl_->retry_sleep = blk.get<unsigned int>("retry_sleep");
    this->impl_->num_decode_threads = blk.get<unsigned int>("num_decode_threads");
    this->impl_->oob_skip = blk.get<bool>("oob_skip");
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }
  this->config_.update( blk );
  return true;
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::initialize()
{
  load_vil_plugins();
#ifdef HAS_LTIDSDK
  vil_lti_base_image::set_num_threads(this->impl_->num_decode_threads);
#endif
  this->impl_->current_skip_count = 0;
  this->timestamp_.set_frame_number(0);
  this->impl_->roi_w = UINT_INVALID;
  this->impl_->roi_h = UINT_INVALID;
  this->impl_->prev_timestamp.set_frame_number(0);
  this->impl_->prev_timestamp.set_time(0.0);

  // Ensure the cache_mode_2_path is a valid writeable directory
  if(this->impl_->cache_mode == cache_mode_2_DISK)
  {
    if(boost::filesystem::exists(this->impl_->cache_mode_2_path))
    {
      if(!boost::filesystem::is_directory(this->impl_->cache_mode_2_path))
      {
        LOG_ERROR(this->name() << ": cache_mode_2_path must be a directory" );
        return false;
      }
    }
    else
    {
      if(!boost::filesystem::create_directory(this->impl_->cache_mode_2_path))
      {
        LOG_ERROR(this->name() << ": Unable to create cache directory" );
        return false;
      }
    }
  }

  LOG_INFO(this->name() << ": Ready" );
  return true;
}


template<class PixType>
process::step_status
filename_frame_metadata_process<PixType>
::step2()
{
  if(this->filename_.empty())
  {
    LOG_ERROR(this->name() << ": Empty filename input");
    return process::FAILURE;
  }

  if(this->impl_->crop_mode == filename_frame_metadata_process_impl::PIXEL_INPUT_STRING_MODE &&
    this->pixel_roi_.empty())
  {
    LOG_ERROR(this->name() << ": Empty pixel_roi input");
    return process::FAILURE;
  }

  if(!this->parse_framenum())
  {
    LOG_ERROR(this->name() << ": Unable to parse frame number from filename '" << this->filename_ << "'" );
    return process::FAILURE;
  }

  process::step_status skip_status;

  // Open and cache the image file, give appropriate retry and skip parameters
  vil_stream_sptr stream = this->cache_file();
  skip_status = this->check_skip(stream && stream->ok(), "Failed to open file: " + this->filename_);
  if(skip_status != process::SUCCESS)
  {
    return skip_status;
  }

  // Initialize the decoding stream
  vil_image_resource_sptr img = vil_load_image_resource_raw(stream.as_pointer());
  skip_status = this->check_skip(img, "Failed to determine file type.");
  if(skip_status != process::SUCCESS)
  {
    return skip_status;
  }

  // Decode metadata
  skip_status = this->check_skip(this->parse_metadata(img), "Failed to decode metadata.");
  if(skip_status != process::SUCCESS)
  {
    return skip_status;
  }

  // Ensure increasing timestamps and frame numbers
  assert(this->impl_->prev_timestamp.has_time());
  assert(this->timestamp_.has_time());
  skip_status = this->check_skip(this->impl_->prev_timestamp.time() < this->timestamp_.time(),
    "Timestamp is not increasing.");
  if(skip_status != process::SUCCESS)
  {
    return skip_status;
  }
  assert(this->impl_->prev_timestamp.has_frame_number());
  assert(this->timestamp_.has_frame_number());
  skip_status = this->check_skip(this->impl_->prev_timestamp.frame_number() < this->timestamp_.frame_number(),
    "Frame number is not increasing.");
  if(skip_status != process::SUCCESS)
  {
    return skip_status;
  }
  this->impl_->prev_timestamp = this->timestamp_;

  LOG_INFO(this->name() << ": Current frame: " << this->timestamp_ );
  this->impl_->current_skip_count = 0;

  this->build_transforms();
  //Setup the crop string
  switch(this->impl_->crop_mode)
  {
    case filename_frame_metadata_process_impl::PIXEL_INPUT_STRING_MODE:
      if( this->pixel_roi_.empty() )
      {
        LOG_ERROR(this->name() << ": Empty pixel_roi input");
        return process::FAILURE;
      }
      if(!this->parse_pixel_roi())
      {
        LOG_ERROR(this->name() << ": Unable to parse pixel roi: '" << this->pixel_roi_ << "'" );
        return process::FAILURE;
      }
      this->build_geo_roi();
      break;
    case filename_frame_metadata_process_impl::GEOGRAPHIC_CONFIG_MODE:
      this->compute_pixel_roi();
      break;
    default:
      LOG_ERROR(this->name() << ": Invalid crop mode");
      return process::FAILURE;
  };

  LOG_INFO(this->name() << ": Pixel ROI: " << this->impl_->roi_pixel);

  LOG_INFO(this->name() << ": Geo ROI: \n"
    << this->impl_->roi_geo[0].geo_representation(2) << ", "
    << this->impl_->roi_geo[0].utm_ups_representation(3) << "\n"
    << this->impl_->roi_geo[1].geo_representation(2) << ", "
    << this->impl_->roi_geo[1].utm_ups_representation(3) << "\n"
    << this->impl_->roi_geo[2].geo_representation(2) << ", "
    << this->impl_->roi_geo[2].utm_ups_representation(3) << "\n"
    << this->impl_->roi_geo[3].geo_representation(2) << ", "
    << this->impl_->roi_geo[3].utm_ups_representation(3) );

  skip_status = this->check_skip(this->decode_roi(img), "Failed to decode pixels");
  if(skip_status != process::SUCCESS)
  {
    return skip_status;
  }

  return process::SUCCESS;
}


template<class PixType>
process::step_status
filename_frame_metadata_process<PixType>
::check_skip(bool condition, const std::string &message)
{
  if(!condition)
  {
    if(this->impl_->current_skip_count <= this->impl_->max_skip_count)
    {
      ++this->impl_->current_skip_count;
      LOG_WARN(this->name() << ": " << message << "  Skipping." );
      return process::SKIP;
    }
    else
    {
      LOG_ERROR(this->name() << ": " << message
                << "  Skip count exceeded.  Failing." );
      return process::FAILURE;
    }
  }
  return process::SUCCESS;
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::parse_framenum(void)
{
  if( this->impl_->framenum_regex.empty() )
  {
    this->timestamp_.set_frame_number(this->timestamp_.frame_number()+1);
    return true;
  }

  std::string fname = this->filename_;
  boost::cmatch m;
  if( ! boost::regex_search(fname.c_str(), m, this->impl_->framenum_regex) )
  {
    return false;
  }
  // match succeeded, thus we can assume the match object is defined

  // If there were multiple groupings found, we don't know which one to use,
  // so return false. (size returns num groupings + 1)
  if( m.size() - 1 > 1 )
  {
    LOG_ERROR( this->name() << ": Found more than one frame number grouping! "
               << "Don't know which one to take." );
    return false;
  }

  std::string str_fnum = m[1];

  try
  {
    unsigned int fnum = boost::lexical_cast < unsigned int >( str_fnum );
    this->timestamp_.set_frame_number( fnum );
    LOG_DEBUG( this->name() << ": Extracted frame number '" << fnum
               << "' from file name " << fname );

    return true;
  }
  catch(boost::bad_lexical_cast const& e)
  {
    LOG_ERROR( this->name() << ": Could not convert frame number regex match \""
               << str_fnum << "\" to integer." );

    return false;
  }
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::parse_pixel_roi(void)
{
  LOG_DEBUG( this->name() << ": parsing pixel ROI '" << this->pixel_roi_ << "'" );

  boost::cmatch m;
  if( ! boost::regex_match(this->pixel_roi_.c_str(), m, this->impl_->roi_regex))
  {
    LOG_ERROR( this->name() << ": Could not match roi regex to: '" << this->pixel_roi_ << "'" );
    return false;
  }

  try
  {
    // These first two lexical casts are here to prevent width/height from
    // negative. If they are, the lexical cast throws an error.
    int w = static_cast<int>(boost::lexical_cast<unsigned int>(m[1]));
    int h = static_cast<int>(boost::lexical_cast<unsigned int>(m[2]));
    int i0 = boost::lexical_cast<int>(m[3]);
    int j0 = boost::lexical_cast<int>(m[4]);

    this->impl_->roi_pixel = vgl_box_2d<int>(
      vgl_point_2d<int>(i0, j0), w, h, vgl_box_2d<int>::min_pos);
  }
  catch(const boost::bad_lexical_cast &e)
  {
    LOG_ERROR(this->name() << e.what());
    return false;
  }

  return true;
}


template<class PixType>
vil_stream_sptr
filename_frame_metadata_process<PixType>
::cache_file(void)
{
  LOG_INFO(this->name() << ": Opening " << this->filename_ );
  vul_timer timer;
  timer.mark();
  vil_stream_sptr s;
  switch(this->impl_->cache_mode)
  {
    case cache_mode_0_NONE:  s = this->cache_file_0_NONE();  break;
    case cache_mode_1_RAM:   s = this->cache_file_1_RAM();  break;
    case cache_mode_2_DISK:  s = this->cache_file_2_DISK();  break;
    default: s = NULL;
  }
  long ms = timer.all();

  LOG_INFO(this->name() << ": Caching operation completed in " << ms << "ms" );
  return s;
}


// No caching, just open the file and return the stream
template<class PixType>
vil_stream_sptr
filename_frame_metadata_process<PixType>
::cache_file_0_NONE(void)
{
  unsigned int num_tries = 0;
  while(true)
  {
    vil_stream_sptr s(new vil_stream_fstream(this->filename_.c_str(), "r"));
    ++num_tries;
    if(s->ok())
    {
      return s;
    }
    else
    {
      LOG_INFO(this->name() << ": Failed to open file: " << this->filename_);
      if(num_tries < this->impl_->retry_count + 1)
      {
        LOG_INFO(this->name() << ": Sleeping " << this->impl_->retry_sleep
                 << "us and retrying" );
        vpl_usleep(this->impl_->retry_sleep);
      }
      else
      {
        LOG_INFO(this->name() << ": Retry count exausted.  Aborting file read." );
        return NULL;
      }
    }
  }

  return NULL;
}


// Read the file into memory and return an in-memory stream
template<class PixType>
vil_stream_sptr
filename_frame_metadata_process<PixType>
::cache_file_1_RAM(void)
{
  // First open the file with all of it's various retry mechanisms
  vil_stream_sptr fin = cache_file_0_NONE();
  if(!fin || !fin->ok())
  {
    return NULL;
  }

  // Create an in-memory stream with 1MB chunks
  vil_streampos buf_size = 1048576;
  vil_streampos total_size = fin->file_size();
  vil_stream_sptr fcache(new vil_stream_core(static_cast<unsigned>(buf_size)));
  copy_stream(fin, fcache, buf_size, total_size);
  fcache->seek(0);

  return fcache;
}


template<class PixType>
vil_stream_sptr
filename_frame_metadata_process<PixType>
::cache_file_2_DISK(void)
{
  // First open the file with all of it's various retry mechanisms
  vil_stream_sptr fin = cache_file_0_NONE();
  if(!fin || !fin->ok())
  {
    return NULL;
  }

  std::string str_out;
  vil_stream_sptr fcache;
  try
  {
    // Determine the output path
    boost::filesystem::path path_in(this->filename_);
    boost::filesystem::path base_name = path_in.filename();
    boost::filesystem::path path_out =
      this->impl_->cache_mode_2_path / base_name;
    LOG_INFO(this->name() << ": Caching " << path_in << " to " << path_out );
    str_out = path_out.string();
  }
  catch(const boost::filesystem::filesystem_error &ex)
  {
    LOG_ERROR(this->name() << ": Unable to cache file: " << ex.what() );
    return NULL;
  }

  vil_streampos buf_size = 1048576;
  vil_streampos total_size = fin->file_size();
  fcache = new vil_stream_fstream(str_out.c_str(), "w+");
  copy_stream(fin, fcache, buf_size, total_size);

  if(!fcache->ok())
  {
    return NULL;
  }

  fcache->seek(0);
  return fcache;
}


template<class PixType>
void
filename_frame_metadata_process<PixType>
::copy_stream(vil_stream_sptr s_in, vil_stream_sptr s_out,
              vil_streampos buf_size, vil_streampos total_size)
{
  char *buf = new char[static_cast<size_t>(buf_size)];

  vil_streampos total_bytes_read = 0;
  while(total_bytes_read < total_size)
  {
    vil_streampos bytes_read;

    bytes_read = s_in->read(buf, buf_size);

    s_out->write(buf, bytes_read);
    total_bytes_read += bytes_read;
  }
  delete[] buf;
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::parse_metadata(vil_image_resource_sptr img)
{
  switch(this->impl_->metadata_mode)
  {
    case metadata_mode_0_NONE:  return this->parse_metadata_0_NONE(img);
    case metadata_mode_1_NITF:  return this->parse_metadata_1_NITF(img);
    default: return false;
  }
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::parse_metadata_0_NONE(vil_image_resource_sptr img)
{
  // No metadata, only img size
  this->impl_->full_size = vgl_point_2d<unsigned int>(img->ni(), img->nj());

  return true;
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::parse_metadata_1_NITF(vil_image_resource_sptr img)
{
#if defined(HAS_LTI_NITF2) || defined(HAS_VIL_GDAL)
  this->metadata_ = video_metadata();

  if(!this->parse_metadata_0_NONE(img))
  {
    return false;
  }


  std::string fmt(img->file_format());
  if(fmt != "nitf2"  && fmt != "GDALReadable")
  {
    LOG_ERROR(this->name() << ": NITF metadata mode set but image not type nitf2" );
    return false;
  }

  double ll[2];

  // Parse Upper Left
  if(!img->get_property("FRFC_LOC", ll))
  {
    LOG_ERROR(this->name() << ": Failed to retrieve FRFC_LOC property" );
    return false;
  }
  geo_coords frfc_loc(ll[0], ll[1]);
  LOG_INFO(this->name() << ": FRFC_LOC: " << frfc_loc.geo_representation(2) );
  this->impl_->full_geo[0] = frfc_loc;

  // Parse Upper Right
  if(!img->get_property("FRLC_LOC", ll))
  {
    LOG_ERROR(this->name() << ": Failed to retrieve FRLC_LOC property" );
    return false;
  }
  geo_coords frlc_loc(ll[0], ll[1]);
  frlc_loc.set_alt_zone(frfc_loc.zone());
  LOG_INFO(this->name() << ": FRLC_LOC: " << frlc_loc.geo_representation(2) );
  this->impl_->full_geo[1] = frlc_loc;

  // Parse Upper Left
  if(!img->get_property("LRLC_LOC", ll))
  {
    LOG_ERROR(this->name() << ": Failed to retrieve LRLC_LOC property" );
    return false;
  }
  geo_coords lrlc_loc(ll[0], ll[1]);
  lrlc_loc.set_alt_zone(frfc_loc.zone());
  LOG_INFO(this->name() << ": LRLC_LOC: " << lrlc_loc.geo_representation(2) );
  this->impl_->full_geo[2] = lrlc_loc;


  // Parse Upper Left
  if(!img->get_property("LRFC_LOC", ll))
  {
    LOG_ERROR(this->name() << ": Failed to retrieve LRFC_LOC property" );
    return false;
  }
  geo_coords lrfc_loc(ll[0], ll[1]);
  lrfc_loc.set_alt_zone(frfc_loc.zone());
  LOG_INFO(this->name() << ": LRFC_LOC: " << lrfc_loc.geo_representation(2) );
  this->impl_->full_geo[3] = lrfc_loc;

  // Parse the second level time
  std::time_t t_s;
  if(!img->get_property("IDATIM", &t_s))
  {
    LOG_ERROR(this->name() << ": Failed to retrieve IDATIM property" );
    return false;
  }
  LOG_INFO(this->name() << ": IDATIM: " << t_s );


  double t_ms = 0;
  if(!img->get_property("MILLISECONDS", &t_ms))
  {
    t_ms = 0;
  }
  LOG_INFO(this->name() << ": milliseconds : " << t_ms );

  // Combine time sources
  double t_ms_since_1970 = t_s * 1e3 + t_ms;
  this->timestamp_.set_time(t_ms_since_1970*1e3);
  this->metadata_.timeUTC(t_ms_since_1970);

  return true;
#else
  (void)img;
  return false;
#endif
}


template<class PixType>
void
filename_frame_metadata_process<PixType>
::build_transforms(void)
{
  std::vector<vgl_homg_point_2d<double> > geo_corners;  geo_corners.reserve(4);
  if(this->impl_->use_latlon_for_xfm)
  {
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[0].longitude(),
      this->impl_->full_geo[0].latitude()));
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[1].longitude(),
      this->impl_->full_geo[1].latitude()));
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[2].longitude(),
      this->impl_->full_geo[2].latitude()));
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[3].longitude(),
      this->impl_->full_geo[3].latitude()));
  }
  else
  {
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[0].alt_easting(),
      this->impl_->full_geo[0].alt_northing()));
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[1].alt_easting(),
      this->impl_->full_geo[1].alt_northing()));
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[2].alt_easting(),
      this->impl_->full_geo[2].alt_northing()));
    geo_corners.push_back(vgl_homg_point_2d<double>(
      this->impl_->full_geo[3].alt_easting(),
      this->impl_->full_geo[3].alt_northing()));
  }

  std::vector<vgl_homg_point_2d<double> > img_corners;  img_corners.reserve(4);
  img_corners.push_back(vgl_homg_point_2d<double>(0, 0));
  img_corners.push_back(vgl_homg_point_2d<double>(
    this->impl_->full_size.x()-1, 0));
  img_corners.push_back(vgl_homg_point_2d<double>(
    this->impl_->full_size.x()-1, this->impl_->full_size.y()-1));
  img_corners.push_back(vgl_homg_point_2d<double>(
    0, this->impl_->full_size.y()-1));

  vgl_h_matrix_2d_compute_4point solver;
  this->impl_->xfm_f2w = solver.compute(img_corners, geo_corners);
  this->impl_->xfm_w2f = solver.compute(geo_corners, img_corners);
}


template<class PixType>
void
filename_frame_metadata_process<PixType>
::build_geo_roi(void)
{

  vgl_homg_point_2d<double> img_points[4];
  //We are calculating for center of the pixel since that is where the point is located at.
  img_points[0].set(this->impl_->roi_pixel.min_x()+0.5,
                    this->impl_->roi_pixel.min_y()+0.5);
  img_points[1].set(this->impl_->roi_pixel.max_x()-0.5,
                    this->impl_->roi_pixel.min_y()+0.5);
  img_points[2].set(this->impl_->roi_pixel.max_x()-0.5,
                    this->impl_->roi_pixel.max_y()-0.5);
  img_points[3].set(this->impl_->roi_pixel.min_x()+0.5,
                    this->impl_->roi_pixel.max_y()-0.5);


  vgl_homg_point_2d<double> geo_points[4];
  geo_points[0] = this->impl_->xfm_f2w(img_points[0]);
  geo_points[1] = this->impl_->xfm_f2w(img_points[1]);
  geo_points[2] = this->impl_->xfm_f2w(img_points[2]);
  geo_points[3] = this->impl_->xfm_f2w(img_points[3]);

  if(this->impl_->use_latlon_for_xfm)
  {
    this->impl_->roi_geo[0].reset(geo_points[0].y()/geo_points[0].w(),
                                  geo_points[0].x()/geo_points[0].w());
    this->impl_->roi_geo[1].reset(geo_points[1].y()/geo_points[1].w(),
                                  geo_points[1].x()/geo_points[1].w());
    this->impl_->roi_geo[2].reset(geo_points[2].y()/geo_points[2].w(),
                                  geo_points[2].x()/geo_points[2].w());
    this->impl_->roi_geo[3].reset(geo_points[3].y()/geo_points[3].w(),
                                  geo_points[3].x()/geo_points[3].w());
  }
  else
  {
    int z = this->impl_->full_geo[0].zone();
    bool n = this->impl_->full_geo[0].is_north();
    this->impl_->roi_geo[0].reset(z, n,
      geo_points[0].x()/geo_points[0].w(),
      geo_points[0].y()/geo_points[0].w());
    this->impl_->roi_geo[1].reset(z, n,
      geo_points[1].x()/geo_points[1].w(),
      geo_points[1].y()/geo_points[1].w());
    this->impl_->roi_geo[2].reset(z, n,
      geo_points[2].x()/geo_points[2].w(),
      geo_points[2].y()/geo_points[2].w());
    this->impl_->roi_geo[3].reset(z, n,
      geo_points[3].x()/geo_points[3].w(),
      geo_points[3].y()/geo_points[3].w());
  }

  // Update the metadata sxtrcture. The .corner_*(...) methods copy, so
  // creating the tmp_ variable is ok.
  geo_coord::geo_lat_lon tmp_;

  tmp_.set_latitude(this->impl_->roi_geo[0].latitude());
  tmp_.set_longitude(this->impl_->roi_geo[0].longitude());
  this->metadata_.corner_ul(tmp_);

  tmp_.set_latitude(this->impl_->roi_geo[1].latitude());
  tmp_.set_longitude(this->impl_->roi_geo[1].longitude());
  this->metadata_.corner_ur(tmp_);

  tmp_.set_latitude(this->impl_->roi_geo[2].latitude());
  tmp_.set_longitude(this->impl_->roi_geo[2].longitude());
  this->metadata_.corner_lr(tmp_);

  tmp_.set_latitude(this->impl_->roi_geo[3].latitude());
  tmp_.set_longitude(this->impl_->roi_geo[3].longitude());
  this->metadata_.corner_ll(tmp_);
}


template<class PixType>
bool
filename_frame_metadata_process<PixType>
::decode_roi(vil_image_resource_sptr img)
{
  vul_timer decode_timer;
  decode_timer.mark();

  if(this->impl_->roi_pixel.min_x() >= 0 &&
     this->impl_->roi_pixel.min_y() >= 0 &&
     this->impl_->roi_pixel.max_x() <= static_cast<int>(this->impl_->full_size.x()) &&
     this->impl_->roi_pixel.max_y() <= static_cast<int>(this->impl_->full_size.y()))
  {
    this->img_ = img->get_copy_view(
      this->impl_->roi_pixel.min_x(), this->impl_->roi_pixel.width(),
      this->impl_->roi_pixel.min_y(), this->impl_->roi_pixel.height());
    if(!this->img_)
    {
      return false;
    }
  }
  else // roi is out of image
  {
    this->img_ = vil_image_view<PixType>();
    this->img_.set_size( this->impl_->roi_pixel.width(),
                         this->impl_->roi_pixel.height(),
                         img->nplanes() );
    this->img_.fill(0);

    vgl_box_2d<int> full_pixel( vgl_point_2d<int>(0, 0),
      this->impl_->full_size.x(), this->impl_->full_size.y(),
      vgl_box_2d<int>::min_pos);

    vgl_box_2d<int> roi_sub = vgl_intersection(
      this->impl_->roi_pixel, full_pixel);

    // Early exit if we are entirely out of bounds
    if(roi_sub.volume() == 0)
    {
      LOG_WARN(this->name() << ": ROI is entirely out of bounds");
      return !this->impl_->oob_skip;
    }

#ifndef NDEBUG
    LOG_DEBUG(this->name() << ": ROI_PRE_CROP  : " << this->impl_->roi_pixel);
    LOG_DEBUG(this->name() << ": ROI_FULL_FRAME: " << full_pixel);
    LOG_DEBUG(this->name() << ": ROI_POST_CROP : " << roi_sub);
#endif

    vil_image_view<PixType> img_sub = img->get_copy_view(
      roi_sub.min_x(), roi_sub.width(),
      roi_sub.min_y(), roi_sub.height());
    if(!img_sub)
    {
      return false;
    }

    unsigned int img_crop_i0 =
      roi_sub.min_x() - this->impl_->roi_pixel.min_x();
    unsigned int img_crop_j0 =
      roi_sub.min_y() - this->impl_->roi_pixel.min_y();
    vil_image_view<PixType> img_crop = vil_crop(this->img_,
      img_crop_i0, roi_sub.width(), img_crop_j0, roi_sub.height());

    img_crop.deep_copy(img_sub);
  }

  long decode_ms = decode_timer.real();
  LOG_INFO(this->name() << ": Decoding completed in " << decode_ms << "ms" );

  return true;
}

template<class PixType>
bool
filename_frame_metadata_process<PixType>
::parse_roi_geo(const std::string &s)
{
  if(s.empty())
  {
    return true;
  }

  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string tmp;
  while(ss >> tmp)
  {
    parts.push_back(tmp);
  }

  geo_coords cp, p0, p1, p2, p3;
  double radius;
  switch(parts.size())
  {
    case 3: // lat lon center point + radius
      if(parts[2][0] != '+')
      {
        LOG_ERROR(this->name() << ": Radius must be in the format \"+meters\"" );
        return false;
      }
      LOG_INFO(this->name() << ": Parsing 1 point Lat Lon ROI" );
      cp.reset(parts[0] + " " + parts[1]);
      ss.clear();
      ss.str("");
      ss << &parts[2][1];
      if(!(ss >> radius))
      {
        LOG_ERROR(this->name() << ": Unable to parse ROI radius" );
        return false;
      }
      p0.reset(cp.zone(), cp.is_north(),
               cp.easting() - radius, cp.northing() - radius);
      p1.reset(cp.zone(), cp.is_north(),
               cp.easting() + radius, cp.northing() - radius);
      p2.reset(cp.zone(), cp.is_north(),
               cp.easting() + radius, cp.northing() + radius);
      p3.reset(cp.zone(), cp.is_north(),
               cp.easting() - radius, cp.northing() + radius);
      break;
    case 4:
      if(parts[3][0] == '+')
      {
        LOG_INFO(this->name() << ": Parsing 1 point UTM ROI" );
        cp.reset(parts[0] + " " + parts[1] + " " + parts[2]);
        ss.clear();
        ss.str("");
        ss << &parts[3][1];
        if(!(ss >> radius))
        {
          LOG_ERROR(this->name() << ": Unable to parse ROI radius" );
          return false;
        }
        p0.reset(cp.zone(), cp.is_north(),
                 cp.easting() - radius, cp.northing() - radius);
        p1.reset(cp.zone(), cp.is_north(),
                 cp.easting() + radius, cp.northing() - radius);
        p2.reset(cp.zone(), cp.is_north(),
                 cp.easting() + radius, cp.northing() + radius);
        p3.reset(cp.zone(), cp.is_north(),
                 cp.easting() - radius, cp.northing() + radius);
      }
      else // 2 point lat lon
      {
        LOG_INFO(this->name() << ": Parsing 2 point Lat Lon ROI" );
        p0.reset(parts[0] + " " + parts[1]);
        p2.reset(parts[2] + " " + parts[3]);
        p1.reset(p0.latitude(), p2.longitude());
        p3.reset(p2.latitude(), p0.longitude());
      }
      break;
    case 8: // 4 point lat lon
      LOG_INFO(this->name() << ": Parsing 4 point Lat Lon ROI" );
      p0.reset(parts[0] + " " + parts[1]);
      p1.reset(parts[2] + " " + parts[3]);
      p2.reset(parts[4] + " " + parts[5]);
      p3.reset(parts[6] + " " + parts[7]);
      break;
    case 6: // 2 point UTM
      LOG_INFO(this->name() << ": Parsing 2 point UTM ROI" );
      p0.reset(parts[0] + " " + parts[1]  + " " + parts[2]);
      p2.reset(parts[3] + " " + parts[4]  + " " + parts[5]);
      p1.reset(p0.zone(), p0.is_north(), p2.easting(), p0.northing());
      p3.reset(p0.zone(), p0.is_north(), p0.easting(), p2.northing());
      break;
    case 12: // 4 point UTM
      LOG_INFO(this->name() << ": Parsing 4 point UTM ROI" );
      p0.reset(parts[0] + " " + parts[1]  + " " + parts[2]);
      p1.reset(parts[3] + " " + parts[4]  + " " + parts[5]);
      p2.reset(parts[6] + " " + parts[7]  + " " + parts[8]);
      p3.reset(parts[9] + " " + parts[10]  + " " + parts[11]);
      break;
    default:
      LOG_ERROR(this->name() << ": Unable to determine geo coordinate format.  "
      << "Please use all Lat Lon or all UTM coordinates" );
      return false;
  }
  if(!p0.is_valid())
  {
    LOG_ERROR(this->name() << ": ROI geo point 0 is invalid" );
    return false;
  }
  if(!p1.is_valid())
  {
    LOG_ERROR(this->name() << ": ROI geo point 1 is invalid" );
    return false;
  }
  if(!p2.is_valid())
  {
    LOG_ERROR(this->name() << ": ROI geo point 2 is invalid" );
    return false;
  }
  if(!p3.is_valid())
  {
    LOG_ERROR(this->name() << ": ROI geo point 3 is invalid" );
    return false;
  }
  p1.set_alt_zone(p0.zone());
  p2.set_alt_zone(p0.zone());
  p3.set_alt_zone(p0.zone());
  this->impl_->roi_corners_geo.clear();
  this->impl_->roi_corners_geo.push_back(p0);
  this->impl_->roi_corners_geo.push_back(p1);
  this->impl_->roi_corners_geo.push_back(p2);
  this->impl_->roi_corners_geo.push_back(p3);

  return true;
}

template<class PixType>
void
filename_frame_metadata_process<PixType>
::compute_pixel_roi(void)
{
  int z = this->impl_->full_geo[0].zone();
  vgl_homg_point_2d<double> geo_p0, geo_p1, geo_p2, geo_p3;
  LOG_ASSERT(this->impl_->roi_corners_geo.size() == 4, "Should have 4 points at this point");
  if(this->impl_->use_latlon_for_xfm)
  {
    // Transform the ROI into image space
    geo_p0.set(this->impl_->roi_corners_geo[0].longitude(),
                this->impl_->roi_corners_geo[0].latitude());
    geo_p1.set(this->impl_->roi_corners_geo[1].longitude(),
                this->impl_->roi_corners_geo[1].latitude());
    geo_p2.set(this->impl_->roi_corners_geo[2].longitude(),
                this->impl_->roi_corners_geo[2].latitude());
    geo_p3.set(this->impl_->roi_corners_geo[3].longitude(),
                this->impl_->roi_corners_geo[3].latitude());
  }
  else
  {
    // Transform the ROI into image space
    this->impl_->roi_corners_geo[0].set_alt_zone(z);
    this->impl_->roi_corners_geo[1].set_alt_zone(z);
    this->impl_->roi_corners_geo[2].set_alt_zone(z);
    this->impl_->roi_corners_geo[3].set_alt_zone(z);
    geo_p0.set(this->impl_->roi_corners_geo[0].alt_easting(),
                this->impl_->roi_corners_geo[0].alt_northing());
    geo_p1.set(this->impl_->roi_corners_geo[1].alt_easting(),
                this->impl_->roi_corners_geo[1].alt_northing());
    geo_p2.set(this->impl_->roi_corners_geo[2].alt_easting(),
                this->impl_->roi_corners_geo[2].alt_northing());
    geo_p3.set(this->impl_->roi_corners_geo[3].alt_easting(),
                this->impl_->roi_corners_geo[3].alt_northing());
  }

  //Project into image space.  From there we will calculate the pixel ROI.
  vgl_homg_point_2d<double> img_p0 = this->impl_->xfm_w2f(geo_p0);
  img_p0.set((img_p0.x() / img_p0.w()), (img_p0.y() / img_p0.w()));
  vgl_homg_point_2d<double> img_p1 = this->impl_->xfm_w2f(geo_p1);
  img_p1.set((img_p1.x() / img_p1.w()), (img_p1.y() / img_p1.w()));
  vgl_homg_point_2d<double> img_p2 = this->impl_->xfm_w2f(geo_p2);
  img_p2.set((img_p2.x() / img_p2.w()), (img_p2.y() / img_p2.w()));
  vgl_homg_point_2d<double> img_p3 = this->impl_->xfm_w2f(geo_p3);
  img_p3.set((img_p3.x() / img_p3.w()), (img_p3.y() / img_p3.w()));


  vgl_box_2d<double> roi_img;
  roi_img.add(vgl_point_2d<double>(img_p0.x(), img_p0.y()));
  roi_img.add(vgl_point_2d<double>(img_p1.x(), img_p1.y()));
  roi_img.add(vgl_point_2d<double>(img_p2.x(), img_p2.y()));
  roi_img.add(vgl_point_2d<double>(img_p3.x(), img_p3.y()));

  vgl_point_2d< double > min = roi_img.min_point();
  vgl_point_2d< double > max = roi_img.max_point();

  if(this->impl_->roi_w == UINT_INVALID || this->impl_->roi_h == UINT_INVALID)
  {
    // Determine the ROI size in pixels
    this->impl_->roi_w = static_cast<unsigned int>(std::ceil(max.x()) - std::floor(min.x()));
    this->impl_->roi_h = static_cast<unsigned int>(std::ceil(max.y()) - std::floor(min.y()));
  }
  vgl_point_2d< double > img_center((std::floor(min.x())+std::ceil(max.x()))*0.5, (std::floor(min.y())+std::ceil(max.y()))*0.5);

  double i0 = std::floor(img_center.x() - (this->impl_->roi_w / 2.0));
  double j0 = std::floor(img_center.y() - (this->impl_->roi_h / 2.0));
  double i1 = i0 + this->impl_->roi_w;
  double j1 = j0 + this->impl_->roi_h;

  this->impl_->roi_pixel = vgl_box_2d<int>( static_cast<int>(i0), static_cast<int>(i1),
                                            static_cast<int>(j0), static_cast<int>(j1) );

  this->build_geo_roi();
}



} // end namespace vidtk
