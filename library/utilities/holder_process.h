/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_holder_process_h_
#define vidtk_holder_process_h_

#include <vcl_queue.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>


namespace vidtk
{

/// A process to hold on to one input value
///
/// This process is used to keep a single value in snyc with a queue_process.
/// A counter mirrors the size of queue, but only a single value is retained.
template <class TData>
class holder_process
  : public process
{
public:
  typedef holder_process self_type;

  holder_process( vcl_string const& name );

  ~holder_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void clear();

  void set_input_datum( TData const& item );

  void set_default_value( TData const& item );

  VIDTK_INPUT_PORT( set_input_datum, TData const& );

  TData & get_output_datum( );

  VIDTK_OUTPUT_PORT( TData &, get_output_datum );

protected:
  config_block config_;

  bool get_new_value_;

  TData default_value_;

  TData const* input_datum_;

  TData output_datum_;

};


} // end namespace vidtk


#endif // vidtk_holder_process_h_
