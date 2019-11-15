/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ocv_training_display.h"

#include <utilities/vxl_to_cv_converters.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

VIDTK_LOGGER("ocv_training_display");

namespace vidtk
{

std::string get_designation_from_user( const vil_image_view< vxl_byte >& image,
                                       const vgl_box_2d< unsigned >& region,
                                       const std::string& window_name,
                                       const vil_rgb<vxl_byte>& color )
{
  cv::Mat ocv_image;
  cv::Rect ocv_region;
  std::string output;

  deep_cv_conversion( image, ocv_image );
  convert_to_rect( region, ocv_region );
  cv::rectangle( ocv_image, ocv_region, cv::Scalar( color.b, color.g, color.r ), 2 );

  cv::namedWindow( window_name, CV_WINDOW_AUTOSIZE );
  cv::imshow( window_name, ocv_image );
  cv::waitKey( 30 );

  std::cout << "Enter Designation: ";
  std::cin >> output;
  return output;
}

void show_regions_to_user( const vil_image_view< vxl_byte >& image,
                           const std::vector< vgl_box_2d< unsigned > >& regions,
                           const std::string& window_name,
                           const unsigned ms_delay  )
{
  cv::Mat ocv_image;
  cv::Rect ocv_region;

  deep_cv_conversion( image, ocv_image );

  for( unsigned i = 0; i < regions.size(); i++ )
  {
    convert_to_rect( regions[i], ocv_region );
    cv::rectangle( ocv_image, ocv_region, cv::Scalar( 0, 0, 255 ), 2 );
  }

  cv::namedWindow( window_name, CV_WINDOW_AUTOSIZE );
  cv::imshow( window_name, ocv_image );
  cv::waitKey( ms_delay );
}


void show_regions_to_user( const vil_image_view< vxl_byte >& image,
                           const std::vector< std::pair< vgl_box_2d< unsigned >, double > >& regions,
                           const std::string& window_name,
                           const unsigned ms_delay )
{
  cv::Mat ocv_image;
  cv::Rect ocv_region;

  deep_cv_conversion( image, ocv_image );

  for( unsigned i = 0; i < regions.size(); i++ )
  {
    convert_to_rect( regions[i].first, ocv_region );
    double white_scale = std::max( regions[i].second, 0.0 );
    vxl_byte bg_value = ( white_scale > 1.0 ? 255 : ( 1.0 - white_scale ) * 255 );
    cv::rectangle( ocv_image, ocv_region, cv::Scalar( bg_value, bg_value, 255 ), 2 );
  }

  cv::namedWindow( window_name, CV_WINDOW_AUTOSIZE );
  cv::imshow( window_name, ocv_image );
  cv::waitKey( ms_delay );
}

void show_training_regions( const vil_image_view< vxl_byte >& image,
                            const std::vector< vgl_box_2d< unsigned > >& pos_regions,
                            const std::vector< vgl_box_2d< unsigned > >& neg_regions,
                            const std::vector< vgl_point_2d< int > >& pos_points,
                            const std::vector< vgl_point_2d< int > >& neg_points,
                            const std::string& window_name,
                            const unsigned ms_delay )
{
  cv::Mat ocv_image;
  cv::Rect ocv_region;
  cv::Point ocv_point;

  deep_cv_conversion( image, ocv_image );

  for( unsigned i = 0; i < pos_regions.size(); i++ )
  {
    convert_to_rect( pos_regions[i], ocv_region );
    cv::rectangle( ocv_image, ocv_region, cv::Scalar( 0, 255, 0 ), 2 );
  }
  for( unsigned i = 0; i < neg_regions.size(); i++ )
  {
    convert_to_rect( neg_regions[i], ocv_region );
    cv::rectangle( ocv_image, ocv_region, cv::Scalar( 0, 0, 255 ), 2 );
  }
  for( unsigned i = 0; i < pos_points.size(); i++ )
  {
    convert_to_point( pos_points[i], ocv_point );
    cv::circle( ocv_image, ocv_point, 1, cv::Scalar( 100, 200, 34 ), 1 );
  }
  for( unsigned i = 0; i < neg_points.size(); i++ )
  {
    convert_to_point( neg_points[i], ocv_point );
    cv::circle( ocv_image, ocv_point, 2, cv::Scalar( 0, 69, 255 ), 1 );
  }

  cv::namedWindow( window_name, CV_WINDOW_AUTOSIZE );
  cv::imshow( window_name, ocv_image );
  cv::waitKey( ms_delay );
}

}
