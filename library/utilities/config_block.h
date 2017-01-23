/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_config_block_h_
#define vidtk_config_block_h_

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

#include <utilities/checked_bool.h>
#include <utilities/deprecation.h>

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <ostream>
#include <stdexcept>

namespace vidtk
{

/** Table for managing enum options.
 *
 * This table is used in cases where the valid config options consists
 * of a finite set of strings that need to be converted to an
 * enum. Entries for converting a string to an enum are added using
 * the add() method.
 *
 * Example
\code
enum mode
{
   VIDTK_METADATA_FIXED = 1,
   VIDTK_METADATA_STREAM,
   VIDTK_METADATA_ORTHO
};

vidtk::config_enum_option metadata_mode;
metadata_mode.add( "stream", VIDTK_METADATA_STREAM)
             .add( "fixed", VIDTK_METADATA_FIXED)
             .add( "ortho", VIDTK_METADATA_ORTHO);

try
{
  // Get option and validate against table
  typedef world_coord_super_process_impl< PixType > wcsp_impl_t;
  impl_->metadata_type = blk.get< typename wcsp_impl_t::mode >( metadata_mode, "metadata_type");
}
catch( config_block_parse_error const& e)
{
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what());

    return false;
}
\endcode
*/
class config_enum_option
{
public:

  /** @brief Add conversion entry.
   *
   * A name to enum conversion entry is added with this call.
   *
   * @param[in] name - String from config that corresponds to enum value.
   * @param[in] val - Enum value for this entry.
   * @return Reference to the object is returned so multiple entries
   * can be easily added.
   */
  config_enum_option& add( std::string const& name, int val );

  /** @brief Find enum value for name.
   *
   * The entry corresponding to the supplied string returned if the
   * name is found.
   *
   * @param[in] name - String representation for enum
   * @param[out] ret_val - Enum value
   *
   * @return \b false is returned if the name is not found. \b true is
   * returned along with the enum value if the name is found.
   */
  bool find( std::string const& name, int& ret_val ) const;

  /** @brief Get list of strings.
   *
   * The list of added names is returned. This is useful for
   * formulating diagnostic messages.
   */
  std::string list() const;

private:
  std::map < std::string, int > list_;
};


// ----------------------------------------------------------------
/// Structure to set configuration block values
struct config_block_value
{
  /// Constructor
  config_block_value();
  /// Set the user value
  void set_user_value( std::string const& );
  /// Set the default value
  void set_default( std::string const& );
  /// Set the optional value
  void set_optional( bool );
  /// Set the description
  void set_description( std::string const& );
  /// Returns true if configuration block has a value.
  bool has_value() const;
  /// Returns true if configuration block has a user value.
  bool has_user_value() const;
  /// Returns true if configuration block has a default value.
  bool has_default_value() const;
  /// String value
  std::string value() const;
  /// If the given is optional return true; otherwise return false.
  bool is_optional() const;
  /// Description
  std::string description() const;
  /// Default value
  std::string default_value() const;
  /// User value
  std::string user_value() const;
private:
  /// User value
  std::string user_value_;
  /// True if user value is set; otherwise false.
  bool has_user_value_;
  /// Default string
  std::string default_;
  /// True is a default is set; otherwise false.
  bool has_default_;
  /// True is optional; otherwise false.
  bool optional_;
  /// Description of configuration block.
  std::string description_;
};


// ----------------------------------------------------------------
/** Set of related configuration times.
 *
 * A configuration block contains a set of related configuration
 * items. A config item consists of a name, a value, and some other
 * stuff. See vidtk::config_block_value for all fields.
 *
 * A configuration block can conatin the parameters for a single
 * process or contain hierarchically structured set of parameters.
 *
 * @sa config_block_value
 */
class config_block
{
public:
  /// Constructor.
  config_block();

  /// Destructor.
  virtual ~config_block();

  /// \brief Add a name to the component check list.
  ///
  /// By default, all parameters loaded from the "environment" (config file,
  /// command line, etc.) are checked to make sure that they were first registered by
  /// the app.  If you're sharing a config file between executables, however,
  /// you don't want program A to trap out because it doesn't register program B's
  /// parameters.  Hence this "component check list": a list of components ( e.g.
  /// "tracker", "pvo", etc.) interpreted as parameter prefixes.  If the list is
  /// non-empty, only unregistered parameters with prefixes appearing in this list
  /// will throw.
  ///
  /// The intention is that programs sharing a config file will add their parameters
  /// as a sub-block in each application, and then add that sub-block's name to the
  /// check list.
  void add_component_to_check_list( const std::string& name );

