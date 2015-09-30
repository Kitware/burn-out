/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "identify_metadata_burnin_process_static.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vil/vil_convert.h>
#include <vil/vil_fill.h>

#include <vcl_fstream.h>

#include <opencv/cxcore.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>


namespace vidtk
{


static double dark_hist_prob( cv::Mat const& img, cv::Mat const& mask, int threshold )
{
  const int chans[] = { 0 };
  cv::MatND hist;
  const int histSize[] = { 256 };
  const float range[] = { 0, 255 };
  const float* ranges[] = { range };

  cv::calcHist( &img, 1, chans, mask, hist, 1, histSize, ranges );

  int val = 0;
  int val_bright = 0;

  for( int i = 0; i < threshold; ++i )
  {
    val += hist.at<int>( i );
  }
  for( int i = 0; 3 * i < threshold; ++i )
  {
    val_bright += hist.at<int>( 255 - i );
  }

  int total = cv::countNonZero( mask );

  return vcl_max( val, val_bright ) / double( total );
}


identify_metadata_burnin_process_static
::identify_metadata_burnin_process_static( vcl_string const& name )
  : process( name, "identify_metadata_burnin_process_static" )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Disable this process, causing the image to pass through unmodified." );

  config_.add_parameter(
    "thresh1",
    "50",
    "The first threshold for the hysteresis procedure" );

  config_.add_parameter(
    "thresh2",
    "250",
    "The second threshold for the hysteresis procedure" );

  config_.add_parameter(
    "aperture",
    "3",
    "Aperture size for edge detection." );

  config_.add_parameter(
    "useL2gradient",
    "false",
    "L2 -> more accurate, but slow; L1 -> accurate, but faster." );

  config_.add_parameter(
    "area_threshold",
    "0.33",
    "The minimum area needed to trigger a detection of a mask." );

  config_.add_parameter(
    "prob_threshold",
    "0.18",
    "The minimum probability value needed to trigger a detection of a mask." );

  config_.add_parameter(
    "histogram_detection_threshold",
    "80",
    "Threshold to use when using histogram detection." );

  config_.add_parameter(
    "segments_file",
    "",
    "File which contains segment definitions." );

  config_.add_parameter(
    "orientation_roi",
    "0 0 0 0",
    "Where to search within the image for the orientation pattern." );

  config_.add_parameter(
    "orientation_pattern",
    "",
    "File which contains the pattern for the orientation mask." );
}


identify_metadata_burnin_process_static
::~identify_metadata_burnin_process_static()
{
}


config_block
identify_metadata_burnin_process_static
::params() const
{
  return config_;
}


