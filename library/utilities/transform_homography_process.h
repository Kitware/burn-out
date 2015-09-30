/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_transform_homography_process_h_
#define vidtk_transform_homography_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vgl/algo/vgl_h_matrix_2d.h>


namespace vidtk
{


/// \brief Apply additional transformations to a homography stream.
class transform_homography_process
  : public process
{
public:
  typedef transform_homography_process self_type;

  transform_homography_process( vcl_string const& name );

  ~transform_homography_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The source homography.
  void set_source_homography( vgl_h_matrix_2d<double> const& H );

  VIDTK_INPUT_PORT( set_source_homography, vgl_h_matrix_2d<double> const& );

  /// \brief The pre-multiply homography.
  void set_premult_homography( vgl_h_matrix_2d<double> const& H );

  VIDTK_INPUT_PORT( set_premult_homography, vgl_h_matrix_2d<double> const& );

  /// \brief The post-multiply homography.
  void set_postmult_homography( vgl_h_matrix_2d<double> const& H );

  VIDTK_INPUT_PORT( set_postmult_homography, vgl_h_matrix_2d<double> const& );


  /// \brief The transformed homography.
  vgl_h_matrix_2d<double> const& homography() const;

  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, homography );

  /// \brief The transformed homography.
  vgl_h_matrix_2d<double> const& inv_homography() const;

  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, inv_homography );

  vgl_h_matrix_2d<double> get_premult_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, get_premult_homography );

private:
  config_block config_;

  /// Should we pre-multiply by a given, fixed matrix?
  bool premult_;

  /// Should we pre-multiply by a given, fixed matrix (loaded from the file)?
  bool premult_file_;

  //  The file used to load the pre-multiply matrix from. Either use this file
  //  or provide a fixed matrix or none. 
  //  NOTE: Currently taking inverse of the (gnd2img0) homography read in 
  //  from the file. TODO: Use (img02gnd) homography files and remove the inverse here. 
  vcl_string premult_filename_;

  /// The matrix by which we should pre-multiply. 
  //  This matrix will be loaded either from the file *or* as a pre-defined 
  //  matrix. Cannot be both (for the sake of generality). 
  vnl_matrix_fixed<double,3,3> premult_M_;

  /// Should we post-multiply by a given, fixed matrix?
  bool postmult_;

  /// The matrix by which we should post-multiply.
  vnl_matrix_fixed<double,3,3> postmult_M_;

  /// The source homography
  vgl_h_matrix_2d<double> const* inp_H_;

  /// The output homography
  vgl_h_matrix_2d<double> out_H_;

  /// The output inverse homography
  vgl_h_matrix_2d<double> out_inv_H_;
};


} // end namespace vidtk


#endif // vidtk_transform_homography_process_h_
