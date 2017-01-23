/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sg_background_model_process_h_
#define vidtk_sg_background_model_process_h_

#include <object_detectors/fg_image_process.h>
#include <object_detectors/sg_gm_pixel_model.h>
#include <process_framework/pipeline_aid.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vbl/vbl_array_2d.h>
#include <memory>

namespace vidtk
{


template<class PixType>
class sg_background_model_process
  : public fg_image_process<PixType>
{
public:
  typedef sg_background_model_process self_type;

  sg_background_model_process( std::string const& name );

  virtual ~sg_background_model_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Specify the next image.
  ///
  /// This should be called prior to each call to step().  The image
  /// object must persist until step() is complete.
  void set_source_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// Read in the image to world homography for the current image
  void set_homography( vgl_h_matrix_2d<double> homog );
  VIDTK_INPUT_PORT( set_homography, vgl_h_matrix_2d<double> );

  virtual vil_image_view<bool> fg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );

  /// \brief A representative image of the background.
  virtual vil_image_view<PixType> bg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, bg_image );

  /// \brief A probability image for confidence scoring
  virtual vil_image_view<float> diff_image() const;
  VIDTK_OUTPUT_PORT(vil_image_view<float>, diff_image);

protected:
  void initialize_array( unsigned ni, unsigned nj, unsigned nplanes );
  void warp_array();
  void delete_array();

  config_block config_;

  std::auto_ptr<sg_gm_pixel_model_params> param_;

  vil_image_view<PixType> const* inp_frame_;

  vgl_h_matrix_2d<double> image_to_world_homog_;

  vgl_h_matrix_2d<double> ref_image_homog_;

  vgl_h_matrix_2d<double> padding_shift_;

  /// The per-pixel models.
  vbl_array_2d< sg_gm_pixel_model* > models_;

  /// The foreground mask for the current image.
  vil_image_view<bool> fgnd_mask_;
  /// The heat map for the current image.
  vil_image_view<float> prob_image_;
  /// Size of padding around reference image in the array
  unsigned array_padding_;
};


} // end namespace vidtk


#endif // vidtk_sg_background_model_process_h_
