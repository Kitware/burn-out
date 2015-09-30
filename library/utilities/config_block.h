/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
@mainpage Common Header Files SDK
Welcome to the Common Header Files SDK!
Here you will find header files that can be helpful in both activity and event detector code.

  Kitware (c) 2010
  Restrictions applicable to use by the US Government:
  UNLIMITED RIGHTS
  Restrictions applicable for all other users:
  This software and all information and expression are the property of Kitware, Inc. All Rights Reserved.
  <p><a href="../../../html/index.html" class="el">Return to SDK Documentation Home Page </a></p>
*/

/** \file
    \brief
    Method and field definitions for use with configuration blocks.
*/

#ifndef vidtk_config_block_h_
#define vidtk_config_block_h_

#include <vcl_string.h>
#include <vcl_utility.h>
#include <vcl_vector.h>
#include <vcl_map.h>
#include <vcl_ostream.h>
#include <vcl_stdexcept.h>

#include <utilities/checked_bool.h>

namespace vidtk
{

///Structure to set configuration block values
struct config_block_value
{
  ///Constructor
  config_block_value();
  ///Set the user value
  void set_user_value( vcl_string const& );
  ///Set the default value
  void set_default( vcl_string const& );
  ///Set the optional value
  void set_optional( bool );
  ///Set the description
  void set_description( vcl_string const& );
  ///Returns true is configuration block has a value.
  bool has_value() const;
  ///String value
  vcl_string value() const;
  ///If the given is optional return true; otherwise return false.
  bool is_optional() const;
  ///User value
  vcl_string user_value_;
  ///True if user value is set; otherwise false.
  bool has_user_value_;
  ///Default string
  vcl_string default_;
  ///True is a default is set; otherwise false.
  bool has_default_;
  ///True is optional; otherwise false.
  bool optional_;
  ///Description of configuration block.
  vcl_string description_;
};

///Method and field definitions for use with configuration blocks.
class config_block
{
public:
  ///Constructor.
  config_block();

  /// \brief Add a name to the component check list.
  ///
  /// By default, all parameters loaded from the "environment" (config file,
  /// command line, etc.) are checked to make sure that they were first registered by
  /// the app.  If you're sharing a config file between executables, however,
  /// you don't want program A to trap out because it doesn't register program B's
  /// parameters.  Hence this "component check list": a list of components ( e.g.
  ///"tracker", "pvo", etc.) interpreted as parameter prefixes.  If the list is
  /// non-empty, only unregistered parameters with prefixes appearing in this list
  /// will throw.
  ///
  /// The intention is that programs sharing a config file will add their parameters
  /// as a sub-block in each application, and then add that sub-block's name to the
  /// check list.
  void add_component_to_check_list( const vcl_string& name );

  /// \brief Reset the component check list.
  void reset_check_list();

  /// \brief See below.
  ///
  ///\deprecated  Use add_parameter()
  void add( vcl_string const& name );

  /// \brief See below.
  ///
  /// \deprecated  Use add_parameter()
  void add( vcl_string const& name, vcl_string const& default_value );

  /// \brief Specify an optional parameter.
  ///
  /// If the user does not provide a value for this parameter, there
  /// is no fall back "default" value.  However, it is not an error for
  /// the user not to provide this value.
  void add_optional( vcl_string const& name,
                     vcl_string const& description );

  /// \brief Specify a required parameter.
  ///
  /// This parameter will not have a default value, so the user will
  /// be forced to provide a value for it.  It is an error if the user
  /// does not provide this value.
  void add_parameter( vcl_string const& name,
                      vcl_string const& description );

  /// \brief Specify a required parameter that has a reasonable default.
  ///
  /// If the user does not provide a value for this parameter, the
  /// default will be used.  In general, the process should not need
  /// to distinguish between whether the user provided an explicit
  /// value, or left the default.  If you do need to distinguish,
  /// consider using add_optional().
  void add_parameter( vcl_string const& name,
                      vcl_string const& default_value,
                      vcl_string const& description );

  /// \brief Merge the contents of another block into this block.
  ///
  /// The contents of \a blk is merged into this block.  \a blk should
  /// not introduce any names at already exist in this block.
  void merge( config_block const& blk );

  /// \brief Update the contents of the block with the contents of another block.
  ///
  /// \a blk should not contain any names that are not already present
  /// in this block.  The user values of this block are updated by the
  /// values (user or default) of \a blk.  The defaults of this block
  /// are unchanged.
  void update( config_block const& blk );

