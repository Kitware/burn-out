/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pipeline_aid_h_
#define vidtk_pipeline_aid_h_

/**
  \file
   \brief
   Implementation of functions to help aid the pipeline.

   NOTE: If any of the port macros are to be used, the user class MUST define a
   typedef "self_type" that is set to the type of the class that is using the
   macros. For example, if a class "foo" is going to use one of the input/output
   port macros defined in this header, a type def of the form
   "typedef foo self_type;" would need to exist in the class definition.
*/

#include <utility>
#include <string>
#include <functional>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/typeof/std/utility.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <pipeline_framework/pipeline_node.h>
#include <utilities/deprecation.h>

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
  node_id_t id_;
  ///Boost output function
  boost::function< ReturnType() > out_func_;
  ///Output port name
  std::string name_;
};

///Description of an input port
template<class ArgumentType>
struct input_port_description
{
  ///Input port id
  node_id_t id_;
  ///Boost input function
  boost::function< void(ArgumentType) > in_func_;
  ///Input port name
  std::string name_;
  ///True if the input port is optional; otherwise false
  bool optional_;
  /// True if the port is allowed to be connected multiple times.
  bool multiple_;
};

template<class ReturnType>
output_port_description<ReturnType>
make_output( node_id_t id,
             boost::function< ReturnType() > const& func,
             std::string const& name )
{
  output_port_description<ReturnType> d;
  d.id_ = id;
  d.out_func_ = func;
  d.name_ = name;
  return d;
}


template<class ArgumentType>
input_port_description<ArgumentType>
make_input( node_id_t id,
            boost::function< void(ArgumentType) > const& func,
            std::string const& name,
            bool optional = false,
            bool multiple = false )
{
  input_port_description<ArgumentType> d;
  d.id_ = id;
  d.in_func_ = func;
  d.name_ = name;
  d.optional_ = optional;
  d.multiple_ = multiple;
  return d;
}


} // end namespace pipeline_aid
} // end namespace vidtk


// MSVC complains about typename outside of a template. This isn't allowed in
// C++03. Doing this hides the warnings on Windows, but since this is a stopgap
// solution until we make it an error, not warning on Windows should be fine.
#ifdef _WIN32
#define NO_TYPENAME_ALLOWED_OUTSIDE_TEMPLATE
#endif
#ifdef __GNUC__
// GCC 4.4 doesn't accept this either.
#if (__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ <= 4))
#define NO_TYPENAME_ALLOWED_OUTSIDE_TEMPLATE
#endif
#endif

/* Assertions about return types not matching the method exactly.
 * This can cause subtle errors if they do not match exactly.
 */
#ifndef NO_TYPENAME_ALLOWED_OUTSIDE_TEMPLATE
#define VIDTK_CHECK_PORT_TYPES
#endif
#ifdef VIDTK_CHECK_PORT_TYPES
#define CHECK_INPUT_PORT_TYPES(ArgumentType, Function)                                                     \
  typedef BOOST_TYPEOF(&Function) FuncType;                                                                \
  typedef typename boost::mpl::at_c<boost::function_types::parameter_types<FuncType>,1>::type FuncArgType; \
  BOOST_STATIC_ASSERT((boost::is_same<ArgumentType, FuncArgType>::value));
#define CHECK_OUTPUT_PORT_TYPES(ReturnType, Function)                                 \
  typedef BOOST_TYPEOF(&Function) FuncType;                                           \
  typedef typename boost::function_types::result_type<FuncType>::type FuncResultType; \
  BOOST_STATIC_ASSERT((boost::is_same<ReturnType, FuncResultType>::value));
#else
#define CHECK_INPUT_PORT_TYPES(ArgumentType, Function)
#define CHECK_OUTPUT_PORT_TYPES(ReturnType, Function)
#endif

/* Assertion that the return type is not a reference.
 * This can cause errors in an async pipeline without pads.
 */
//#define VIDTK_OUTPUT_REFERENCES_ARE_ERRORS
#ifdef VIDTK_OUTPUT_REFERENCES_ARE_ERRORS
#define CHECK_OUTPUT_REFERENCES(ReturnType) \
  BOOST_STATIC_ASSERT((!boost::is_reference<ReturnType>::value));
#else
#ifndef NO_TYPENAME_ALLOWED_OUTSIDE_TEMPLATE
namespace
{

VIDTK_DEPRECATED("Output ports returning a const& are discouraged and should be avoided.")
inline void check_reference_type(boost::mpl::true_*)
{
}

inline void check_reference_type(...)
{
}

}

