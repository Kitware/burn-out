/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_downsampling_process_h_
#define vidtk_downsampling_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <pipeline_framework/pipeline_queue_monitor.h>
#include <pipeline_framework/pipeline.h>

#include <utilities/frame_downsampler.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

// ----------------------------------------------------------------
/**
 * \brief Downsampling process base class.
 *
 * This class implements the decision logic for when to down sample.
 * Derived classes must implement the inputs and outputs that define
 * the flow to be downsampled.
 *
 * A process which serves to downsample some input stream in one of two
 * circumstances, either when some later pipeline elements are processing
 * at a rate below that of the incoming data for an extended period of time,
 * or when the incoming data is being received at a higher-than-desired
 * frame rate without looking at any downstream processes.
 *
 * This process implements the downsampling by occasionally not
 * passing along any outputs.
 */
class downsampling_process
  : public process
{

public:

  typedef downsampling_process self_type;

  downsampling_process( std::string const& );
  virtual ~downsampling_process();

  virtual vidtk::config_block params() const;
  virtual bool set_params( vidtk::config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  /// Set pipeline to monitor. This call is required for certain operating
  /// modes of this process and generally points at the pipeline this
  /// node is within.
  virtual bool set_pipeline_to_monitor( const vidtk::pipeline* p );

  /// Set a known frame rate, if known from an external source.
  void set_frame_rate( const double frame_rate );

  /// Set a known total frame count, if known from an extenal source.
  void set_frame_count( const unsigned frame_count );

  /// Estimated output frame rate produced by this process.
  double output_frame_rate() const;

  /// Estimated total number of frames which will be passed by this
  /// process if a total frame count is set. Note: this is an estimate.
  unsigned output_frame_count() const;

protected:

  // Virtual function which must be defined by derived processes to return
  // the timestamp for the current frame, but only if the rate_limiter option
  // is going to be enabled.
  virtual vidtk::timestamp current_timestamp();

  // Virtual function which can be defined by derived processes to indicate
  // that the current frame is "critical" and must not be dropped in any
  // circumstance.
  virtual bool is_frame_critical();

  // Are we currently in a dedicated frame break?
  bool in_burst_break() const;

private:

  // Settings
  bool disabled_;
  vidtk::config_block config_;

  // The type of output to produce when dropping a frame
  process::step_status dropped_frame_output_;

  // Settings for the pipeline queue monitor and downsampler
  bool frame_downsampler_enabled_;
  bool queue_monitor_enabled_;
  unsigned max_allowed_queue_len_;
  std::vector< std::string > nodes_to_monitor_;
  const pipeline* pipeline_to_monitor_;

  // The pipeline queue monitor class used, if enabled
  boost::scoped_ptr< pipeline_queue_monitor > queue_monitor_;

  // The frame estimator class used, if enabled
  boost::scoped_ptr< frame_downsampler > frame_downsampler_;
};

}

#endif // vidtk_downsampling_process_h_
