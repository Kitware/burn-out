/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _STRING_FRIENDLY_ENUM_H_
#define _STRING_FRIENDLY_ENUM_H_

#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <string>
#include <map>

#define VIDTK_ENUM_STR_(r, data, elem) BOOST_PP_STRINGIZE(elem)
#define VIDTK_ENUM_MAP_(r, data, elem) retval[elem] = BOOST_PP_STRINGIZE(elem)

/**
 * \brief Macro to create enum with string equivalents.
 *
 * This macro is used to generate a string friendly enum. In addition
 * to generating the enum type, methods to convert enum to string,
 * string to enum and generate help text are generated.
 *
 * The generated enum and supporting inline methods are intended to
 * be generated within a class definition, otherwise errors will result.
 *
 * This macro intended to support the following:
 * - Generate a language acceptable enum definition
 * - Convert from string to enum code
 * - Convert from enum code to string
 * - Supply list of enum strings to be used in help text
 *
 * @param name Name of the enumeration type.
 * @param start Value of the first enum.
 * @param entries List of enum entries.
 *
 * Example:
 * \code
 #include "string_friendly_enum.h"

 STRING_FRIENDLY_ENUM(myEnumDefinition, 2,
   (MYENUM_YELLOW)           // enums delimited with parens instead of commas
   (MYENUM_RED)
   (GREEN)
   (BLUE)
   (MAGENTA)
   (ETC)
  )  // no semi-colon
 * \endcode
 *
 * Will generate the following inline methods
 *
 * \code

enum myEnumDefinition { MYENUM_YELLOW = 2, MYENUM_RED, GREEN, BLUE, MAGENTA, ETC, LAST_myEnumDefinition };

static std::string convert_myEnumDefinition_enum( myEnumDefinition val );
static myEnumDefinition convert_myEnumDefinition_enum( std::string const& n );
static std::map< enum_value, std::string > name ##_enum_list();
* \endcode
 */
#define STRING_FRIENDLY_ENUM(name, start, entries)                      \
  BOOST_PP_SEQ_ENUM(                                                    \
    (enum name { BOOST_PP_SEQ_HEAD(entries) = start)                    \
      BOOST_PP_SEQ_TAIL(entries)                                        \
       (LAST_##name };)                                                 \
    )                                                                   \
                                                                        \
  static std::string convert_ ## name ## _enum( name val )              \
  {                                                                     \
  BOOST_PP_SEQ_ENUM (                                                   \
    (const char* name ## _str[]={ BOOST_PP_STRINGIZE(BOOST_PP_SEQ_HEAD(entries)) ) \
     BOOST_PP_SEQ_TRANSFORM(VIDTK_ENUM_STR_, _, BOOST_PP_SEQ_TAIL(entries) ) \
      )                                                                 \
    };                                                                  \
                                                                        \
  if ( (val < start) || (val >= LAST_##name) ) { return "out-of-range"; } \
    return name ## _str[val-start];                                     \
  }                                                                     \
                                                                        \
  static name convert_ ## name ## _enum( std::string const& n )         \
  {                                                                     \
  BOOST_PP_SEQ_ENUM (                                                   \
    (const char* name ## _str[]={ BOOST_PP_STRINGIZE(BOOST_PP_SEQ_HEAD(entries)) ) \
     BOOST_PP_SEQ_TRANSFORM(VIDTK_ENUM_STR_, _, BOOST_PP_SEQ_TAIL(entries) ) \
      )                                                                 \
    };                                                                  \
                                                                        \
  for (unsigned int i = 0; i < (sizeof (name ## _str) / sizeof (name ## _str[0]) ); ++i) \
  {                                                                     \
    if (  name ## _str[i] == n ) { return static_cast< name >(i+start); } \
  }                                                                     \
  return static_cast< name >(-1);                                       \
  }                                                                     \
                                                                        \
  typedef std::map< name, std::string > name ##_enum_list_t;            \
  static name ##_enum_list_t name ##_enum_list()                        \
  {                                                                     \
    std::map< name, std::string > retval;                               \
                                                                        \
    BOOST_PP_SEQ_ENUM (                                                 \
      BOOST_PP_SEQ_TRANSFORM(VIDTK_ENUM_MAP_, _, entries )              \
      );                                                                \
                                                                        \
      return retval;                                                    \
  }

// The symbols VIDTK_ENUM_* can not be undef'd here because they are
// needed when the main macro is instantiated.

#endif /* _STRING_FRIENDLY_ENUM_H_ */
