/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */



#include "kw_archive_writer_process.h"

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER("kw_archive_writer_process");

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
  template< class PixType >
  kw_archive_writer_process< PixType >::
  kw_archive_writer_process(std::string const& _name)
    : process(_name, "kw_archive_writer_process"),
      disable_(true),
      separate_meta_(true),
      compress_image_(true),
      gsd_( 0 ),
      archive_writer_(0)
  {
    config_.add_parameter("disabled", "true", "");
    config_.add_parameter("output_directory", ".", "");
    config_.add_parameter("base_filename", "kw_archive",
                           "Base filename (no extension)");
    config_.add_parameter("separate_meta", "true",
                           "Whether to write separate .meta file");
    config_.add_parameter("mission_id", "", "mission id to store in archive");
    config_.add_parameter("stream_id", "", "stream id to store in archive");
    config_.add_parameter("compress_image", "true",
                 "Whether to compress image data stored in archive");

    // Obsolete parameters
    // Replace by output_directory & base_filename
    config_.add_parameter("file_path", "OBSOLETE");
    config_.add_parameter("file_name", "OBSOLETE");
  }


  template< class PixType >
  kw_archive_writer_process< PixType >::
  ~kw_archive_writer_process()
  {
    delete archive_writer_;
  }


// ----------------------------------------------------------------
/** Collect parameters from this process.
 *
 * This method returns a bonfig block that contains all the
 * parameters needed by this process.
 *
 * @return Config block with all parameters.
 */
  template< class PixType >
  config_block kw_archive_writer_process< PixType >::
  params() const
  {
    return (config_);
  }


// ----------------------------------------------------------------
/** Set values for parameters.
 *
 * This method is called with an updated config block that
 * contains the most current configuration values. We will
 * query our parameters from this block.
 *
 * @param[in] blk - updated config block
 */
  template< class PixType >
  bool kw_archive_writer_process< PixType >::
  set_params( config_block const& blk)
  {
    // Check for obsolete params
    if (blk.has("file_path") || blk.has("file_name"))
    {
      std::string message =
        "Obsolete config parameters for kw_archive_writer_process. "
        "Parameters \"file_path\" and \"file_name\" "
        "have been replaced by \"output_directory\" and \"base_filename\". "
        "kw_archive_writer_process disabled.";
        LOG_ERROR(message);
      disable_ = true;
      return false;
    }

    try
    {
      disable_ = blk.get<bool>("disabled");
      output_directory_ = blk.get<std::string>("output_directory");
      base_filename_ = blk.get<std::string>("base_filename");
      separate_meta_ = blk.get<bool>("separate_meta");
      mission_id_ = blk.get<std::string>("mission_id");
      stream_id_ = blk.get<std::string>("stream_id");
      compress_image_ = blk.get<bool>("compress_image");
    }
    catch( config_block_parse_error const& e)
    {
      LOG_ERROR( this->name() << ": set_params failed: "
                 << e.what() );
      return false;
    }

    return (true);
  }


// ----------------------------------------------------------------
/** Initialize this process.
 *
 * This initialize method is called once after the parameters
 * have been supplied. It is our duty to initialize this process
 * and make it ready to operate.
 */
  template< class PixType >
  bool kw_archive_writer_process< PixType >::
  initialize()
  {
    if ( disable_ )
    {
      return (true);
    }

    // Create and initialize the real writer
    this->archive_writer_ = new vidtk::kw_archive_index_writer< PixType >();
    std::string path = output_directory_ + "/" + base_filename_;
    typename vidtk::kw_archive_index_writer< PixType >::open_parameters writer_params =
      typename vidtk::kw_archive_index_writer< PixType >::open_parameters()
      .set_base_filename(path)
      .set_separate_meta(separate_meta_)
      .set_overwrite(true)
      .set_mission_id(mission_id_)
      .set_stream_id(stream_id_)
      .set_compress_image(compress_image_);

    bool status = archive_writer_->open(writer_params);
    return status;
  }


// ----------------------------------------------------------------
/** Main processing method.
 *
 * This method is called once per iteration of the pipeline.
 */
  template< class PixType >
  bool kw_archive_writer_process< PixType >::
  step()
  {
    // Need to return true in case we are in an async pipeline.
    // Need to consume inputs so source node will not block.
    if ( disable_ )
    {
      return (true);
    }

    // Test for invalid input - means we have reached end of video
    if (! timestamp_.is_valid() )
    {
      LOG_DEBUG(name() << ": invalid time stamp seen - assume EOF"); // TEMP
      return (false);
    }

    if ( ! timestamp_.has_time())
    {
      LOG_WARN ("Must have valid time in timestamp input.");
      return (true);
    }

    // Write frame
    bool status = archive_writer_->write_frame(timestamp_,
                                               corner_points_,
                                               image_,
                                               frame_to_ref_,
                                               gsd_);
    return status;
  }


// ================================================================
// -- inputs --
template< class PixType >
void kw_archive_writer_process< PixType >::
set_input_timestamp ( vidtk::timestamp const& val)
{
  timestamp_ = val;
}


template< class PixType >
void kw_archive_writer_process< PixType >::
set_input_corner_points(vidtk::video_metadata const& val)
{
  corner_points_ = val;
}


template< class PixType >
void kw_archive_writer_process< PixType >::
set_input_src_to_ref_homography ( vidtk::image_to_image_homography const& val)
{
  frame_to_ref_ = val;
}


template< class PixType >
void kw_archive_writer_process< PixType >::
set_input_world_units_per_pixel ( double val )
{
  gsd_ = val;
}


template< class PixType >
void kw_archive_writer_process< PixType >::
set_input_image ( vil_image_view < PixType > const& val)
{
  image_ = val;
}

} // end namespace
