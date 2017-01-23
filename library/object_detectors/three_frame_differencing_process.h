/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_three_frame_differencing_process_h_
#define vidtk_three_frame_differencing_process_h_

#include <vector>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <object_detectors/fg_image_process.h>
#include <object_detectors/three_frame_differencing.h>

#include <utilities/buffer.h>
#include <utilities/config_block.h>

#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Find moving object by pixelwise comparison of registered images.
/// This process will allocate new memory for each output image on
/// every frame.
template < typename PixType, typename FloatType >
class three_frame_differencing_process
  : public process
{
public:
  typedef three_frame_differencing_process self_type;

  three_frame_differencing_process( std::string const& name );
  virtual ~three_frame_differencing_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  void set_first_frame( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_first_frame, vil_image_view<PixType> const& );

  void set_second_frame( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_second_frame, vil_image_view<PixType> const& );

  void set_third_frame( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_third_frame, vil_image_view<PixType> const& );

  void set_difference_flag( bool flag );
  VIDTK_INPUT_PORT( set_difference_flag, bool );

  virtual vil_image_view<bool> fg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );

  virtual vil_image_view<FloatType> diff_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<FloatType>, diff_image );

protected:

  config_block config_;

  /// Current input images
  vil_image_view<PixType> stab_frame_1_;
  vil_image_view<PixType> stab_frame_2_;
  vil_image_view<PixType> stab_frame_3_;

  /// Input parameters
  bool difference_flag_;

  /// Current output images
  vil_image_view<FloatType> diff_image_;
  vil_image_view<bool> fg_image_;

  /// Internal algorithm
  three_frame_differencing<PixType,FloatType> algorithm_;

};

} // namespace vidtk

#endif
