/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _TRACK_DEMULTIPLEXER_PROCESS_H_
#define _TRACK_DEMULTIPLEXER_PROCESS_H_


#include "track_demultiplexer.h"

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <boost/scoped_ptr.hpp>

#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <utilities/config_block.h>

#include <tracking_data/shot_break_flags.h>

#include <vil/vil_image_view.h>
#include <vil/vil_image_resource.h>

// Define pass thru ports before include
// Pass through ports (type, name, default-value)
#define DEMUX_PASS_THRU_DATA_LIST(CALL)                                \
  CALL ( vil_image_resource_sptr,           image,                   NULL ) \
  CALL ( vidtk::image_to_image_homography,  src_to_ref_homography,   vidtk::image_to_image_homography() ) \
  CALL ( vidtk::image_to_utm_homography,    src_to_utm_homography,   vidtk::image_to_utm_homography() ) \
  CALL ( vidtk::image_to_plane_homography,  ref_to_wld_homography,   vidtk::image_to_plane_homography() ) \
  CALL ( double,                            world_units_per_pixel,   0.0 ) \
  CALL ( vidtk::video_modality,             video_modality,          vidtk::VIDTK_INVALID ) \
  CALL ( vidtk::shot_break_flags,           shot_break_flags,        vidtk::shot_break_flags() )


namespace vidtk {


// ----------------------------------------------------------------
/**
 *
 *
 */
class track_demultiplexer_process
  : public ::vidtk::process
{
public:
  typedef track_demultiplexer_process self_type;

  track_demultiplexer_process(std::string const& name );
  virtual ~track_demultiplexer_process();

  // -- process interface --
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual process::step_status step2();

  // -- process inputs --
  void input_timestamp ( vidtk::timestamp const& value );
  VIDTK_OPTIONAL_INPUT_PORT( input_timestamp, vidtk::timestamp const& );

  void input_tracks ( vidtk::track::vector_t const& value );
  VIDTK_OPTIONAL_INPUT_PORT( input_tracks, vidtk::track::vector_t const& );

  // -- process outputs --
  vidtk::timestamp output_timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, output_timestamp );

  vidtk::track::vector_t output_new_tracks ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::track::vector_t, output_new_tracks );

  vidtk::track::vector_t output_updated_tracks ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::track::vector_t, output_updated_tracks );

  vidtk::track::vector_t output_not_updated_tracks ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::track::vector_t, output_not_updated_tracks );

  vidtk::track::vector_t output_terminated_tracks ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::track::vector_t, output_terminated_tracks );

  vidtk::track::vector_t output_active_tracks ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::track::vector_t, output_active_tracks );


  // ------------- pass thru ports ------------
#define DECLARE_PASS_THRU_PORTS(T,N,D)                  \
void input_ ## N( T const& val);                        \
VIDTK_OPTIONAL_INPUT_PORT( input_ ## N, T const& );     \
T output_ ## N() const;                          \
VIDTK_OUTPUT_PORT( T, output_ ## N);

  DEMUX_PASS_THRU_DATA_LIST(DECLARE_PASS_THRU_PORTS)

#undef DECLARE_PASS_THRU_PORTS

protected:
  void reset_outputs();
  void reset_inputs();
  bool valid_inputs();

private:
  config_block config_;

  vidtk::track_demultiplexer demux_;

  vidtk::timestamp input_timestamp_;
  vidtk::track::vector_t input_tracks_;

  vidtk::timestamp output_timestamp_;

  int state_; // current operating state

  class impl;
  boost::scoped_ptr<impl> impl_;

}; // end class track_demultiplexer_process

} // end namespace


#endif /* _TRACK_DEMULTIPLEXER_PROCESS_H_ */
