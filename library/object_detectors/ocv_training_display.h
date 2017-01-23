/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_training_display_h_
#define vidtk_training_display_h_

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <string>
#include <vector>
#include <utility>

namespace vidtk
{

/// Show the designated region to the user, and await an inputted string.
std::string get_designation_from_user( const vil_image_view< vxl_byte >& image,
                                       const vgl_box_2d< unsigned >& region,
                                       const std::string& window_name = "TrainingDisplay",
                                       const vil_rgb<vxl_byte>& color = vil_rgb<vxl_byte>(255,0,0) );

/// Show all of the specified regions to the user. Block for ms_delay milliseconds.
void show_regions_to_user( const vil_image_view< vxl_byte >& image,
                           const std::vector< vgl_box_2d< unsigned > >& regions,
                           const std::string& window_name = "DetectionDisplay",
                           const unsigned ms_delay = 30 );

/// Show all of the specified regions to the user. Block for ms_delay milliseconds.
void show_regions_to_user( const vil_image_view< vxl_byte >& image,
                           const std::vector< std::pair< vgl_box_2d< unsigned >, double > >& regions,
                           const std::string& window_name = "DetectionDisplay",
                           const unsigned ms_delay = 30 );

/// Show all of the specified regions to the user. Block for ms_delay milliseconds.
void show_training_regions( const vil_image_view< vxl_byte >& image,
                            const std::vector< vgl_box_2d< unsigned > >& pos_regions,
                            const std::vector< vgl_box_2d< unsigned > >& neg_regions,
                            const std::vector< vgl_point_2d< int > >& pos_points,
                            const std::vector< vgl_point_2d< int > >& neg_points,
                            const std::string& window_name = "TrainingSamplesDisplay",
                            const unsigned ms_delay = 30 );


} // end namespace vidtk

#endif // vidtk_training_display_h_