  /// \brief Removes the specified parameters from the current this block.
  /// 
  /// \a blk should not contain any names that are not already present
  /// in this block.  The parameters identified in \a blk are removed
  /// the block in the current object.
  void remove( config_block const& blk );

  /// \brief Incorporate \a blk into the config block.
  ///
  /// All the parameter names in \a blk will be prefixed with "\a
  /// sub_block_name:" when adding them.
  ///
  /// \sa subblock.
  void add_subblock( config_block const& blk, vcl_string const& subblock_name );

  /// \brief Extract a subset of parameters from the config block.
  ///
  /// All the parameters that begin with "\a sub_block_name:" will be
  /// extracted into a new block.  The prefix will be removed when
  /// extracting the block.
  ///
  /// \sa add_subblock.
  config_block subblock( vcl_string const& sub_block_name ) const;


  /// \brief Does the parameter \a name have a value set?
  ///
  /// This could be the default value or a user-specified value.
  bool has( vcl_string const& name ) const;

  ///Set the checked bool value.
  checked_bool set( vcl_string const& name, vcl_string const& val );

  ///Get the checked bool value given the name and value.
  template<class T>
  checked_bool set( vcl_string const& name, T const& val );
  ///Return a the checked bool value as a const variable given the name and value as a unsigned char.
  checked_bool get( vcl_string const& name, unsigned char& val ) const;
  ///Return the checked bool value as a const variable given the name and value as a signed char.
  checked_bool get( vcl_string const& name, signed char& val ) const;
  ///Return the checked bool value as a const variable given the name and value as a string.
  checked_bool get( vcl_string const& name, vcl_string& val ) const;
  ///Return the checked bool value as a const variable given the name and value as a bool.
  checked_bool get( vcl_string const& name, bool& val ) const;
  ///Return the checked bool value as a const variable given name and value.
  template<class T>
  checked_bool get( vcl_string const& name, T& val ) const;
  ///Return the checked bool value as a const variable given name and value.
  template<class T, unsigned N>
  checked_bool get( vcl_string const& name, T (&val)[N] ) const;


  /// Set the value, and return *this.
  ///
  /// This is useful for code blocks like
  /// \code
  ///    process.set_params( process.params()
  ///                        .set_value( "dist", 5 )
  ///                        .set_value( "angle", 14 ) );
  /// \endcode
  ///
  /// Will throw unchecked_return_value if any of the set calls fail.
  template<class T>
  config_block& set_value( vcl_string const& name, T const& val );


  /// "Critical" or "checked" retrieval.
  ///
  /// Tries to read the parameter named \a name as a value of type \c
  /// T. Throws an error on failure. Use like
  /// \code
  ///    get<double>( "name" );
  /// \endcode
  template<class T>
  T get( vcl_string const& name ) const;

  ///Parse the given file.
  checked_bool parse( vcl_string const& filename );

  /// \brief Parse the arguments as if they were lines in a config file.
  ///
  /// Each argument should correspond to a line, so an example
  /// execution of a program that calls this routine may look like
  ///
  /// \code
  ///   executable "block src" "type = image_list" "image_list::glob = *.jpg" "endblock"
  /// \endcode
  ///
  /// Unknown or unparsable arguments are \b not ignored, and cause an
  /// error (return \c false).
  ///
  /// In keeping with the traditional command line argument tradition
  /// where argv[0] is the executable file name, argv[0] is ignored by
  /// the parser.
  checked_bool parse_arguments( int argc, char** argv );
  ///Print to file.
  void print( vcl_ostream& ostr ) const;
  ///Set the failt flag on the missing block.
  void set_fail_on_missing_block( bool flag );
  ///Map of strings to enumerate values
  vcl_map< vcl_string, config_block_value > enumerate_values() const;

private:
  void add( vcl_string const& name, config_block_value const& val );

  typedef vcl_map< vcl_string, config_block_value > value_map_type;
  value_map_type vmap_;

  // Added this optional flag to suppress the set_param() failure because
  // config file contains processes that have not been added to the pipeline.
  bool fail_on_missing_block_;

  // the component check list.
  vcl_vector<vcl_string> check_list_;

};
///Definition of configuration block parse error.
class config_block_parse_error
  : public vcl_runtime_error
{
public:
  ///Constructor: pass error message.
  config_block_parse_error( vcl_string const& msg )
    : vcl_runtime_error( msg )
  {
  }
};


} // end namespace vitk

#include "config_block.txx"

#endif // vidtk_config_block_h_
