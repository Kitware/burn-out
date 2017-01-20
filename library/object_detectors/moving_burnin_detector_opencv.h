/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_moving_burnin_detector_process_h_
#define vidtk_video_moving_burnin_detector_process_h_

#include "moving_burnin_detector_process.h"

#include <opencv/cxcore.h>

#include <deque>
#include <vector>

namespace vidtk
{

class moving_burnin_detector_opencv
  : public moving_burnin_detector_impl
{
public:

  bool set_params( config_block const& );
  bool initialize();
  bool step();

  void set_input_image( vil_image_view< vxl_byte > const& img );
  void scale_params_for_image( vil_image_view< vxl_byte > const& img );

  struct metadata
  {
    cv::Point center;
    std::vector< cv::Rect > brackets;
    cv::Rect rectangle;
    std::vector< cv::Rect > text;
    std::vector< cv::Point > stroke;
  };

  void detect_cross( const cv::Mat& edges, cv::Point& center );
  void detect_bracket( const cv::Mat& edges, const cv::Rect& roi, std::vector< cv::Rect >& brackets );
  void detect_rectangle( const cv::Mat& edge_image, const std::vector< cv::Rect >& brackets, cv::Rect& rect );
  void detect_text( const cv::Mat & bands, const cv::Mat& edges, const cv::Rect& roi, std::vector< cv::Rect >& text );
  void draw_cross( cv::Mat& img, const cv::Point& center, double clr, int width );
  void draw_bracket( cv::Mat& img, const cv::Rect& rect, vxl_uint_8 clr, int width );
  void draw_bracket_tl( std::set< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_bracket_tr( std::set< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_bracket_bl( std::set< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_bracket_br( std::set< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_rectangle( cv::Mat& img, const cv::Rect& rect, vxl_uint_8 clr, int width );
  void draw_rectangle( std::set< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_text( cv::Mat& img, const cv::Rect& rect, vxl_uint_8 clr );
  void draw_metadata_mask( cv::Mat& img, const metadata& md );
  void log_detection_stats( const metadata& md );

  bool invalid_image_;

  vil_image_view<vxl_byte> byte_mask_;

  // Text templates
  std::vector< std::deque< cv::Rect > > text_temporal_mem_;
  std::vector< int > text_temporal_sum_;
  std::vector< std::pair< cv::Mat, cv::Mat > > template_imgs_;
};

} // end namespace vidtk

#endif
