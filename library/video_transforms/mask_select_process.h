/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_mask_select_process_h_
#define vidtk_mask_select_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

namespace vidtk
{


/// \brief Selectively pass through a mask image
class mask_select_process
  : public process
{
public:
  typedef mask_select_process self_type;

  mask_select_process( std::string const& name );

  virtual ~mask_select_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input data image.
  void set_data_image( vil_image_view<vxl_byte> const& img );
  VIDTK_INPUT_PORT( set_data_image, vil_image_view<vxl_byte> const& );

  /// \brief The input mask.
  void set_mask( vil_image_view<bool> const& mask );
  VIDTK_INPUT_PORT( set_mask, vil_image_view<bool> const& );

  /// \brief The output mask.
  vil_image_view<bool> mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, mask );


private:

  // Parameters
  config_block config_;

  double threshold_;

  vil_image_view<vxl_byte> data_image_;
  vil_image_view<bool> in_mask_;
  vil_image_view<bool> out_mask_;

};


} // end namespace vidtk


#endif // vidtk_mask_select_process_h_
