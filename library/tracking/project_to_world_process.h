/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_project_to_world_process_h_
#define vidtk_project_to_world_process_h_

#include <vcl_vector.h>

#include <vnl/vnl_double_3x4.h>
#include <vnl/vnl_double_3x3.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/image_object.h>


namespace vidtk
{


class project_to_world_process
  : public process
{
public:
  typedef project_to_world_process self_type;

  project_to_world_process( vcl_string const& name );

  ~project_to_world_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  vnl_double_3x3 const& image_to_world_homography();

  /// \brief Set the objects to be projected.
  ///
  /// Note that the objects themselves will *not* be copied, and thus
  /// the areas will be modified "in place".
  void set_source_objects( vcl_vector< image_object_sptr > const& objs );

  VIDTK_INPUT_PORT( set_source_objects, vcl_vector< image_object_sptr > const& );

  /// \brief Set the image to world homography for the current frame.
  ///
  /// This homography will be combined with the P-matrix supplied in
  /// the configuration.  So, if the P-matrix is supplied, then this
  /// "image to world" homography can be a image to reference
  /// homography, while the P-matrix maps from the refererence to the
  /// world.
  void set_image_to_world_homography( vgl_h_matrix_2d<double> const& H );

  VIDTK_INPUT_PORT( set_image_to_world_homography, vgl_h_matrix_2d<double> const& );

  vcl_vector< image_object_sptr > const& objects() const;

  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

protected:
  void back_project( vnl_vector_fixed<double,2> const& img_loc,
                     vnl_vector_fixed<double,3>& world_loc ) const;
  void update_area( image_object& obj );

  config_block config_;

  vcl_string pmatrix_filename_;
  bool has_pmatrix_filename_;

  /// The camera matrix.
  vnl_double_3x4 P_matrix_;

  /// Update the area of the object to be some measure of area in
  /// world units.
  bool update_area_;

  /// Transform the image coordinates matrix.
  int image_shift_[2];


  /// Projection from the ground plane to the image plane as given by
  /// the camera P matrix.
  vnl_double_3x3 forw_cam_proj_;

  /// Backprojection from the image plane to the ground plane as given
  /// by the camera P matrix.
  vnl_double_3x3 back_cam_proj_;

  /// Backprojection from the image plane to the ground plane after
  /// applying both the current image->world homography and the camera
  /// P matrix.
  vnl_double_3x3 back_proj_;

  vcl_vector< image_object_sptr > objs_;
};


} // end namespace vidtk


#endif // vidtk_project_to_world_process_h_
