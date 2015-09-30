/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef gmm_background_model_process_h_
#define gmm_background_model_process_h_

#include <tracking/fg_image_process.h>
#include <process_framework/pipeline_aid.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vbl/vbl_array_2d.h>
#include <vcl_memory.h>

#include <bbgm/bbgm_image_sptr.h>
#include <bbgm/bbgm_image_of.h>
#include <bbgm/bbgm_image_of.txx>
#include <bbgm/bbgm_update.h>
#include <bbgm/bbgm_detect.h>

#include <vpdl/vpdt/vpdt_gaussian.h>
#include <vpdl/vpdt/vpdt_mixture_of.h>
#include <vpdl/vpdt/vpdt_update_mog.h>
#include <vpdl/vpdt/vpdt_distribution_accessors.h>
#include <vpdl/vpdt/vpdt_gaussian_detector.h>
#include <vpdl/vpdt/vpdt_mixture_detector.h>

#include <vil/algo/vil_structuring_element.h>

namespace vidtk
{

struct gmm_background_model_params
{
  gmm_background_model_params()
  {
  }

  float init_variance_;
  float gaussian_match_thresh_;
  float mininum_stdev_;
  int max_components_;
  int window_size_;
  float background_distance_;
  float weight_;
  float match_radius_;
  unsigned int num_pix_comp_;
};

template<class PixType>
class gmm_background_model_process
  : public fg_image_process<PixType>
{
public:
  typedef gmm_background_model_process self_type;
  typedef vpdt_gaussian< vnl_vector_fixed<float,3>, float > gauss_type;
  typedef gauss_type::field_type field_type;
  typedef vpdt_mixture_of<gauss_type> mix_gauss_type;
  typedef vpdt_mog_lm_updater<mix_gauss_type> updater_type;
  typedef vpdt_gaussian_mthresh_detector<gauss_type> gdetector_type;
  typedef vpdt_mixture_top_weight_detector<mix_gauss_type, gdetector_type> detector_type;

  gmm_background_model_process( vcl_string const& name );

  ~gmm_background_model_process();

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

  virtual vil_image_view<bool> const& fg_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<bool> const&, fg_image );

  /// \brief A representative image of the background.
  virtual vil_image_view<PixType> bg_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, bg_image );

protected:
  void initialize_array( unsigned ni, unsigned nj, unsigned nplanes );
  void warp_array();
  void delete_array();

  config_block config_;

  vcl_auto_ptr<gmm_background_model_params> param_;

  vil_image_view<PixType> const* inp_frame_;

  vgl_h_matrix_2d<double> image_to_world_homog_;

  vgl_h_matrix_2d<double> ref_image_homog_;

  vgl_h_matrix_2d<double> padding_shift_;

  /// The per-pixel models.
  //vbl_array_2d< sg_gm_pixel_model* > models_;
  bbgm_image_of<mix_gauss_type> bg_model_;
  updater_type * updater_;
  detector_type * detector_;
  vil_structuring_element se_;

  /// The foreground mask for the current image.
  vil_image_view<bool> fgnd_mask_;

  /// Size of padding around reference image in the array
  unsigned array_padding_;
};


} // end namespace vidtk


#endif // gmm_background_model_process_h_