  /// \brief Reset the component check list.
  void reset_check_list();

  /** Specify an optional parameter.
   * This method adds a config parameter to che config block.
   * If the user does not provide a value for this parameter, there
   * is no fall back "default" value.  However, it is not an error for
   * the user not to provide this value.
   * @param[in] name - name of parameter
   * @param[in] description - parameter description
   */
  virtual void add_optional( std::string const& name,
                             std::string const& description );

  /** Specify a required parameter.
   * This method adds a config parameter to the config block.
   * This parameter will not have a default value, so the user will
   * be forced to provide a value for it.  It is an error if the user
   * does not provide this value.
   *
   * @param[in] name - name of parameter
   * @param[in] description - parameter description
   */
  virtual void add_parameter( std::string const& name,
                              std::string const& description );

  /** Specify a required parameter that has a reasonable default.
   * This method adds a config parameter to the config block.
   * If the user does not provide a value for this parameter, the
   * default will be used. In general, the process should not need
   * to distinguish between whether the user provided an explicit
   * value, or left the default.  If you do need to distinguish,
   * consider using add_optional().
   *
   * A parameter can be added using this method multiple times without
   * causing an error. The last set of values added will be retained.
   *
   * @param[in] name - name of parameter
   * @param[in] default_value - default value for parameter
   * @param[in] description - parameter description
   */
  virtual void add_parameter( std::string const& name,
                              std::string const& default_value,
                              std::string const& description );

  /// \brief Merge the contents of another block into this block.
  ///
  /// The contents of \a blk is merged into this block.  \a blk must
  /// not introduce any names that already exist in this block.
  /// @param[in] blk - config block to be merged with this.
  virtual void merge( config_block const& blk );

  /// \brief Update the contents of the block with the contents of another block.
  ///
  /// \a blk must not contain any names that are not already present
  /// in this block.  The user values of this block are updated by the
  /// values (user or default) of \a blk.  The defaults of this block
  /// are unchanged.
  /// @param[in] blk - config block for updating this
  virtual void update( config_block const& blk );

  /// \brief Removes the specified parameters from the current this block.
  ///
  /// \a blk should not contain any names that are not already present
  /// in this block.  The parameters identified in \a blk are removed
  /// the block in the current object.
  /// @param[in] blk - config block containing items to remove from this
  virtual void remove( config_block const& blk );

  /// \brief Incorporate \a blk into the config block.
  ///
  /// All the parameter names in \a blk will be prefixed with "\a
  /// sub_block_name:" when adding them.
  ///
  /// @param[in] blk - config block to be added to this as a sub block
  /// @param[in] subblock_name - name of the sub block to add
  virtual void add_subblock( config_block const& blk, std::string const& subblock_name );

  /// \brief Extract a subset of parameters from the config block.
  ///
  /// All the parameters that begin with "\a subblock_name:" will be
  /// extracted into a new block.  The prefix will be removed when
  /// extracting the block.
  ///
  /// \sa add_subblock.
  virtual config_block subblock( std::string const& subblock_name ) const;

  /// \brief Move a subblock to a new location within this config block.
  ///
  /// All of the input parameters in the subblock given by the input
  /// string will be moved to a new subblock. All old parameters will
  /// be removed.
  ///
  /// @param[in] old_subblock_name - name of the old sub block
  /// @param[in] new_subblock_name - name of the new sub block
  virtual bool move_subblock( std::string const& old_subblock_name,
                              std::string const& new_subblock_name );

  /// \brief Does a given subblock exist in this config block?
  ///
  /// Does a subblock of the given name exist in this config block?
  ///
  /// @param[in] subblock_name - name of the sub block to test for
  virtual bool has_subblock( std::string const& subblock_name );

  /// \brief Remove a given subblock identified by name.
  ///
  /// This function will remove an entire config subblock.
  /// It returns true if anything was removed.
  ///
  /// @param[in] subblock_name - name of the sub block to remove
  virtual bool remove_subblock( std::string const& subblock_name );

  /// \brief Does the parameter \a name have a value set?
  ///
  /// This could be the default value or a user-specified value.
  /// @param[in] name - configuration item fully qualified name
  virtual bool has( std::string const& name ) const;

