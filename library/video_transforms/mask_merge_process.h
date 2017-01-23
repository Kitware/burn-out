/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_mask_merge_process_h_
#define vidtk_mask_merge_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/image_border.h>

#include <vil/vil_image_view.h>
#include <string>

namespace vidtk
{

class mask_merge_process
  : public process
{
public:
  typedef mask_merge_process self_type;

  mask_merge_process( std::string const& name );
  virtual ~mask_merge_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_mask_a( vil_image_view<bool> const& img );
  VIDTK_INPUT_PORT( set_mask_a, vil_image_view<bool> const& );

  void set_mask_b( vil_image_view<bool> const& img );
  VIDTK_INPUT_PORT( set_mask_b, vil_image_view<bool> const& );

  void set_mask_c( vil_image_view<bool> const& img );
  VIDTK_INPUT_PORT( set_mask_c, vil_image_view<bool> const& );

  void set_border( image_border const& border );
  VIDTK_INPUT_PORT( set_border, image_border const& );

  vil_image_view<bool> mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, mask );

  vil_image_view<bool> borderless_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, borderless_mask );

private:
  config_block config_;

  vil_image_view<bool> a_img_;
  vil_image_view<bool> b_img_;
  vil_image_view<bool> c_img_;

  image_border border_;

  vil_image_view<bool> out_img_;
  vil_image_view<bool> borderless_out_img_;
};


} // end namespace vidtk

#endif // vidtk_mask_merge_process_h_
