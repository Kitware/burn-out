/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_async_observer_process_h_
#define vidtk_async_observer_process_h_

#include <string>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{

// ----------------------------------------------------------------
/** Synchronously observe a data stream.
 *
 * This process synchronously observes a data stream and makes any
 * data items available outside the pipeline. The common use case here
 * is a gui that observes pipeline results.
 *
 * The get_data() method will block until there is data available or
 * step() cycles without getting a datum.
 *
 * The set_input() method will block if the datum has not beed claimed
 * by get_data().
 *
 * The step() method will accept a data item if one was presented on
 * the input port. Step will block if the previous data element has
 * not been claimed via the get_data() method.
 *
 * Typical sequences are as follows:
 *
 * \msc
 pipeline,observer,gui;
 --- [ label = "typical sequence" ];
 pipeline=>observer [ label = "set_input()" ];
 pipeline=>observer [ label = "step()" ];
 gui=>observer [ label = "get_data()" ];
 gui<<observer [ label = "data item" ];

 --- [ label = "pipeline waiting" ];
 pipeline=>observer [ label = "set_input()" ];
 pipeline<<observer [ label = "set_input()" ];
 pipeline=>observer [ label = "step()" ];
 pipeline<<observer [ label = "step()" ];
 pipeline=>observer [ label = "set_input()" ];
 gui=>observer [ label = "get_data()" ];
 gui<<observer [ label = "data item" ];
 pipeline<<observer [ label = "set_input()" ];

 --- [ label = "gui waiting" ];
 gui=>observer [ label = "get_data()" ];
 pipeline=>observer [ label = "set_input()" ];
 pipeline=>observer [ label = "step()" ];
 gui<<observer [ label = "data item" ];

 --- [ label = "simple end of input" ];
 pipeline=>observer [ label = "set_input()" ];
 pipeline=>observer [ label = "step()" ];
 gui=>observer [ label = "get_data()" ];
 pipeline=>observer [ label = "step()" ];
 gui=>observer [ label = "get_data()" ];
 gui<<observer [ label = "returns false" ];

 --- [ label = "end of input" ];
 pipeline=>observer [ label = "set_input()" ];
 pipeline<<observer [ label = "set_input()" ];
 pipeline=>observer [ label = "step()" ];
 pipeline<<observer [ label = "step()" ];
 pipeline=>observer [ label = "step()" ];
 gui=>observer [ label = "get_data()" ];
 gui<<observer [ label = "data item" ];
 gui=>observer [ label = "get_data()" ];
 pipeline<<observer [ label = "step()" ];
 gui<<observer [ label = "returns false" ];
 * \endmsc
 */
template<typename T>
class async_observer_process : public process
{
public:
  typedef async_observer_process self_type;

  async_observer_process( const std::string &name);
  virtual ~async_observer_process();

  virtual config_block params() const;
  virtual bool set_params( const config_block &);
  virtual bool initialize();
  virtual bool step();

  /** Supply datum to the process.
   * @param[in] datum - data item
   */
  void set_input( const T &datum );
  // this port is optional so that it is "notified" when no more data is available
  VIDTK_OPTIONAL_INPUT_PORT( set_input, const T& );

  /** Get the next data item from outside the pipeline.
   * @param[out] ref - data item is returned here
   * @return \c true if datum is returned; \c false if there is no more data.
   * No data is passed back if false is returned.
   */
  bool get_data(T& ref);


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
