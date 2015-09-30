/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "format_block.h"

namespace vidtk
{


vcl_string
format_block( vcl_string const& content,
              vcl_string const& prefix,
              unsigned line_length )
{
  // // The prefix is counted in the line_length, so subtract that out to
  // // get the line length we have to work with.
  // if( prefix.length() < line_length )
  // {
  //   line_length -= prefix.length();
  // }
  // else
  // {
  //   line_length = 0;
  // }

  vcl_string output_block;

  vcl_string::const_iterator cur_pos = content.begin();

  while( cur_pos != content.end() )
  {
    // We are starting a new line in the source string.  First
    // determine the extra "prefix" added by the content providing
    // using explicit indentation.
    vcl_string current_prefix = prefix;
    while( cur_pos != content.end() && *cur_pos == ' ' )
    {
      current_prefix += *cur_pos;
      ++cur_pos;
    }

    // Now parse all the words in this source line

    // The current output line
    vcl_string cur_line = current_prefix;
    bool at_beginning_of_line = true;
    while( cur_pos != content.end() && *cur_pos != '\n' )
    {
      // Find the next word
      vcl_string word;
      while( cur_pos != content.end() &&
             *cur_pos != ' ' &&
             *cur_pos != '\n' )
      {
        word += *cur_pos;
        ++cur_pos;
      }
      // Skip subsequent spaces
      while( cur_pos != content.end() && *cur_pos == ' ' )
      {
        ++cur_pos;
      }

      if( at_beginning_of_line )
      {
        cur_line += word;
        at_beginning_of_line = false;
      }
      else
      {
        if( cur_line.length() + word.length() + 1 <= line_length )
        {
          cur_line += ' ';
          cur_line += word;
        }
        else
        {
          output_block += cur_line;
          output_block += "\n";
          cur_line = current_prefix;
          cur_line += word;
        }
      }
    }
    output_block += cur_line;
    output_block += "\n";
    if( cur_pos != content.end() )
    {
      ++cur_pos;
    }
  }

  return output_block;
}


} // end namespace vidtk
