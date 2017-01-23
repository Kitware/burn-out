/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TAGGED_DATA_READER_PROCESS_H_
#define _TAGGED_DATA_READER_PROCESS_H_

#include "base_reader_process.h"
#include "tagged_data_ports.h"

namespace vidtk {

// ----------------------------------------------------------------
/** Metadata reader process.
 *
 * This process reads a tagged metadata file and produces outputs.
 * There are no enables for individual tags like in the writer.  The
 * tags found in the input file drive the outputs. If there is a tag,
 * then it is output. If there is no tag, then no output on that port.
 *
 * If an output is not needed, then don't hook it up.
 *
 * Note that the timestamp vector output is simulated by generating a
 * vector of one from the last timestamp.
 */
class tagged_data_reader_process
  : public base_reader_process
{
public:
  typedef tagged_data_reader_process self_type;

  tagged_data_reader_process(std::string const& name);
  virtual ~tagged_data_reader_process();

  // Process interface
  virtual bool set_params(config_block const&);
  virtual bool initialize_internal();


#define MDRW_INPUT(N,T,W,I)                             \
public:                                                 \
  T get_output_ ## N() { return m_ ## N; }       \
VIDTK_OUTPUT_PORT(  T, get_output_ ## N );       \
private:                                                \
  T m_ ## N;

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT


 public:

private:
  virtual int error_hook( int status);
  virtual int pre_step_hook();
  virtual void post_step_hook();

  int m_state;

}; // end class tagged_data_reader_process

} // end namespace

#endif /* _TAGGED_DATA_READER_PROCESS_H_ */
