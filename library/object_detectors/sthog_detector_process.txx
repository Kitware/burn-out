/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include <vxl_config.h>
#include <vil/vil_math.h>
#include <vil/vil_convert.h>

#include <object_detectors/sthog_detector_process.h>

#include <opencv2/core/core.hpp>
#include <boost/tokenizer.hpp>

#include <logger/logger.h>
VIDTK_LOGGER("sthog_detector_process_txx");

namespace vidtk
{

template <class PixType>
sthog_detector_process<PixType>
::sthog_detector_process( std::string const& _name )
  : process( _name, "sthog_detector_process" )
{
}


template <class PixType>
sthog_detector_process<PixType>
::~sthog_detector_process()

{
}


template <class PixType>
config_block
sthog_detector_process<PixType>
::params() const
{
  config_block config_;

  config_.add_parameter(
    "disabled",
    "false",
    "Disable this process." );

  config_.add_parameter(
    "model_path_root",
    "",
    "Root path to the model files." );

  config_.add_parameter(
    "model_filenames",
    "",
    "A space delimited list of learned model files of the object of interest, e.g. linear svm model for sthog features.Multiple models can be specified, e.g. for multiple orientations.");

  config_.add_parameter(
    "det_threshold",
    "0.0",
    "Detection threshold on the raw classification score. Multiple thresholds can be specified, one for each model.");

  config_.add_parameter(
    "resize_scale",
    "1.0",
    "Scale factor to resize the detection image.");

  config_.add_parameter(
    "group_threshold",
    "2",
    "Group threshold for non-maximum suppresssion.");

  config_.add_parameter(
    "det_levels",
    "1",
    "Number of hierachical detections. It can be used to detect objects with different scales/sizes.");

  config_.add_parameter(
    "output_file",
    "",
    "Output file containing detection results. One line per detection: frame_name, detection_scores, followed by four corners starting from upper left and running counter clockwise." );

  config_.add_parameter(
    "output_image_pattern",
    "im%05d.png",
    "Output image file pattern");

  config_.add_parameter(
    "sthog_frames",
    "1",
    "Number of frames/slices in STHOG configuration" );

  config_.add_parameter(
    "hog_window_size",
    "64 64",
    "Window size of HOG configuration" );

   config_.add_parameter(
    "hog_stride_step",
    "8 8",
    "Stride step in HOG configuration, i.e. moving window step size." );

   config_.add_parameter(
    "det_tile_size",
    "0 0",
    "It breaks large image into smaller tiles" );

   config_.add_parameter(
     "det_tile_margin",
     "0",
     "Overlap pixels between tiles" );

   config_.add_parameter(
     "hog_cell_size",
     "8 8",
     "Cell size of HOG configuration." );

   config_.add_parameter(
     "hog_block_size",
     "16 16",
     "Block size of HOG configuration." );

   config_.add_parameter(
     "hog_orientations",
     "9",
     "Number of angular orientations in HOG configuration" );

   config_.add_parameter(
     "write_image_mode",
     "detection_only",
     "Sets the configuration for writing out images\n"
     "Options are:\n"
     "\tnone: Don't write out any png data.\n"
     "\tall: Write out all images with or without detection.\n"
     "\tregion_only: Only write the region data to images.\n"
     "\tdetection_only: Only write out the images with detections.\n"

);

  return config_;
}


template <class PixType>
bool
sthog_detector_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );
    if ( !disabled_ )
    {

      std::string model_path_root = blk.get<std::string>("model_path_root");

      std::string model_files_string =
        blk.get<std::string>( "model_filenames" );
      boost::char_separator<char> sep(" ");
      boost::tokenizer<boost::char_separator<char> >
        tokens(model_files_string, sep);

      model_filename_vec_.clear();
      boost::tokenizer<boost::char_separator<char> >::iterator it =
        tokens.begin();
      for (; it != tokens.end(); ++it)
      {
        std::string fullpath = model_path_root + *it;
        model_filename_vec_.push_back(fullpath);
        LOG_DEBUG( "model : " << fullpath );
      }

      det_threshold_ = blk.get<double>( "det_threshold" );
      LOG_DEBUG("threshold  = " << det_threshold_ );

      group_threshold_ = blk.get<double>( "group_threshold" );
      det_levels_ = blk.get<int>( "det_levels" );
      resize_scale_ = blk.get<float>( "resize_scale" );
      det_tile_size_ = blk.get<vnl_int_2>( "det_tile_size" );
      det_tile_margin_ = blk.get<int>( "det_tile_margin" );
      output_file_ = blk.get<std::string>( "output_file" );
      output_image_pattern_ = blk.get<std::string>( "output_image_pattern" );
      sthog_frames_ = blk.get<int>( "sthog_frames" );
      hog_window_size_ = blk.get<vnl_int_2>( "hog_window_size" );
      hog_stride_step_ = blk.get<vnl_int_2>( "hog_stride_step" );

      std::string img_wrt_md = blk.get<std::string>( "write_image_mode" );
      if (img_wrt_md == "none")
      {
        write_image_mode_ = sthog_detector::WRITE_NONE;
      }
      else if (img_wrt_md == "all")
      {
        write_image_mode_ = sthog_detector::WRITE_ALL;
      }
      else if (img_wrt_md == "detection_only")
      {
        write_image_mode_ = sthog_detector::WRITE_DETECTION_ONLY;
      }
      else if (img_wrt_md == "region_only")
      {
        write_image_mode_ = sthog_detector::WRITE_DETECTION_REGION_ONLY;
      }
      else
      {
        write_image_mode_ = sthog_detector::WRITE_INVALID;
        throw config_block_parse_error("Invalid write_image mode: " + img_wrt_md);
      }

      if( model_filename_vec_.size() <= 0 )
      {
        throw config_block_parse_error("Need one or more model files. Provided " + model_filename_vec_.size() );
      }
    }
  }
  catch( const config_block_parse_error &e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  return true;
}


