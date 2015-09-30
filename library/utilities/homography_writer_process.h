/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_writer_process_h_
#define vidtk_homography_writer_process_h_

#include <vcl_vector.h>
#include <vcl_fstream.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/track.h>

#include <vgl/vgl_point_2d.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_3x3.h>

#include <utilities/homography.h>

namespace vidtk
{

class homography_writer_process
  : public process
{
public:
  typedef homography_writer_process self_type;

  homography_writer_process( vcl_string const& name );

  ~homography_writer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_output_file(vcl_string file);

  void set_source_homography( vgl_h_matrix_2d<double> const& H );

  VIDTK_INPUT_PORT( set_source_homography, vgl_h_matrix_2d<double> const& );

  void set_source_vidtk_homography( image_to_image_homography const& H );
  VIDTK_INPUT_PORT( set_source_vidtk_homography, image_to_image_homography const& );

  void set_source_timestamp( vidtk::timestamp const& ts );

  VIDTK_INPUT_PORT( set_source_timestamp, vidtk::timestamp const& );

private:
  bool disabled_;

  vcl_ofstream filestrm;

  vcl_string filename;

  config_block config_;

  vnl_double_3x3 Homography;

  unsigned FrameNumber;

  double TimeStamp;

  bool append_file_;
    
};


} // end namespace vidtk


#endif // vidtk_homography_writer_process_h_

