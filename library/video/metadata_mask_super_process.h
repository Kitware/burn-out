/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_metadata_mask_super_process_h_
#define vidtk_metadata_mask_super_process_h_

#include <kwklt/klt_track.h>

#include <pipeline/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>

#include <tracking/shot_break_flags.h>

namespace vidtk
{

template< class PixType >
class metadata_mask_super_process_impl;
class shot_break_flags;

template< class PixType >
class metadata_mask_super_process
  : public super_process
{
public:
  typedef metadata_mask_super_process self_type;

  metadata_mask_super_process( vcl_string const& name );

  ~metadata_mask_super_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual process::step_status step2();

  void set_source_image(vil_image_view< vxl_byte > const&);
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< vxl_byte > const& );

  void set_source_timestamp( timestamp const& );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  vil_image_view< bool > get_mask ( ) const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, get_mask );


private:
  metadata_mask_super_process_impl<PixType> * impl_;
};

} // end namespace vidtk

#endif // vidtk_metadata_mask_super_process_h_
