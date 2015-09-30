/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_identify_metadata_burnin_process_h_
#define vidtk_video_identify_metadata_burnin_process_h_

#include "identify_metadata_burnin_process.h"

#include <opencv/cxcore.h>

#include <vcl_deque.h>

namespace vidtk
{

class identify_metadata_burnin_process_impl_opencv
  : public identify_metadata_burnin_process_impl
{
public:
  bool set_params( config_block const& );

  bool initialize();
  bool step();

  void set_input_image( vil_image_view< vxl_byte > const& img );

  struct metadata
  {
    cv::Point center;
    vcl_vector< cv::Rect > brackets;
    cv::Rect rectangle;
    vcl_vector< cv::Rect > text;
    vcl_vector< cv::Point > stroke;
  };

  void detect_cross( const cv::Mat& edges, cv::Point& center );
  void detect_bracket( const cv::Mat& edges, const cv::Rect& roi, vcl_vector< cv::Rect >& brackets );
  void detect_rectangle( const cv::Mat& edge_image, const vcl_vector< cv::Rect >& brackets, cv::Rect& rect );
  void detect_text( const cv::Mat & bands, const cv::Mat& edges, const cv::Rect& roi, vcl_vector< cv::Rect >& text );
  void draw_cross( cv::Mat& img, const cv::Point& center, double clr, int width );
  void draw_bracket( cv::Mat& img, const cv::Rect& rect, vxl_uint_8 clr, int width );
  void draw_bracket_tl( vcl_vector< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_bracket_tr( vcl_vector< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_bracket_bl( vcl_vector< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_bracket_br( vcl_vector< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_rectangle( cv::Mat& img, const cv::Rect& rect, vxl_uint_8 clr, int width );
  void draw_rectangle( vcl_vector< cv::Point >& pts, const cv::Rect& rect, int width );
  void draw_text( cv::Mat& img, const cv::Rect& rect, vxl_uint_8 clr );
  void draw_metadata_mask( cv::Mat& img, const metadata& md );
  void log_detection_stats( const metadata& md );

  bool invalid_image_;

  vil_image_view<vxl_byte> byte_mask_;

  // Text templates
  vcl_vector< vcl_deque< cv::Rect > > text_temporal_mem_;
  vcl_vector< int > text_temporal_sum_;
  vcl_vector< vcl_pair< cv::Mat, cv::Mat > > template_imgs_;
};

} // end namespace vidtk

#endif
