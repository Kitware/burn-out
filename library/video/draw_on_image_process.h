/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_draw_on_image_process_h_
#define vidtk_draw_on_image_process_h_

/// \file Creates burn-in of information on an image.
///
/// This process over-writes the incoming image inplace. If a deep copy is 
/// required, then an explict deep copy process should be added before this
/// image.

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

template< class PixType >
class draw_on_image_process
 : public process
{
public:
  typedef draw_on_image_process self_type;

  draw_on_image_process( vcl_string const& name );

  ~draw_on_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_world_units_per_pixel( double gsd );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  vil_image_view<PixType> const& image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, image );

private:
  config_block config_;

  bool disabled_;

  double gsd_; 

  vil_image_view<PixType> const* in_img_;

  vil_image_view<PixType> out_img_;
};

} // end namespace vidtk

#endif // vidtk_draw_on_image_process_h_
