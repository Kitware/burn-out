/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_amhi_create_image_process_h_
#define vidtk_amhi_create_image_process_h_

#include <vcl_vector.h>
#include <vil/vil_image_view.h>
//#include <vgl/vgl_point_2d.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/track.h>
#include <tracking/amhi.h>
#include <utilities/timestamp.h>

namespace vidtk
{
/// \brief 
///
/// 
template <class PixType>
class amhi_create_image_process
  : public process
{
public:
  typedef amhi_create_image_process self_type;
  
  amhi_create_image_process( vcl_string const& name);

  ~amhi_create_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// The timestamp for the current frame.
  void set_timestamp( timestamp const& ts );

  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  void set_enabled( bool const & );

  VIDTK_INPUT_PORT( set_enabled, bool const & );

  //Used only to get the right size (ni x nj)
  void set_source_image( vil_image_view<PixType> const& );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_source_tracks( vcl_vector<track_sptr> const& );

  VIDTK_INPUT_PORT( set_source_tracks, vcl_vector<track_sptr> const& );

  vil_image_view<vxl_byte> const& amhi_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, amhi_image );
  
protected:
  config_block config_;

  // Input data
  vcl_vector< track_sptr > const* src_trks_;
  vil_image_view<PixType> const* src_img_;
  bool enabled_;
  timestamp const* cur_ts_;

  // Output data
  vil_image_view<vxl_byte> amhi_image_;
};

} // end namespace vidtk


#endif // vidtk_amhi_create_image_process_h_
