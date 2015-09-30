/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_eo_ir_detector_h_
#define vidtk_eo_ir_detector_h_

#include <vil/vil_image_view.h>

#include <utilities/config_block.h>

namespace vidtk
{

template < class PixType >
class eo_ir_detector
{
public:
  eo_ir_detector( PixType rg_diff = 8,
                  PixType rb_diff = 8,
                  PixType gb_diff = 8,
                  float eo_ir_sample_size = 0.10,
                  float eo_multiplier = 4.0 );
  bool is_ir( vil_image_view< PixType > const & img ) const;
  bool is_eo( vil_image_view< PixType > const & img ) const;

  config_block params() const { return config_; }
  bool set_params( config_block const& cfg );


protected:
  PixType rg_diff_;
  PixType rb_diff_;
  PixType gb_diff_;
  float eo_ir_sample_size_;
  float eo_multiplier_;

  config_block config_;

};

}

#endif
