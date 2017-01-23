/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_integral_image_process_h_
#define vidtk_integral_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

template <typename InputPixType>
class vil_image_view;

namespace vidtk
{

/// A process for computing generalized integral images
///
template <typename InputPixType, typename OutputPixType = int>
class integral_image_process
  : public process
{

public:

  typedef integral_image_process self_type;
  typedef vil_image_view<InputPixType> input_type;
  typedef vil_image_view<OutputPixType> output_type;

  integral_image_process( std::string const& name );
  virtual ~integral_image_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view<InputPixType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<InputPixType> const& );

  vil_image_view<OutputPixType> integral_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<OutputPixType>, integral_image );

private:

  // Inputs
  /// @todo Remove pointers to input data
  vil_image_view<InputPixType> const* src_image_;

  // Outputs
  vil_image_view<OutputPixType> output_ii_;

  // Internal parameters/settings
  config_block config_;
  bool disabled_;
};


} // end namespace vidtk


#endif // vidtk_integral_image_process_h_
