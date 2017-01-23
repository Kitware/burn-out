/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_external_settings_h_
#define vidtk_external_settings_h_

#include <utilities/config_block.h>
#include <utilities/string_to_vector.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>

#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>


namespace vidtk
{


/// \brief External arbitrary algorithm settings base class.
/**
 * An optional settings class to store all settings variables for a
 * particular class or function, either as public member variables, or in
 * vidtk config_block form, while still supporting both methods.
 * This class, together with it's associated macros, were created to satisfy
 * a few separate properties to simplify development, including:
 *
 *  - Being able to declare all settings in a contiguous list, without
 *    ever having the need to state the same information twice (default
 *    parameters, types, etc...) in multiple different locations within code.
 *  - Supporting the parsing of these options from a vidtk config block,
 *    from a file, while also allowing the usage of these settings via
 *    public member variables in case an internal or external application
 *    chooses not to use vidtk-style config_blocks.
 *  - In case the underlying structure of these config parameters ever
 *    change, they can be easily modified because they are declared in macro
 *    form, not in multiple places within the same class.
 *  - The functions contained within this class can easily be used within a
 *    vidtk-style processes without repeating any settings declarations.
 *
 * This class is used, for example, with most online descriptor generators
 * in order to have some settings made accessible to the users of these
 * generators, which all have settings which derive from this class for
 * shared operation.
 */
class external_settings
{
public:

  typedef std::string string_t;

  /// Constructor.
  external_settings() {}

  /// Destructor.
  virtual ~external_settings() {}

  /// Return a vidtk config block for all parameters.
  virtual config_block config() const = 0;

  /// Parse a vidtk style config block.
  /**
   * \throws config_block_parse_error
   */
  virtual void read_config( const config_block& blk ) = 0;

