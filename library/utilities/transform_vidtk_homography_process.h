/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_transform_vidtk_homography_process_h_
#define vidtk_transform_vidtk_homography_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/homography.h>


namespace vidtk
{


/// \brief Apply additional transformations to a homography stream.
template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
class transform_vidtk_homography_process
  : public process
{
public:
  typedef transform_vidtk_homography_process self_type;

  typedef homography_T< InSrcType,   InDestType  > src_homog_t;
  typedef homography_T< InDestType,  OutDestType > pre_homog_t;
  typedef homography_T< OutSrcType,  InSrcType   > post_homog_t;
  typedef homography_T< OutSrcType,  OutDestType > out_homog_t ;
  typedef homography_T< OutDestType, OutSrcType  > inv_out_homog_t;

  transform_vidtk_homography_process( vcl_string const& name );

  ~transform_vidtk_homography_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// ------------------- Input ports -------------------------------
  
  /// \brief The source homography.
  void set_source_homography( src_homog_t const& H );
  VIDTK_INPUT_PORT( set_source_homography, src_homog_t const& );

  /// \brief The pre-multiply homography.
  void set_premult_homography( pre_homog_t const& H );
  VIDTK_INPUT_PORT( set_premult_homography, pre_homog_t const& );

  /// \brief The post-multiply homography.
  void set_postmult_homography( post_homog_t const& H );
  VIDTK_INPUT_PORT( set_postmult_homography, post_homog_t const& );

  /// ------------------- Output ports -------------------------------

  /// \brief The transformed homography.
  out_homog_t const& homography() const;
  VIDTK_OUTPUT_PORT( out_homog_t const&, homography );

  /// \brief The transformed homography.
  inv_out_homog_t const& inv_homography() const;
  VIDTK_OUTPUT_PORT( inv_out_homog_t const&, inv_homography );

  /// \brief The transformed homography.
  typename out_homog_t::transform_t const& bare_homography() const;
  VIDTK_OUTPUT_PORT( typename out_homog_t::transform_t const&, bare_homography );

  /// \brief The transformed homography.
  typename inv_out_homog_t::transform_t const& inv_bare_homography() const;
  VIDTK_OUTPUT_PORT( typename inv_out_homog_t::transform_t const&, inv_bare_homography );

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
  pre_homog_t premult_M_;

  /// Should we post-multiply by a given, fixed matrix?
  bool postmult_;

  /// The matrix by which we should post-multiply.
  post_homog_t postmult_M_;

  /// The source homography
  src_homog_t const* inp_H_;

  /// The output homography
  out_homog_t out_H_;

  /// The output inverse homography
  inv_out_homog_t out_inv_H_;
};


} // end namespace vidtk


#endif // vidtk_transform_vidtk_homography_process_h_
