/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pipeline_aid_h_
#define vidtk_pipeline_aid_h_

/**
  \file
   \brief
   Implementation of functions to help aid the pipeline.
*/

#include <vcl_utility.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>

//The following defines are to take care of the semicolon added after
//the templated functions.  In VC 8, 9, and 10 the *_port_unimplemnted_by_design
//causes many, many, many warnings.
//NOTE: We might want to come up with a cleaner solutions to this.
#if defined(VCL_VC_8) || defined(VCL_VC_9) || defined (VCL_VC_10)
#  define VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )
#else
#  define VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName ) \
  void FunctionName ## _port_unimplemented_by_design()
#endif

namespace vidtk
{
/**
\namespace vidtk::pipeline_aid
\brief Pipeline aid
*/
namespace pipeline_aid
{

///Description of an ouput port
template<class ReturnType>
struct output_port_description
{
  ///Output port id
  void* id_;
  ///Boost output function
  boost::function< ReturnType() > out_func_;
  ///Output port name
  vcl_string name_;
};

///Description of an input port
template<class ArgumentType>
struct input_port_description
{
  ///Input port id
  void* id_;
  ///Boost input function
  boost::function< void(ArgumentType) > in_func_;
  ///Input port name
  vcl_string name_;
  ///True if the input port is optional; otherwise false
  bool optional_;
};

template<class ReturnType>
output_port_description<ReturnType>
make_output( void* id,
             boost::function< ReturnType() > const& func,
             vcl_string const& name )
{
  output_port_description<ReturnType> d;
  d.id_ = id;
  d.out_func_ = func;
  d.name_ = name;
  return d;
}


template<class ArgumentType>
input_port_description<ArgumentType>
make_input( void* id,
            boost::function< void(ArgumentType) > const& func,
            vcl_string const& name,
            bool optional = false )
{
  input_port_description<ArgumentType> d;
  d.id_ = id;
  d.in_func_ = func;
  d.name_ = name;
  d.optional_ = optional;
  return d;
}


} // end namespace pipeline_aid
} // end namespace vidtk



#define VIDTK_OUTPUT_PORT( ReturnType, FunctionName )                   \
  vidtk::pipeline_aid::output_port_description< ReturnType >            \
  FunctionName ## _port () {                                            \
    return vidtk::pipeline_aid::make_output(                            \
      this,                                                             \
      boost::function< ReturnType() > (                                 \
        boost::bind( vcl_mem_fun( &self_type::FunctionName ), this ) ), \
      #FunctionName );                                                  \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )


#define VIDTK_OUTPUT_PORT_1_ARG( ReturnType, FunctionName, Arg1Type )   \
  vidtk::pipeline_aid::output_port_description< ReturnType >            \
  FunctionName ## _port ( Arg1Type arg1 ) {                             \
    return vidtk::pipeline_aid::make_output(                            \
      this,                                                             \
      boost::function< ReturnType() > (                                 \
        boost::bind( &self_type::FunctionName, this, arg1 ) ),          \
      #FunctionName );                                                  \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )



#define VIDTK_INPUT_PORT( FunctionName, ArgumentType )                  \
  vidtk::pipeline_aid::input_port_description<ArgumentType>             \
  FunctionName ## _port () {                                            \
    return vidtk::pipeline_aid::make_input(                             \
      this,                                                             \
      boost::function< void(ArgumentType) > (                           \
        boost::bind( &self_type::FunctionName, this, _1 ) ),            \
      #FunctionName,                                                    \
      false );                                                          \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )

#define VIDTK_OPTIONAL_INPUT_PORT( FunctionName, ArgumentType )         \
  vidtk::pipeline_aid::input_port_description<ArgumentType>             \
  FunctionName ## _port () {                                            \
    return vidtk::pipeline_aid::make_input(                             \
      this,                                                             \
      boost::function< void(ArgumentType) > (                           \
        boost::bind( &self_type::FunctionName, this, _1 ) ),            \
      #FunctionName,                                                    \
      true );                                                           \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )


#define VIDTK_PASS_THRU_PORT(BASENAME, TYPE)                    \
private:                                                        \
  TYPE BASENAME ## _data;                                       \
public:                                                         \
  void set_input_ ## BASENAME( TYPE const& v)                   \
  {                                                             \
    BASENAME ## _data = v;                                      \
  }                                                             \
  VIDTK_INPUT_PORT( set_input_ ## BASENAME, TYPE const& );      \
  TYPE get_output_ ## BASENAME()                                \
  {                                                             \
    return BASENAME ## _data;                                   \
  }                                                             \
  VIDTK_OUTPUT_PORT( TYPE, get_output_ ## BASENAME );           \
  VIDTK_ALLOW_SEMICOLON_AT_END( BASENAME )


#define VIDTK_OPTIONAL_PASS_THRU_PORT(BASENAME, TYPE)                   \
private:                                                                \
  TYPE BASENAME ## _data;                                               \
public:                                                                 \
  void set_input_ ## BASENAME( TYPE const& v)                           \
  {                                                                     \
    BASENAME ## _data = v;                                              \
  }                                                                     \
  VIDTK_OPTIONAL_INPUT_PORT( set_input_ ## BASENAME, TYPE const& );     \
  TYPE get_output_ ## BASENAME()                                        \
  {                                                                     \
    return BASENAME ## _data;                                           \
  }                                                                     \
  VIDTK_OUTPUT_PORT( TYPE, get_output_ ## BASENAME );                   \
  VIDTK_ALLOW_SEMICOLON_AT_END( BASENAME )


#endif // vidtk_pipeline_aid_h_
