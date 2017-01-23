/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_properties_border_detection_h_
#define vidtk_video_properties_border_detection_h_

#include <tracking_data/image_border.h>

#include <vil/vil_image_view.h>

#include <vector>


/// @file border_detection.h
/// @brief Misc helper functions for detecting borders in video or images


namespace vidtk
{


/// Settings for the solid color border detection algorithm
template< typename PixType >
struct border_detection_settings
{
  /// Border detection method, should we look for a specific coloured
  /// border or auto-detect the color?
  enum { AUTO, BLACK, WHITE, SPECIFIED_COLOR } detection_method_;

  /// Number of pixels to dilute the detected border by
  unsigned side_dilation_;

  /// Pixel sampling rate (sample every 1 in sample_rate pixels)
  unsigned sample_rate_;

  /// Color, if manually specified. Must have an entry for each input plane.
  std::vector< PixType > color_;

  /// Allowable variance from the given colour
  PixType default_variance_;

  /// Allowable variance from the given colour for HD data
  PixType high_res_variance_;

  /// The percentage of pixels in a row or column which must be the given color
  double required_percentage_;

  /// Number of invalid rows or columns required before we consider the border terminated
  unsigned invalid_count_;

  /// Whether or not we should run edge detection to refine detected borders
  bool edge_refinement_;

  /// Edge search radius in pixels
  unsigned edge_search_radius_;

  /// Percentage of edge pixels required for border qualifications
  double edge_percentage_req_;

  /// Edge threshold for border qualifications
  PixType edge_threshold_;

  /// If color type is auto, only search for common border colors (grays and blacks)
  bool common_colors_only_;

  // Default constructor
  border_detection_settings()
  : detection_method_( AUTO ),
    side_dilation_( 3 ),
    sample_rate_( 2 ),
    color_(),
    default_variance_( 10 ),
    high_res_variance_( 4 ),
    required_percentage_( 0.95 ),
    invalid_count_( 1 ),
    edge_refinement_( false ),
    edge_search_radius_( 6 ),
    edge_percentage_req_( 0.25 ),
    edge_threshold_( 5 ),
    common_colors_only_( false )
  {}
};


/// Functions to try and detect solid coloured rectangular image borders. Scans
/// each border row and column outwards from the sides of the image until a certain
/// percentage of pixels are not within some variance of some color. If in auto mode,
/// will and attempt to estimate a border colour if one exists.
template< typename PixType >
void detect_solid_borders( const vil_image_view< PixType >& input_image,
                           const border_detection_settings< PixType >& settings,
                           image_border& output );


/// Solid border detector variant which expects a color and pre-computed grayscale
/// image. The grayscale version is used for the edge refinement stage.
template< typename PixType >
void detect_solid_borders( const vil_image_view< PixType >& color_image,
                           const vil_image_view< PixType >& gray_image,
                           const border_detection_settings< PixType >& settings,
                           image_border& output );


/// An alternative border detection function specialized for detecting a specific
/// type of image border, one which can't be described by a rectangle and where all
/// pure black or white pixels touching the image boundaries are considered border
/// pixels.
template< typename PixType >
void detect_bw_nonrect_borders( const vil_image_view< PixType >& img,
                                image_border_mask& output,
                                const PixType tolerance = 1 );


/// Fill all pixels between some image border and the sides of an image with a
/// given fill value.
template< typename PixType >
void fill_border_pixels( vil_image_view< PixType >& img,
                         const image_border& border,
                         const PixType value );


} // end namespace vidtk


#endif // vidtk_video_border_detection_h_
