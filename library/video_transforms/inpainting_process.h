/*ckwg +5
 * Copyright 2011-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_inpainting_process_h_
#define vidtk_inpainting_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <video_transforms/threaded_image_transform.h>
#include <video_transforms/moving_mosaic_generator.h>

#include <utilities/homography.h>

#include <tracking_data/image_border.h>
#include <tracking_data/shot_break_flags.h>

namespace vidtk
{

/// \brief Inpaints some RGB or grayscale input given some mask.
template< typename PixType >
class inpainting_process
  : public process
{
public:
  typedef inpainting_process self_type;
  typedef vil_image_view< PixType > image_t;
  typedef vil_image_view< bool > mask_t;
  typedef image_border border_t;
  typedef image_to_image_homography homography_t;
  typedef threaded_image_transform< image_t, mask_t, image_t > thread_sys_t;

  inpainting_process( std::string const& name );
  virtual ~inpainting_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  /// \brief Set input image.
  void set_source_image( image_t const& img );
  VIDTK_INPUT_PORT( set_source_image, image_t const& );

  /// \brief Set input timestamp.
  void set_source_timestamp( vidtk::timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, vidtk::timestamp const& );

  /// \brief Set input mask.
  void set_mask( mask_t const& img );
  VIDTK_INPUT_PORT( set_mask, mask_t const& );

  /// \brief Set input motion mask.
  void set_motion_mask( mask_t const& img );
  VIDTK_INPUT_PORT( set_motion_mask, mask_t const& );

  /// \brief Set input border.
  void set_border( border_t const& border );
  VIDTK_INPUT_PORT( set_border, border_t const& );

  /// \brief Set input homography.
  void set_homography( homography_t const& h );
  VIDTK_INPUT_PORT( set_homography, homography_t const& );

  /// \brief Set input shot break flags.
  void set_shot_break_flags( shot_break_flags const& sbf );
  VIDTK_INPUT_PORT( set_shot_break_flags, shot_break_flags const& );

  /// \brief Set input obstruction type.
  void set_type( std::string const& s );
  VIDTK_INPUT_PORT( set_type, std::string const& );

  /// \brief The inpainted output image.
  image_t inpainted_image() const;
  VIDTK_OUTPUT_PORT( image_t, inpainted_image );

private:

  // Parameters
  config_block config_;
  bool disabled_;
  enum{ NEAREST, TELEA, NAVIER, NONE } algorithm_;
  enum{ FILL_SOLID, INPAINT } border_method_;
  double radius_;
  double stab_image_factor_;
  double unstab_image_factor_;
  int border_dilation_;
  double vert_center_ignore_;
  double hori_center_ignore_;
  bool use_mosaic_;
  unsigned max_buffer_size_;
  unsigned min_size_for_hist_;
  double flush_trigger_;
  double illum_trigger_;

  // Internal algorithm helpers
  boost::scoped_ptr< thread_sys_t > threads_;

  void inpaint( const image_t& input,
                const mask_t& mask,
                image_t output ) const;

  void compute_warped_image();

  // Inputs
  image_t input_image_;
  vidtk::timestamp input_ts_;
  mask_t input_mask_;
  mask_t input_motion_mask_;
  border_t input_border_;
  homography_t input_homography_;
  shot_break_flags input_sbf_;
  std::string input_type_;

  // Internal buffers
  image_t last_input_image_;
  image_t last_output_image_;
  mask_t last_unpainted_mask_;
  homography_t last_homography_;
  moving_mosaic_generator< PixType > mmg_;
  mask_t adj_motion_mask_;

  image_t warped_image_;
  mask_t warped_mask_;

  struct buffered_entry_t
  {
    image_t image;
    mask_t mask;
    homography_t homog;
    border_t border;
    vidtk::timestamp ts;

    buffered_entry_t(
      const image_t& i, const mask_t& m,
      const homography_t& h, const border_t& b,
      const vidtk::timestamp& t )
    : image( i ), mask( m ), homog( h ), border( b ), ts( t )
    {}
  };

  bool buffering_enabled_;
  bool is_buffering_;
  double reference_overlap_;
  std::vector< buffered_entry_t > buffer_;
  void flush_buffer( bool reset_mosaic = false );

  // Outputs
  image_t inpainted_image_;
};


} // end namespace vidtk


#endif // vidtk_inpainting_process_h_
