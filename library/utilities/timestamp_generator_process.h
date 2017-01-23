/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_timestamp_generator_process_h_
#define vidtk_timestamp_generator_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>
#include <utilities/timestamp_generator.h>
#include <utilities/config_block.h>

namespace vidtk
{


/// A process to produce the vidtk::timestamp stream for a dataset.
///
/// This node will act as a root/source node in the pipeline.

class timestamp_generator_process
  : public process
{
public:
  typedef timestamp_generator_process self_type;

  timestamp_generator_process( std::string const& name );
  ~timestamp_generator_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

private:
  timestamp_generator generator_;
  config_block config_;

  // I/O data
  vidtk::timestamp ts_;
};


} // end namespace vidtk


#endif // vidtk_timestamp_generator_process_h_