#define CHECK_OUTPUT_REFERENCES(ReturnType) \
  check_reference_type(static_cast<typename boost::is_reference<ReturnType>::type*>(NULL));
#else
#define CHECK_OUTPUT_REFERENCES(ReturnType)
#endif
#endif



#define VIDTK_OUTPUT_PORT( ReturnType, FunctionName )                   \
  VIDTK_OUTPUT_PORT_( ReturnType, FunctionName )


#define VIDTK_OUTPUT_PORT_( ReturnType, FunctionName )                  \
  vidtk::pipeline_aid::output_port_description< ReturnType >            \
  FunctionName ## _port () {                                            \
    CHECK_OUTPUT_REFERENCES(ReturnType)                                 \
    CHECK_OUTPUT_PORT_TYPES(ReturnType, self_type::FunctionName)        \
    return vidtk::pipeline_aid::make_output<ReturnType>(                \
      this,                                                             \
      boost::function< ReturnType() > (                                 \
        boost::bind( std::mem_fun( &self_type::FunctionName ), this ) ), \
      #FunctionName );                                                  \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )


#define VIDTK_OUTPUT_PORT_1_ARG( ReturnType, FunctionName, Arg1Type )   \
  VIDTK_OUTPUT_PORT_1_ARG_( ReturnType, FunctionName, Arg1Type )


#define VIDTK_OUTPUT_PORT_1_ARG_( ReturnType, FunctionName, Arg1Type )  \
  vidtk::pipeline_aid::output_port_description< ReturnType >            \
  FunctionName ## _port ( Arg1Type arg1 ) {                             \
    CHECK_OUTPUT_REFERENCES(ReturnType)                                 \
    CHECK_OUTPUT_PORT_TYPES(ReturnType, self_type::FunctionName)        \
    return vidtk::pipeline_aid::make_output<ReturnType>(                \
      this,                                                             \
      boost::function< ReturnType() > (                                 \
        boost::bind( &self_type::FunctionName, this, arg1 ) ),          \
      #FunctionName );                                                  \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )



#define VIDTK_INPUT_PORT( FunctionName, ArgumentType )                  \
  VIDTK_INPUT_PORT_( FunctionName, ArgumentType )


#define VIDTK_INPUT_PORT_( FunctionName, ArgumentType )                 \
  vidtk::pipeline_aid::input_port_description<ArgumentType>             \
  FunctionName ## _port () {                                            \
    CHECK_INPUT_PORT_TYPES(ArgumentType, self_type::FunctionName)       \
    return vidtk::pipeline_aid::make_input<ArgumentType>(               \
      this,                                                             \
      boost::function< void(ArgumentType) > (                           \
        boost::bind( &self_type::FunctionName, this, _1 ) ),            \
      #FunctionName,                                                    \
      false );                                                          \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )


#define VIDTK_OPTIONAL_INPUT_PORT( FunctionName, ArgumentType )         \
  VIDTK_OPTIONAL_INPUT_PORT_( FunctionName, ArgumentType )


#define VIDTK_OPTIONAL_INPUT_PORT_( FunctionName, ArgumentType )        \
  vidtk::pipeline_aid::input_port_description<ArgumentType>             \
  FunctionName ## _port () {                                            \
    CHECK_INPUT_PORT_TYPES(ArgumentType, self_type::FunctionName)       \
    return vidtk::pipeline_aid::make_input<ArgumentType>(               \
      this,                                                             \
      boost::function< void(ArgumentType) > (                           \
        boost::bind( &self_type::FunctionName, this, _1 ) ),            \
      #FunctionName,                                                    \
      true );                                                           \
  }                                                                     \
  VIDTK_ALLOW_SEMICOLON_AT_END( FunctionName )


#define VIDTK_MULTI_INPUT_PORT( FunctionName, ArgumentType )            \
  VIDTK_MULTI_INPUT_PORT_( FunctionName, ArgumentType )


#define VIDTK_MULTI_INPUT_PORT_( FunctionName, ArgumentType )           \
  vidtk::pipeline_aid::input_port_description<ArgumentType>             \
  FunctionName ## _port () {                                            \
    return vidtk::pipeline_aid::make_input(                             \
      this,                                                             \
      boost::function< void(ArgumentType) > (                           \
        boost::bind( &self_type::FunctionName, this, _1 ) ),            \
      #FunctionName,                                                    \
      false,                                                            \
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
