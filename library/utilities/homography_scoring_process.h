/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_scoring_process_h_
#define vidtk_homography_scoring_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

class homography_scoring_process
  : public process
{
public:
  typedef homography_scoring_process self_type;

  homography_scoring_process( vcl_string const& name );

  ~homography_scoring_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_good_homography( vgl_h_matrix_2d<double> const& H );
  VIDTK_INPUT_PORT( set_good_homography, vgl_h_matrix_2d<double> const& );

  void set_test_homography( vgl_h_matrix_2d<double> const& H );
  VIDTK_INPUT_PORT( set_test_homography, vgl_h_matrix_2d<double> const& );

  bool is_good_homography() const;
  VIDTK_OUTPUT_PORT( bool, is_good_homography );

private:
  bool disabled_;

  double max_dist_offset_;
  double area_percent_factor_;
  int quadrant_;
  int height_;
  int width_;

  config_block config_;

  vgl_h_matrix_2d<double> good_homography_;
  vgl_h_matrix_2d<double> test_homography_;

  bool is_good_homog_;
};


} // end namespace vidtk


#endif // vidtk_homography_scoring_process_h_