template <class PixType>
bool
sthog_detector_process<PixType>
::initialize()
{
  return true;
}


template <class PixType>
bool
sthog_detector_process<PixType>
::step()
{
  clear_outputs();

  if( disabled_ )
  {
    clear_inputs();
    return true;
  }

  if( this->input_image_.nplanes() != 1 )
  {
    LOG_ERROR( this->name() << ": Image format not supported; single plane (greyscale) images expected, received "
      << this->input_image_.nplanes() << " plane image instead.");
    clear_inputs();
    return false;
  }

  cv::Mat ref_im;
  switch( vil_pixel_format_of( PixType() ) )
  {
  case VIL_PIXEL_FORMAT_BYTE:
    ref_im = cv::Mat( this->input_image_.nj(), this->input_image_.ni(),
                      CV_8UC1, this->input_image_.top_left_ptr() );
    break;
  case VIL_PIXEL_FORMAT_UINT_16:
    ref_im = cv::Mat( this->input_image_.nj(), this->input_image_.ni(),
                      CV_16UC1, this->input_image_.top_left_ptr() );
    break;
  default:
    LOG_ERROR( this->name() << ": Image format not supported");
    clear_inputs();
    return false;
  }

  sthog_detector detector( model_filename_vec_, det_threshold_, output_file_, group_threshold_, sthog_detector::SHOW_NONE, write_image_mode_, output_image_pattern_, hog_window_size_[0], hog_window_size_[1], resize_scale_, det_tile_size_[0], det_tile_size_[1], det_tile_margin_);

  detector.detect( ref_im, det_levels_, cv::Size(hog_stride_step_[0], hog_stride_step_[1]) );

  std::vector<cv::Rect> bbox=detector.get_found_filtered();
  std::vector<double> scores=detector.get_hit_scores_filtered();

  if( bbox.size() != scores.size() )
  {
    LOG_ERROR( "The number of detections and the number of scores are different." );
    clear_inputs();
    return false;
  }

  std::vector<cv::Rect>::iterator it = bbox.begin();
  std::vector<double>::iterator it_sc = scores.begin();

  //We are already assured that bbox and scores are the same size so we can trust ++it, ++it_sc.
  while(it!=bbox.end() && it_sc!=scores.end())
  {
    // when we return from the OpenCV padded image, we can go negative
    // which the unsigned bbox is not happy with.
    image_object_sptr st = new image_object;
    if (it->x < 0 )
    {
      it->width += it->x;
      it->x = 0;
    }
    if (it->y < 0 )
    {
      it->height += it->y;
      it->y = 0;
    }

    //set bounding box and img_loc
    st->set_bbox( it->x, it->x + it->width, it->y, it->y + it->height );
    st->set_image_loc( st->get_bbox().centroid_x(),  st->get_bbox().centroid_y() );

    st->set_confidence( *it_sc );
    detection_boxes_.push_back( st );

    ++it;
    ++it_sc;
  }

  for(unsigned i = 0; i < bbox.size(); ++i)
  {
    cv::Rect r = bbox[i];
    LOG_DEBUG( i+1 << " " << scores[i] << " " << r.x << " " << r.y << " " << r.x + r.width << " " << r.y + r.height << " " << r.x + r.width << " " << r.y );
  }

  clear_inputs();

  return true;
}

template <class PixType>
void
sthog_detector_process<PixType>
::clear_inputs()
{
  this->input_image_ = vil_image_view<PixType>();
}

template <class PixType>
void
sthog_detector_process<PixType>
::clear_outputs()
{
  this->detection_boxes_.clear();
}

template <class PixType>
void
sthog_detector_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{

  if( img.nplanes() != 1 && img.nplanes() != 3)
  {
    LOG_ERROR( "Input image does not have 1 or 3 channels." );
    return;
  }

  // convert to gray scale
  vil_image_view<PixType> grey;
  if( img.nplanes() == 3 )
  {
    vil_convert_planes_to_grey( img, grey );
  }
  else
  {
    grey = img;
  }

  this->input_image_ = vil_image_view<PixType>( img.ni(), img.nj(), 1, 1 );
  vil_copy_reformat( grey, this->input_image_ );

  if( ( this->input_image_.nplanes() != 1 && this->input_image_.planestep() != 1 ) ||
      this->input_image_.istep() != this->input_image_.nplanes() ||
      this->input_image_.jstep() != ( this->input_image_.ni() * this->input_image_.nplanes() ) )
  {
    LOG_ERROR( "Input image memory is not aligned for OpenCV." );
    return;
  }

  return;
}

template <class PixType>
std::vector< image_object_sptr >
sthog_detector_process<PixType>
::detect_object() const
{
  return detection_boxes_;
}


} // end namespace vidtk
