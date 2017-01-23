/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_rescale_tracks_process_h_
#define vidtk_rescale_tracks_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/rescale_tracks.h>

#include <tracking_data/track.h>

namespace vidtk
{

/// \brief Upscale the image coordinates stored in some track vector.
///
/// This class is useful for upscaling the image coordinates stored in a
/// track vector. It may come into play when we have downsampled some source
/// imagery, run it through the tracker, then want to upscale the tracks
/// to the scale of the original source imagery. Output tracks will be deep
/// copied if the process is not disabled, otherwise inputs will be passed.
class rescale_tracks_process
  : public process
{
public:
  typedef rescale_tracks_process self_type;

  rescale_tracks_process( std::string const& name );
  virtual ~rescale_tracks_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  void set_input_tracks( track::vector_t const& trks );
  VIDTK_INPUT_PORT( set_input_tracks, track::vector_t const& );

  track::vector_t output_tracks() const;
  VIDTK_OUTPUT_PORT( track::vector_t, output_tracks );

private:

  config_block config_;

  bool disabled_;
  rescale_tracks_settings parameters_;

  track::vector_t inputs_;
  track::vector_t outputs_;
};


} // end namespace vidtk


#endif // vidtk_rescale_tracks_process_h_
