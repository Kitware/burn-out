/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_reader_process_h_
#define vidtk_homography_reader_process_h_

#include <vector>
#include <fstream>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/homography.h>
#include <utilities/timestamp.h>

namespace vidtk
{


/// Reads previously computed homographies from a file.
///
/// The process will output the read homographies frame by frame, as
/// if the homographies were computed online.
class homography_reader_process
  : public process
{
public:
  typedef homography_reader_process self_type;
  typedef process_smart_pointer<self_type> pointer;

  homography_reader_process( std::string const& name );

  ~homography_reader_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Input port(s)

  /// Timestamp port used to enforce the last frame when reading only the
  /// first part of the homography file.
  void set_timestamp( timestamp const & ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const & );

  /// Output port(s)

  vgl_h_matrix_2d<double> image_to_world_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, image_to_world_homography );

  vgl_h_matrix_2d<double> world_to_image_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, world_to_image_homography );

  image_to_plane_homography image_to_world_vidtk_homography_plane() const;
  VIDTK_OUTPUT_PORT( image_to_plane_homography, image_to_world_vidtk_homography_plane );

  plane_to_image_homography world_to_image_vidtk_homography_plane() const;
  VIDTK_OUTPUT_PORT( plane_to_image_homography, world_to_image_vidtk_homography_plane );

  image_to_image_homography image_to_world_vidtk_homography_image() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, image_to_world_vidtk_homography_image );

  image_to_image_homography world_to_image_vidtk_homography_image() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, world_to_image_vidtk_homography_image );


private:

  config_block config_;

  std::string input_filename_;

  std::ifstream homog_str_;

  vgl_h_matrix_2d<double> img_to_world_H_;
  vgl_h_matrix_2d<double> world_to_img_H_;

  timestamp reference_;
  timestamp time_;

  unsigned version_;
};


} // end namespace vidtk


#endif // vidtk_homography_reader_process_h_
