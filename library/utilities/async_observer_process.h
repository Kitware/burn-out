/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_async_observer_process_h_
#define vidtk_async_observer_process_h_

#include <vcl_string.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{

/// Store one input and then block until an asynchronous 3rd party observes it.
/// The common use case here is a gui that observes pipeline results.
template<typename T>
class async_observer_process : public process
{
public:
  typedef async_observer_process self_type;

  async_observer_process( const vcl_string &name);

  ~async_observer_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_input( const T &in );
  // this port is optional so that it is "notified" when no more data is available
  VIDTK_OPTIONAL_INPUT_PORT( set_input, const T& );

  /// Get the next data item from outside the pipeline, return true if more data is available
  bool get_data(T&);


private:
  config_block config_;

  T data_;

  bool data_available_;
  bool data_set_;
  bool is_done_;
  boost::condition_variable cond_data_available_;
  boost::mutex mut_;
};

}

#endif
