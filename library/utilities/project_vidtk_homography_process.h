/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_project_vidtk_homography_process_h_
#define vidtk_project_vidtk_homography_process_h_

/// \file
/// Process to project a homography into a different coordinate system.
///
/// This process implements the following equation
///   H_out = P^-1 * H_in * P,
/// where P is a transformation from the desired output coordinate system to
/// that of input homography. Use-cases of this process can include uncropping
/// or cropping operation on the image-to-image homographies. For these cases
/// P remains unchanged for all the frames. An input port could be added if
/// a fixed P is not sufficient in the future.

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/homography.h>
#include <utilities/config_block.h>


namespace vidtk
{


//-----------------------projection-process------------------------------------//
template< class ST, class DT >
class project_vidtk_homography_process
  : public process
{
public:
  typedef project_vidtk_homography_process self_type;
  typedef homography_T< ST, DT > homog_t;

  project_vidtk_homography_process( std::string const & name );
  ~project_vidtk_homography_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  /// ------------------- Input port -------------------------------

  void set_source_homography( homog_t const& H );
  VIDTK_INPUT_PORT( set_source_homography, homog_t const& );

  /// ------------------- Output port -------------------------------

  /// \brief The transformed homography.
  homog_t homography() const;
  VIDTK_OUTPUT_PORT( homog_t, homography );

private:
  config_block config_;

  /// The source homography
  homog_t input_H_;

  /// The output homography
  homog_t output_H_;

  /// Should 'project' the homography in a different coordinate system.
  ///
  /// i.e. P^-1 * H * P
  bool project_;

  bool got_homog_;

  // Transformation matrices used with 'projection_matrix' parameter.
  homography::transform_t P_, P_inv_;
};

} // end namespace vidtk


#endif // vidtk_project_vidtk_homography_process_h_
