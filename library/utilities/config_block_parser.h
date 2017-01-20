/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_config_block_parser_h_
#define vidtk_config_block_parser_h_

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <boost/tuple/tuple.hpp>

#include <utilities/config_block.h>
#include <utilities/checked_bool.h>

namespace vidtk
{

// ----------------------------------------------------------------
/**
 * @brief Parse config file into config block.
 *
 * This class processes a config file and populates a config block. A
 * config source must be specified before it is parsed. Once a config
 * is parsed, this object should be deleted since it is not reusable.
 */
class config_block_parser
{
public:
  config_block_parser();
  ~config_block_parser();

  /**
   * @brief Specify config file name
   *
   * This method specifies the config file to read. The file is opened
   * as a way of checking that it exists. This method of the
   * set_input_stream() method must be called before calling parse.
   *
   * @param filename Name of file to read
   *
   * @return \b true if file can be opened.
   */
  checked_bool set_filename( std::string const& filename );

  /**
   * @brief Specify config stream to parse.
   *
   * @param istr Stream to read config from.
   */
  void set_input_stream( std::istream& istr );

  /**
   * @brief Parse config data into config_block.
   *
   * @param blk Config block to create.
   *
   * @return \b true is config parsed correctly, otherwise there was
   * an error
   */
  checked_bool parse( config_block& blk );

private:
  /// @brief Read the next word from the current line.
  ///
  /// @param msg is a message indicating the what kind of word is
  /// expected, and is only used to generate an error message if the
  /// read fails.
  std::string read_word( char const* msg = 0 );

  /// @brief Read the rest of the current line.
  ///
  /// @param msg is a message indicating the what kind of line is
  /// expected, and is only used to generate an error message if the
  /// read fails.
  std::string read_rest_of_line( char const* msg = 0 );

  void update_block_name();
  void set( std::string key, std::string value );

  config_block* blk_;

  // Stack element to keep track of active files. The third element
  // indicates if we own the file or it was passed in. If we own it,
  // then we must delete it. The fourth item saves previous line count.
  // The 5th element is the file name with directory
  typedef boost::tuple <std::istream *, std::string, bool, int, std::string> file_stack_item_t;
  std::vector < file_stack_item_t > include_file_stack_;

  /// The input stream.  May point to fstr_ if reading from a file.
  std::istream* istr_;
  std::istringstream line_str_;
  std::string filename_;
  std::string config_file_dir_;
  unsigned line_number_;
  /// Names of the current blocks (parameter name prefixes)
  std::vector< std::string > block_names_;
  /// Cached concatation of the block names.
  std::string block_name_;

  bool parse_error_found_;
};

} // end namespace vidtk

#endif // vidtk_config_block_parser_h_
