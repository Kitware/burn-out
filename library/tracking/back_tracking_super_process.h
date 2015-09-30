/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef back_tracking_super_process_h_
#define back_tracking_super_process_h_

#include <pipeline/super_process.h>

#include <tracking/multiple_features.h>
#include <tracking/image_object.h>
#include <tracking/track.h>
#include <tracking/frame_objects_buffer_process.h>

#include <utilities/config_block.h>
#include <utilities/buffer.h>
#include <utilities/timestamp.h>
#include <utilities/paired_buffer.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

// This class is created to back (reverse) track into the past.
// This process is supposed to take a set of initialized tracks at
// each frame and back-track them as far as possible within a pre-
// specified time limit (duration).

template< class PixType >
class back_tracking_super_process_impl;
class gui_process;

template< class PixType >
class back_tracking_super_process
  : public super_process
{
public:
  typedef back_tracking_super_process self_type;
  typedef paired_buffer< timestamp, vil_image_view<PixType> > ts_img_buff_type;
  typedef paired_buffer< timestamp, double > gsd_buff_type;

  back_tracking_super_process( vcl_string const& name );

  ~back_tracking_super_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual process::step_status step2();

  void set_mf_params( multiple_features const& );

  void set_gui( process_smart_pointer< gui_process > );

  // Input port(s)

  void set_image_buffer( buffer < vil_image_view<PixType> > const& );
  VIDTK_INPUT_PORT( set_image_buffer, buffer < vil_image_view<PixType> > const& );

  void set_ts_image_buffer( ts_img_buff_type const& buf );
  VIDTK_INPUT_PORT( set_ts_image_buffer, ts_img_buff_type const& );

  void set_gsd_buffer( gsd_buff_type const& buf );
  VIDTK_INPUT_PORT( set_gsd_buffer, gsd_buff_type const& );

  void set_timestamp_buffer( buffer< timestamp > const& );
  VIDTK_INPUT_PORT( set_timestamp_buffer, buffer< timestamp > const& );

  void set_mod_buffer( buffer< vcl_vector<image_object_sptr> > const& );
  VIDTK_INPUT_PORT( set_mod_buffer, buffer< vcl_vector<image_object_sptr> > const& );

  void set_img2wld_homog_buffer( buffer< vgl_h_matrix_2d<double> > const& );
  VIDTK_INPUT_PORT( set_img2wld_homog_buffer, buffer< vgl_h_matrix_2d<double> >  const& );

  void set_wld2img_homog_buffer( buffer< vgl_h_matrix_2d<double> > const& );
  VIDTK_INPUT_PORT( set_wld2img_homog_buffer, buffer< vgl_h_matrix_2d<double> >  const& );

  void in_tracks( vcl_vector< track_sptr > const& );
  VIDTK_INPUT_PORT( in_tracks, vcl_vector< track_sptr > const& );

  void set_fg_objs_buffer( frame_objs_type const& );
  VIDTK_INPUT_PORT( set_fg_objs_buffer, frame_objs_type );

  // Output port(s)

  /// Final set of tracks after performing back-tracking through the *duration*.
  vcl_vector< track_sptr > const& updated_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, updated_tracks );

  frame_objs_type const& updated_fg_objs_buffer() const;
  VIDTK_OUTPUT_PORT( frame_objs_type const&, updated_fg_objs_buffer );

private:
  back_tracking_super_process_impl<PixType> * impl_;
};

}// vidtk

#endif // back_tracking_super_process_h_
