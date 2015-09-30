/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_amhi_initializer_process_h_
#define vidtk_amhi_initializer_process_h_

#include <vcl_vector.h>
#include <vil/vil_image_view.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/track.h>
#include <tracking/amhi.h>
#include <utilities/buffer.h>


namespace vidtk
{
/// \brief
///
///
template < class PixType >
class amhi_initializer_process
  : public process
{
public:
  typedef amhi_initializer_process self_type;

  amhi_initializer_process( vcl_string const& name );

  ~amhi_initializer_process();

  virtual config_block params()
    const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  void set_source_tracks( vcl_vector<track_sptr> const& );

  VIDTK_INPUT_PORT( set_source_tracks, vcl_vector<track_sptr> const& );

  void set_source_image_buffer( buffer< vil_image_view<PixType> > const& image_buf );

  VIDTK_INPUT_PORT( set_source_image_buffer, buffer< vil_image_view<PixType> > const& );

  vcl_vector<track_sptr> const& out_tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector<track_sptr> const&,  out_tracks );

protected:
  config_block config_;

  //Config parameters
  double amhi_alpha_;
  double padding_factor_;
  double w_bbox_online_;
  double min_pixel_weight_;
  double max_frac_valley_width_;
  double min_frac_valley_depth_;
  bool use_weights_;

  vbl_smart_ptr< amhi<PixType> > amhi_;

  // Input data
  vcl_vector< track_sptr > const* src_trks_;
  buffer< vil_image_view<PixType> > const* image_buffer_;
  bool enabled_;

  // Output data
  vcl_vector< track_sptr > out_trks_;
};


} // end namespace vidtk


#endif // vidtk_amhi_initializer_process_h_
