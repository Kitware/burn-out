/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_function_caller_process_h_
#define vidtk_function_caller_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <process_framework/function_caller_helper.h>


namespace vidtk
{


template <class ArgType>
class function_caller_process
  : public process
{
public:
  typedef function_caller_process self_type;

  function_caller_process( std::string const& name );

  ~function_caller_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_function( boost::function< void(ArgType) > const& f );

  void set_argument( ArgType v );

  VIDTK_INPUT_PORT( set_argument, ArgType );

private:
  helper::holder<ArgType> arg1_;

  boost::function< void(ArgType) > func_;
};


} // end namespace vidtk


// we can't expect to explicitly initialize this for all possible
// function calls.
#include "function_caller_process.txx"

#endif // vidtk_function_caller_process_h_
