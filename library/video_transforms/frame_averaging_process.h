/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_averaging_process_h_
#define vidtk_frame_averaging_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <video_transforms/frame_averaging.h>
#include <vil/vil_image_view.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

/// A process for computing averages across multiple frames.
///
/// Can compute windowed averages, cumulative averages, exponential averages
/// and can optionally calculate an estimated instantaneous variance for each pixel.
///
/// The BufferType specifies the precision of the internal buffer in cases
/// where the buffer is necessary (storing exponential/cumalative averages).
///
/// If enabled, this process can also produce an output variance image
/// which measures an estimate of per-pixel variance for the duration
/// of the moving average.
template <typename PixType, typename BufferType = double>
class frame_averaging_process
  : public process
{

public:

  typedef frame_averaging_process self_type;
  typedef boost::scoped_ptr< online_frame_averager<PixType> > frame_averager_sptr;

  frame_averaging_process( std::string const& name );
  virtual ~frame_averaging_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_reset_flag( bool flag );
  VIDTK_INPUT_PORT( set_reset_flag, bool );

  vil_image_view<PixType> averaged_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, averaged_image );

  vil_image_view<double> variance_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<double>, variance_image );

private:

  // Inputs
  /// @todo Remove pointers to input data
  vil_image_view<PixType> const* src_image_;
  bool reset_flag_;

  // Possible Outputs
  vil_image_view<PixType> output_avg_image_;
  vil_image_view<double> output_var_image_;

  // Internal parameters/settings
  config_block config_;
  bool estimate_variance_;

  // The actual frame averager
  frame_averager_sptr averager_;
};


} // end namespace vidtk


#endif // vidtk_frame_averaging_process_h_
