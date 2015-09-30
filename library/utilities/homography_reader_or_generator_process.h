/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_reader_or_generator_process_h_
#define vidtk_homography_reader_or_generator_process_h_

#include <vcl_vector.h>
#include <vcl_fstream.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/timestamp.h>


namespace vidtk
{


/// This process with either generate consitant homographies or read for a file.
///
/// The process will output the homographies frame by frame, as
/// if the homographies were computed online.
class homography_reader_or_generator_process
  : public process
{
public:
  typedef homography_reader_or_generator_process self_type;
  typedef process_smart_pointer<self_type> pointer;

  homography_reader_or_generator_process( vcl_string const& name );

  ~homography_reader_or_generator_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  vgl_h_matrix_2d<double> const& image_to_world_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, image_to_world_homography );

  vgl_h_matrix_2d<double> const& world_to_image_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, world_to_image_homography );

  void signal_new_image(timestamp);
  VIDTK_OPTIONAL_INPUT_PORT( signal_new_image, timestamp );


private:

  config_block config_;

  vcl_string input_filename_;

  vcl_ifstream homog_str_;

  bool generate_;
  bool image_needs_homog_;
  unsigned int number_to_generate_;
  unsigned int number_generated_;
  vgl_h_matrix_2d<double> img_to_world_H_;
  vgl_h_matrix_2d<double> world_to_img_H_;
};


} // end namespace vidtk

#endif // vidtk_homography_reader_or_generator_process_h_
