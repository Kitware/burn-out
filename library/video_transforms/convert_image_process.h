/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_convert_image_process_h_
#define vidtk_convert_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

namespace vidtk
{

template <class FromPixType, class ToPixType>
class convert_image_process
  : public process
{
public:
  typedef convert_image_process self_type;

  convert_image_process( std::string const& name );
  virtual ~convert_image_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_image( vil_image_view<FromPixType> const& img );
  VIDTK_INPUT_PORT( set_image, vil_image_view<FromPixType> const& );

  /// \brief The cropped output image.
  vil_image_view<ToPixType> converted_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<ToPixType>, converted_image );

private:

  // Parameters
  config_block config_;

  // Input
  vil_image_view<FromPixType> input_image_;

  // Output
  vil_image_view<ToPixType> converted_image_;
};


} // end namespace vidtk


#endif // vidtk_convert_image_process_h_
