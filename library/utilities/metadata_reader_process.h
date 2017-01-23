/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef metadata_reader_process_h_
#define metadata_reader_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>

#include <fstream>

namespace vidtk
{

/// Reads previously computed metadata in a CSV file.
///
/// This process will output homographies, timestamps, etc.
/// on frame by frame basis. The CSV file is currently being
/// being produced by MAASProxy2Tool.
///
/// First line of the input file will contain comment explaining
/// contents of each column.

class metadata_reader_process
  : public process
{
public:
  typedef metadata_reader_process self_type;

  metadata_reader_process( std::string const& name );

  ~metadata_reader_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  // Input ports

  // Output ports

  vgl_h_matrix_2d< double > image_to_world_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d< double >,
                     image_to_world_homography );

  vgl_h_matrix_2d< double > world_to_image_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d< double >,
                     world_to_image_homography );

  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp,
                     timestamp );

protected:
  config_block config_;

  std::string input_filename_;

  std::ifstream input_file_;

  vgl_h_matrix_2d<double> H_img_to_wld_;

  vgl_h_matrix_2d<double> H_wld_to_img_;

  vidtk::timestamp ts_;
};

} // end namespace vidtk

#endif // metadata_reader_process_h_
