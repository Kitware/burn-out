/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_function_caller_2_process_h_
#define vidtk_function_caller_2_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>


namespace vidtk
{


namespace helper
{

template<class T>
struct holder
{
  holder() { }
  holder( T p ) : val_( p ) { }
  T value() const { return val_; }
  T val_;
};

template<class T>
struct holder<T&>
{
  holder() : ptr_( NULL ) { }
  holder( T& p ) : ptr_( &p ) { }
  T& value() const { return *ptr_; }
  T* ptr_;
};

}


template <class Arg1Type, class Arg2Type>
class function_caller_2_process
  : public process
{
public:
  typedef function_caller_2_process self_type;

  function_caller_2_process( vcl_string const& name );

  ~function_caller_2_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_function( boost::function< void(Arg1Type,Arg2Type) > const& f );

  void set_argument_1( Arg1Type v );

  void set_argument_2( Arg2Type v );

  VIDTK_INPUT_PORT( set_argument_1, Arg1Type );

  VIDTK_INPUT_PORT( set_argument_2, Arg2Type );

private:
  helper::holder<Arg1Type> arg1_;
  helper::holder<Arg2Type> arg2_;

  boost::function< void(Arg1Type,Arg2Type) > func_;
};


} // end namespace vidtk


// we can't expect to explicitly initialize this for all possible
// function calls.
#include "function_caller_2_process.txx"

#endif // vidtk_function_caller_2_process_h_
