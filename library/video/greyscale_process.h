/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_greyscale_process_h_
#define vidtk_greyscale_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vil/vil_image_view.h>

namespace vidtk
{


template <class PixType>
class greyscale_process
  : public process
{
public:
  typedef greyscale_process self_type;

  greyscale_process( vcl_string const& name );

  ~greyscale_process();

  // -- process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_image, vil_image_view<PixType> const& );

  vil_image_view<PixType> const& image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, image );

  vil_image_view<PixType> const& copied_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, copied_image );


private:
  config_block config_;

  vil_image_view< PixType > const* in_img_;
  vil_image_view< PixType > out_img_;
  mutable vil_image_view< PixType > copied_out_img_;
};


} // end namespace vidtk


#endif // vidtk_greyscale_process_h_