  /// Load these config parameters directly from a file.
  bool load_config_from_file( const string_t& filename );
};


// Standard type declaration and file parsing macros:
#define ext_settings_declare_param( NAME, TYPE, DEFAULT, DESC )                 \
  TYPE NAME;

#define ext_settings_add_config_param( NAME, TYPE, DEFAULT, DESC )              \
  conf.add_parameter( #NAME, boost::lexical_cast<string_t>(DEFAULT), DESC );

#define ext_settings_read_config_param( NAME, TYPE, DEFAULT, DESC )             \
  this-> NAME = blk.get< TYPE >( #NAME );

#define ext_settings_initialize_param( NAME, TYPE, DEFAULT, DESC )              \
  NAME = DEFAULT;


// This macro allows the declaration of an external_settings derived class given
// a name for the class, and a list of settings declared in macro form. This list
// of settings should contain a list of 4 parameters for each parameter: the
// invididual setting name, its type, its default value, and a description of
// the parameter. For example, the following will generate a simple settings
// class entitled "awesome_settings" for the given variables:
//
// define settings_macro( add_param )
//   add_param( some_percentage, double, 0.50, "Percentage of awesomeness" )
//   add_param( some_iteration_count, unsigned int, 10, "Number of itrs" )
//
// init_external_settings( awesome_settings, settings_macro )
//
// undef settings_macro
//
// Notes:
//
//  -This macro was originally designed for use with simple types only, however
//   it can be used with any class which has a stream operator defined for it
//   for string serialization.
#define init_external_settings( NAME, MACRO )                                   \
                                                                                \
class NAME : public external_settings                                           \
{                                                                               \
public:                                                                         \
  MACRO( ext_settings_declare_param )                                           \
                                                                                \
  config_block config() const                                                   \
  {                                                                             \
    config_block conf;                                                          \
    MACRO( ext_settings_add_config_param )                                      \
    return conf;                                                                \
  }                                                                             \
                                                                                \
  void read_config( const config_block& blk )                                   \
  {                                                                             \
    MACRO( ext_settings_read_config_param )                                     \
  }                                                                             \
                                                                                \
  NAME ()                                                                       \
  {                                                                             \
    MACRO( ext_settings_initialize_param )                                      \
  }                                                                             \
                                                                                \
  NAME ( const config_block& blk )                                              \
  {                                                                             \
    this->read_config( blk );                                                   \
  }                                                                             \
};                                                                              \


// Redefinition of init_external_settings macro for consistency.
#define init_external_settings1( NAME, MACRO )                                  \
  init_external_settings( NAME, MACRO )


// Array parameter declaration and file parsing macros:
#define ext_settings_declare_array( NAME, TYPE, SIZE, DEFAULT, DESC )           \
  std::vector< TYPE > NAME;

#define ext_settings_add_config_array( NAME, TYPE, SIZE, DEFAULT, DESC )        \
  conf.add_parameter( #NAME, DEFAULT, DESC );

#define ext_settings_read_config_array( NAME, TYPE, SIZE, DEFAULT, DESC )       \
  if( SIZE != 0 )                                                               \
  {                                                                             \
    this-> NAME .resize( SIZE );                                                \
    blk.get< TYPE >( #NAME, & this-> NAME [0], SIZE );                          \
  }                                                                             \
  else                                                                          \
  {                                                                             \
    std::string tmp_str = blk.get< std::string >( #NAME );                      \
    if( !string_to_vector( tmp_str, this-> NAME ) )                             \
    {                                                                           \
      throw config_block_parse_error( "Unable to read vector " + tmp_str );     \
    }                                                                           \
  }

#define ext_settings_initialize_array( NAME, TYPE, SIZE, DEFAULT, DESC )        \
  string_to_vector( DEFAULT, this-> NAME );


// Declare an external_settings class via macro in a method similar to the above,
// only one which additionally supports declaring an array of parameters with an
// extra argument. Example usage:
//
// define settings_macro( simple_param, array_param )
//   simple_param( some_percentage,
//                 double,
//                 0.50,
//                "Percentage of awesomeness" )
//   array_param( iteration_counts,
//                unsigned int,
//                5, <-- array size, can be 0 for unknown
//                "0 2 4 6 8", <-- default array values
//                "Array of iterations" )
//
// init_external_settings2( awesome_settings, settings_macro )
//
// undef settings_macro
//
#define init_external_settings2( NAME, MACRO )                                  \
                                                                                \
class NAME : public external_settings                                           \
{                                                                               \
public:                                                                         \
  MACRO( ext_settings_declare_param,                                            \
         ext_settings_declare_array )                                           \
                                                                                \
  config_block config() const                                                   \
  {                                                                             \
    config_block conf;                                                          \
                                                                                \
    MACRO( ext_settings_add_config_param,                                       \
           ext_settings_add_config_array )                                      \
                                                                                \
    return conf;                                                                \
  }                                                                             \
                                                                                \
  void read_config( const config_block& blk )                                   \
  {                                                                             \
    MACRO( ext_settings_read_config_param,                                      \
           ext_settings_read_config_array )                                     \
  }                                                                             \
                                                                                \
  NAME ()                                                                       \
  {                                                                             \
    MACRO( ext_settings_initialize_param,                                       \
           ext_settings_initialize_array )                                      \
  }                                                                             \
                                                                                \
  NAME ( const config_block& blk )                                              \
  {                                                                             \
    this->read_config( blk );                                                   \
  }                                                                             \
};                                                                              \


// Enum parameter declaration and file parsing macros:
#define ext_settings_declare_enum( NAME, VALUES, DEFAULT, DESC )                \
  enum NAME ## _t { BOOST_PP_SEQ_ENUM( VALUES ) } NAME ;                        \
  vidtk::config_enum_option NAME ## _parser;

#define ext_settings_add_config_enum( NAME, VALUES, DEFAULT, DESC )             \
  conf.add_parameter( #NAME, boost::algorithm::to_lower_copy< string_t >        \
    ( #DEFAULT ), DESC );

#define ext_settings_read_config_enum( NAME, VALUES, DEFAULT, DESC )            \
  NAME = blk.get< NAME ## _t >( NAME ## _parser, #NAME );

#define ext_settings_add_config_enum_pair( NAME, VALUE )                        \
  NAME ## _parser.add( boost::algorithm::to_lower_copy< string_t >              \
    ( #VALUE ) , VALUE );

#define ext_settings_initialize_enum_category( REP, NAME, VALUE )               \
  ext_settings_add_config_enum_pair( NAME, VALUE );

#define ext_settings_initialize_enum( NAME, VALUES, DEFAULT, DESC )             \
  BOOST_PP_SEQ_FOR_EACH( ext_settings_initialize_enum_category, NAME, VALUES ); \
  NAME = DEFAULT;


// Declare an external_settings class via macro in a method similar to the above,
// only one which additionally supports declaring enumuerations in an extra list.
//
// define settings_macro( simple_param, array_param, enum_param )
//   simple_param( some_percentage,
//                 double,
//                 0.50,
//                "Percentage of awesomeness" )
//   enum_param( some_category_var,
//               (POSSIBLE_VALUE1) (POSSIBLE_VALUE2),
//               POSSIBLE_VALUE1,
//               "Enum description" )
//
// init_external_settings3( awesome_settings, settings_macro )
//
// undef settings_macro
//

#define init_external_settings3( NAME, MACRO )                                  \
                                                                                \
class NAME : public external_settings                                           \
{                                                                               \
public:                                                                         \
  MACRO( ext_settings_declare_param,                                            \
         ext_settings_declare_array,                                            \
         ext_settings_declare_enum )                                            \
                                                                                \
  config_block config() const                                                   \
  {                                                                             \
    config_block conf;                                                          \
                                                                                \
    MACRO( ext_settings_add_config_param,                                       \
           ext_settings_add_config_array,                                       \
           ext_settings_add_config_enum )                                       \
                                                                                \
    return conf;                                                                \
  }                                                                             \
                                                                                \
  void read_config( const config_block& blk )                                   \
  {                                                                             \
    MACRO( ext_settings_read_config_param,                                      \
           ext_settings_read_config_array,                                      \
           ext_settings_read_config_enum )                                      \
  }                                                                             \
                                                                                \
  NAME ()                                                                       \
  {                                                                             \
    MACRO( ext_settings_initialize_param,                                       \
           ext_settings_initialize_array,                                       \
           ext_settings_initialize_enum )                                       \
  }                                                                             \
                                                                                \
  NAME ( const config_block& blk )                                              \
  {                                                                             \
    MACRO( ext_settings_initialize_param,                                       \
           ext_settings_initialize_array,                                       \
           ext_settings_initialize_enum )                                       \
                                                                                \
    this->read_config( blk );                                                   \
  }                                                                             \
};                                                                              \


} // end namespace vidtk

#endif // vidtk_external_settings_h_
