/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_config_block_parser_h_
#define vidtk_config_block_parser_h_

#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_fstream.h>
#include <vcl_sstream.h>

#include <utilities/config_block.h>
#include <utilities/checked_bool.h>

namespace vidtk
{

class config_block_parser
{
public:
  config_block_parser();

  checked_bool set_filename( vcl_string const& filename );

  void set_input_stream( vcl_istream& istr );

  checked_bool parse( config_block& blk );

private:
  /// \brief Read the next word from the current line.
  ///
  /// \a msg is a message indicating the what kind of word is
  /// expected, and is only used to generate an error message if the
  /// read fails.
  vcl_string read_word( char const* msg = 0 );
  /// \brief Read the rest of the current line.
  ///
  /// \a msg is a message indicating the what kind of line is
  /// expected, and is only used to generate an error message if the
  /// read fails.
  vcl_string read_rest_of_line( char const* msg = 0 );
  void update_block_name();
  void set( vcl_string key, vcl_string value );

private:
  config_block* blk_;

  /// The input stream.  May point to fstr_ if reading from a file.
  vcl_istream* istr_;
  vcl_istringstream line_str_;
  vcl_string filename_;
  vcl_string config_file_dir_;
  unsigned line_number_;
  /// Names of the current blocks (parameter name prefixes)
  vcl_vector< vcl_string > block_names_;
  /// Cached concatation of the block names.
  vcl_string block_name_;

};


} // end namespace vidtk


#endif // vidtk_config_block_parser_h_