  /// \brief get has_user_value flag by parameter name
  virtual checked_bool get_has_user_value(
            std::string const& parameter_name, bool & has_user_value ) const;

  /// Set the checked bool value.
  virtual checked_bool set( std::string const& name, std::string const& val );

  /// Set the checked bool value.
  virtual checked_bool set( std::string const& name, config_block_value const& val );

  /// Get the checked bool value given the name and value.
  template<class T>
  checked_bool set( std::string const& name, T const& val );

  ///Return the checked bool value as a const variable given name and value.
  template<class T, unsigned N>
  checked_bool get( std::string const& name, T (&val)[N] ) const;

  /// Return the checked bool value as a const variable given name and value.
  template<class T>
  checked_bool get( std::string const& name, T* ptr, unsigned N ) const;


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
  /// @throw unchecked_return_value
  template<class T>
  config_block& set_value( std::string const& name, T const& val );


  /// "Critical" or "checked" retrieval.
  ///
  /// Tries to read the parameter named \a name as a value of type \c
  /// T. Throws an error on failure. Use like
  /// \code
  ///    get < double > ( "name" );
  /// \endcode
  /// @throw config_block_parse_error
  template<class T>
  T get( std::string const& name ) const;


  /** Get parameter from enumerated options.
   *
   * This method gets the specified string value and checks it against
   * the supplied table. The corresponding enum value is returned if
   * the string is found. An exception is thrown if not.
   *
   * You may have to do some type wrangling to get the correct return
   * type.
   *
   * @param[in] table - object defining conversion from string to enum.
   * @param[in] name - config item name to be converted.
   * @throw config_block_parse_error
   *
   * Example:
   * \code
  try
  {
    // Get option and validate against table
    typedef world_coord_super_process_impl< PixType > wcsp_impl_t;
    impl_->metadata_type = blk.get< typename wcsp_impl_t::mode >( metadata_mode, "metadata_type");
  }
  catch( config_block_parse_error const& e)
  {
      LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what());

      return false;
  }
  \endcode
   */
  template <class T>
  T get( config_enum_option const& table, std::string const& name ) const;


  ///Parse the given file.
  virtual checked_bool parse( std::string const& filename );

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
  virtual checked_bool parse_arguments( int argc, char** argv );

  ///Print to file.
  virtual void print( std::ostream& ostr ) const;

  /**
   * @brief Print config block with tree structure.
   *
   * This method formats the config block in a tree format suitable
   * for extraction and reuse in a config file.
   *
   * @param os Format config on this stream.
   * @param argv0 Name of the program running. This string is used in
   * the header of the file as part of the description. If the
   * application name is not known or not available, something
   * creative can be used instead.
   * @param githash The git hash of the build. This is used in the
   * header of the output file as part of the description. If the git
   * hash is not known do not supply any value and leave the default.
   */
  void print_as_tree( std::ostream& os, std::string const& argv0, std::string const& githash = "N/A" ) const;

  /**
   * @brief Print config in markdown format.
   *
   * @param os Format config on this stream.
   * @param argv0 Name of the program running. This string is used in
   * the header of the file as part of the description. If the
   * application name is not known or not available, something
   * creative can be used instead.
   */
  void print_as_markdown( std::ostream& os, std::string const& argv0 ) const;

  ///Set the fail flag on the missing block.
  virtual void set_fail_on_missing_block( bool flag );

  /** Map of strings to enumerate values. This method returns a map keyed
   * on fully qualified config entry name with the data being the
   * configured value.
   */
  virtual  std::map< std::string, config_block_value > enumerate_values() const;


protected:
  virtual void add( std::string const& name, config_block_value const& val );
  int get_enum_int( config_enum_option const& table, std::string const& name ) const;

  checked_bool get_raw_value( std::string const& name, std::string& val ) const;
  void check_if_exists( std::string const& name );

  typedef std::map< std::string, config_block_value > value_map_type;
  value_map_type vmap_;

  // Added this optional flag to suppress the set_param() failure because
  // config file contains processes that have not been added to the pipeline.
  bool fail_on_missing_block_;

  // the component check list.
  std::vector< std::string > check_list_;

};


/// Definition of configuration block parse error.
class config_block_parse_error
  : public std::runtime_error
{
public:
  /// Constructor: pass error message.
  config_block_parse_error( std::string const& msg )
    : std::runtime_error( msg )
  {
  }
};

} // end namespace vitk

#include "config_block.txx"

#endif // vidtk_config_block_h_
