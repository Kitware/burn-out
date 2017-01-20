/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_moving_mosaic_generator_h_
#define vidtk_moving_mosaic_generator_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <utilities/external_settings.h>
#include <utilities/homography.h>

namespace vidtk
{

#define settings_macro( add_param, add_array, add_enumr ) \
  add_array( \
    mosaic_resolution, \
    unsigned, 3, \
    "3000 3000 3", \
    "Maximum resolution per mosaic page." ); \
  add_param( \
    mosaic_output_dir, \
    std::string, \
    "", \
    "Output directory to dump mosaics to." ); \
  add_param( \
    mosaic_homog_file, \
    std::string, \
    "", \
    "Output filename to dump image to mosaic homographies to." ); \
  add_enumr( \
    mosaic_method, \
    (USE_LATEST) (EXP_AVERAGE) (CUM_AVERAGE), \
    USE_LATEST, \
    "Method for adding new images to the mosaic." ); \
  add_enumr( \
    inpainting_method, \
    (NEAREST) (TELEA) (NAVIER), \
    TELEA, \
    "Method for performing pixel inpainting." ); \
  add_param( \
    inpaint_output_mosaics, \
    bool, \
    false, \
    "Inpaint missing pixels in all output mosaics." );

init_external_settings3( moving_mosaic_settings, settings_macro )

#undef settings_macro

/// \brief Generates image mosaics for a moving image sequence.
template< typename PixType=vxl_byte, typename MosaicType=PixType >
class moving_mosaic_generator
{
public:

  typedef vil_image_view< PixType > image_t;
  typedef vil_image_view< bool > mask_t;
  typedef vil_image_view< MosaicType > mosaic_t;
  typedef image_to_image_homography homography_t;
  typedef vgl_h_matrix_2d< double > transform_t;

  moving_mosaic_generator();
  virtual ~moving_mosaic_generator();

  bool configure( const moving_mosaic_settings& s );

  double add_image( const image_t& image,
                    const mask_t& mask,
                    const homography_t& homog );

  bool get_pixels( const mask_t& mask,
                   const homography_t& homog,
                   image_t& output_image,
                   mask_t& output_mask );

  void inpaint_mosaic();
  void reset_mosaic();

private:

  // Parameters
  moving_mosaic_settings settings;

  // Stored variables
  mosaic_t mosaic;
  transform_t ref_to_mosaic;
  unsigned mosaic_id;
  bool is_first;
  bool active_scene;
  vidtk::timestamp ref_frame;
  std::string homog_fn;

  // Helper functions
  mosaic_t new_mosaic();

  void increment_mosaic_id();
  std::string get_mosaic_fn( bool with_path=true );
  void inpaint_mosaic( mosaic_t& image );
  void output_mosaic( mosaic_t& image );
  void reset_mosaic( mosaic_t& image );
};


} // end namespace vidtk


#endif // vidtk_moving_mosaic_generator_h_
