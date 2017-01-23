/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_convert_color_space_process_h_
#define vidtk_convert_color_space_process_h_


#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include "convert_color_space.h"

namespace vidtk
{

template <typename PixType>
class convert_color_space_process
  : public process
{

public:

  typedef convert_color_space_process self_type;

  convert_color_space_process( std::string const& name );
  virtual ~convert_color_space_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// \brief The output image.
  vil_image_view<PixType> converted_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, converted_image );

private:

  // Inputs
  /// @todo Remove pointers to input data
  vil_image_view<PixType> const* src_image_;

  // Possible Outputs
  vil_image_view<PixType> dst_image_;

  // Parameters
  config_block config_;
  color_space src_space_;
  color_space dst_space_;
};


} // end namespace vidtk


#endif // vidtk_convert_color_space_process_h_