bool
identify_metadata_burnin_process_static
::set_params( config_block const& blk )
{
  vcl_string segments_file;
  vcl_string orientation_pattern;

  try
  {
    blk.get( "disabled", disabled_ );
    blk.get( "thresh1", thresh1_ );
    blk.get( "thresh2", thresh2_ );
    blk.get( "aperture", aperture_ );
    blk.get( "useL2gradient", useL2grad_ );
    blk.get( "area_threshold", area_thresh_ );
    blk.get( "prob_threshold", prob_thresh_ );
    blk.get( "histogram_detection_threshold", histogram_detection_thresh_ );
    blk.get( "segments_file", segments_file );
    blk.get( "orientation_roi", orientation_roi_ );
    blk.get( "orientation_pattern", orientation_pattern );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  vcl_ifstream fin;

  fin.open( segments_file.c_str() );

  if( !fin.good() )
  {
    log_error( this->name() << ": Unable to open segments file.\n" );
    return false;
  }

  while( !fin.eof() )
  {
    int n = -1;
    mask cur_mask;

    // Read number of segments
    fin >> n;
    if (n < 0)
    {
      log_error( this->name() << ": Error reading segment count.\n" );
      return false;
    }

    for( int i = 0; i < n; ++i )
    {
      segment s;
      int x = -1;
      int y = -1;

      // Start point
      fin >> x;
      fin >> y;
      if (x < 0 || y < 0)
      {
        log_error( this->name() << ": Error reading segments file (start point).\n" );
        return false;
      }

      s.start = vcl_make_pair( x, y );

      x = -1;
      y = -1;

      // end point
      fin >> x;
      fin >> y;
      if (x < 0 || y < 0)
      {
        log_error( this->name() << ": Error reading segments file (end point).\n" );
        return false;
      }

      s.end = vcl_make_pair( x, y );

      // line thickness
      fin >> s.thickness;

      cur_mask.segments.push_back( s );
    } // end for

    masks_.push_back( cur_mask );
  } // end while

  fin.close();

  if( !orientation_pattern.empty() )
  {
    orientation_mask_ = cv::imread( orientation_pattern, 0 );

    if( orientation_mask_.data == NULL )
    {
      log_error( this->name() << ": Unable to read orientation mask.\n" );
      return false;
    }

    // XXX: Magic constant
    const double magic_threshold = 75;
    cv::threshold( orientation_mask_, orientation_mask_, magic_threshold, 255, CV_THRESH_BINARY_INV );

    cv::dilate( orientation_mask_, orientation_mask_, cv::Mat() );
    cv::erode( orientation_mask_, orientation_mask_, cv::Mat() );
  }

  return true;
}


bool
identify_metadata_burnin_process_static
::initialize()
{
  this->invalid_image_ = false;
  return true;
}


bool
identify_metadata_burnin_process_static
::step()
{
  if( this->disabled_ )
  {
    return true;
  }

  if( this->invalid_image_ )
  {
    this->invalid_image_ = false;
    return false;
  }

  this->invalid_image_ = false;

  vcl_vector< vcl_pair< vil_image_view<vxl_byte>, cv::Mat > > seg_masks;

  vcl_vector< mask >::const_iterator i;
  vcl_vector< mask >::const_iterator end;

  for( i = masks_.begin(), end = masks_.end();
       i != end; ++i)
  {
    vil_image_view<vxl_byte> seg_mask( this->input_image_.ni(),
                                       this->input_image_.nj(), 1, 3 );
    vil_fill( seg_mask, vxl_byte(0) );
    cv::Mat segment_mask( seg_mask.nj(), seg_mask.ni(),
                          CV_8UC1, seg_mask.top_left_ptr() );

    vcl_vector< segment >::const_iterator j;
    vcl_vector< segment >::const_iterator seg_end;

    for( j = i->segments.begin(), seg_end = i->segments.end();
         j != seg_end; ++j )
    {
      cv::line( segment_mask,
                cv::Point( j->start.first, j->start.second ),
                cv::Point( j->end.first, j->end.second ),
                cv::Scalar_< int >( 255, 255, 255 ),
                j->thickness );
    }

    seg_masks.push_back( vcl_make_pair( seg_mask, segment_mask ) );
  }

  if ( this->byte_mask_.size() != this->input_image_.size() )
  {
    this->byte_mask_ = vil_image_view<vxl_byte>( this->input_image_.ni(),
                                                 this->input_image_.nj() );
  }
  vil_fill( byte_mask_, vxl_byte(0) );

  cv::Mat input( this->input_image_.nj(), this->input_image_.ni(),
                 ( this->input_image_.nplanes() == 1 ) ? CV_8UC1 : CV_8UC3,
                 this->input_image_.top_left_ptr() );
  cv::Mat end_mask( byte_mask_.nj(), byte_mask_.ni(),
                    CV_8UC1, byte_mask_.top_left_ptr() );
  cv::Mat gray;
  cv::Mat edges;

  cv::cvtColor( input, gray, CV_RGB2GRAY );

  cv::Canny( gray, edges, this->thresh1_, this->thresh2_, this->aperture_, this->useL2grad_ );
  cv::dilate( edges, edges, cv::Mat() );
  cv::erode( edges, edges, cv::Mat() );

  bool have_burnin = false;

  vcl_vector< vcl_pair< vil_image_view<vxl_byte>, cv::Mat > >::const_iterator m;
  vcl_vector< vcl_pair< vil_image_view<vxl_byte>, cv::Mat > >::const_iterator m_end;

  for( m = seg_masks.begin(), m_end = seg_masks.end();
       m != m_end; ++m)
  {
    cv::Mat buf = cv::Mat::zeros( input.rows, input.cols, CV_8UC1 );

    m->second.copyTo( buf, edges );

    const double area = double( cv::countNonZero( buf ) ) / double( cv::countNonZero( m->second ) );
    const double hist = dark_hist_prob( gray, m->second, this->histogram_detection_thresh_ );

    if( area > this->area_thresh_ && hist > this->prob_thresh_ )
    {
      cv::bitwise_or( end_mask, m->second, end_mask );
      have_burnin = true;
    }
  }

  if( this->orientation_mask_.data && have_burnin )
  {
    cv::Mat res = cv::Mat( this->orientation_roi_[2] - this->orientation_roi_[0] - orientation_mask_.rows - 1,
                           this->orientation_roi_[3] - this->orientation_roi_[1] - orientation_mask_.cols + 1,
                           CV_32FC1 );
    cv::Mat gray_roi = gray( cv::Range( this->orientation_roi_[1], this->orientation_roi_[3] ),
                             cv::Range( this->orientation_roi_[0], this->orientation_roi_[2] ) );
    cv::matchTemplate( gray_roi, this->orientation_mask_, res, CV_TM_CCORR_NORMED );

    cv::Point max;

    cv::minMaxLoc( res, NULL, NULL, NULL, &max );

    cv::Mat mask_roi = end_mask( cv::Range( this->orientation_roi_[1] + max.x,
                                            this->orientation_roi_[1] + max.x + this->orientation_mask_.rows ),
                                 cv::Range( this->orientation_roi_[0] + max.y,
                                            this->orientation_roi_[0] + max.y + this->orientation_mask_.cols ) );

    this->orientation_mask_.copyTo( mask_roi );
  }

  vil_convert_cast< vxl_byte, bool >( byte_mask_, this->mask_ );

  return true;
}


void
identify_metadata_burnin_process_static
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  if( img.nplanes() != 1 && img.nplanes() != 3 )
  {
    log_error( "Input image does not have either 1 or 3 channels.\n" );
    invalid_image_ = true;
    return;
  }

  this->input_image_ = vil_image_view<vxl_byte>( img.ni(), img.nj(), 1, img.nplanes() );
  vil_copy_reformat( img, this->input_image_ );

  if( ( this->input_image_.nplanes() != 1 && this->input_image_.planestep() != 1 ) ||
      this->input_image_.istep() != this->input_image_.nplanes() ||
      this->input_image_.jstep() != ( this->input_image_.ni() * this->input_image_.nplanes() ) )
  {
    log_error( "Input image memory is not aligned for OpenCV.\n" );
    invalid_image_ = true;
    return;
  }
}


vil_image_view<bool> const&
identify_metadata_burnin_process_static
::metadata_mask() const
{
  return this->mask_;
}

} // end namespace vidtk
