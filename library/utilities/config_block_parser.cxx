/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/config_block_parser.h>
#include <utilities/log.h>
#include <vcl_cassert.h>
#include <vul/vul_file.h>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem/path.hpp>
#include <boost/tuple/tuple.hpp>

namespace {


/// Trim whitespace from the left and right of \a str.
void
trim_space( vcl_string& str )
{
  typedef vcl_string::size_type size_type;

  char const* whitespace = " \t\r\n";

  // Trim space from left side
  size_type lft = str.find_first_not_of( whitespace );
  if( lft == str.npos )
  {
    // the string is all spaces
    str.clear();
  }
  else
  {
    size_type rgt = str.find_last_not_of( whitespace );
    assert( rgt != str.npos );
    str.erase( rgt+1 );
    str.erase( 0, lft );
  }
}


} // end anonymous namespace


namespace vidtk
{


config_block_parser
::config_block_parser()
  : istr_( NULL )
{
}


// Stack element to keep track of active files. The second element
// indicates if we own the file or it was passed in. If we own it,
// then we must delete it.
typedef boost::tuple <vcl_istream *, vcl_string, bool> file_stack_item_t;
vcl_vector < file_stack_item_t > include_file_stack;


checked_bool
config_block_parser
::set_filename( vcl_string const& filename )
{
  filename_ = filename;
  line_number_ = 0;
  block_names_.clear();


  vcl_ifstream * fstr = new vcl_ifstream (filename_.c_str());
  if( ! *fstr )
  {
    log_error( "config_block_parser: failed to open \""
               << filename_ << "\" for reading\n" );
    return false;
  }
  else
  {
    istr_ = fstr;
    // indicate we own the file - will be deleted at EOF
    include_file_stack.push_back (file_stack_item_t(istr_, vul_file::dirname(filename_), true) );
    return true;
  }
}


void
config_block_parser
::set_input_stream( vcl_istream& istr )
{
  istr_ = &istr;
  // indicate we do not own the stream.
  include_file_stack.push_back (file_stack_item_t(istr_, vul_file::get_cwd(), false) );

}


// ----------------------------------------------------------------
/** Parse config file.
 *
 *
 */
checked_bool
config_block_parser
::parse( config_block& in_blk )
{

  assert( istr_ != NULL );

  config_file_dir_ = include_file_stack.back().get<1>();

  blk_ = &in_blk;

  update_block_name();

  try
  {
    vcl_string line;
    while( 1 )
    {
      if ( ! vcl_getline( *istr_, line ) ) // end of file
      {
        // Pop current file off of stack.
        file_stack_item_t fh = include_file_stack.back();
        if (fh.get<2>())
        {
          delete fh.get<0>();
        }

        include_file_stack.pop_back();

        // If there are no more elements on the stack, then this is
        // the end of the top level file - we are done.
        if (include_file_stack.empty() )
          {
            break;
          }

        // Resume reading where we left off on the previous file.
        istr_ = include_file_stack.back().get<0>();
        config_file_dir_ = include_file_stack.back().get<1>();
        continue; // restart while loop
      }

      ++line_number_;
      line_str_.str( line );
      line_str_.clear(); // clear old status flags

      trim_space( line );

      // Skip blank lines and comment lines.
      vcl_string::size_type first_char = line.find_first_not_of( " \t" );
      if( first_char == line.npos || line[first_char] == '#' )
      {
        continue;
      }

      vcl_string::size_type equal_pos = line.find_first_of( "=" );

      // The line has an equal sign, assume it is a key-value pair.
      // Otherwise, see if it can parsed as a control string.
      //
      if( equal_pos != line.npos )
      {
        vcl_string key = line.substr( 0, equal_pos );
        vcl_string value = line.substr( equal_pos+1 );

        trim_space( key );
        vcl_string::size_type space_pos = key.find_first_of( " \t" );
        if( space_pos == key.npos )
        {
          this->set( key, value );
        }
        else
        {
          vcl_string type = key.substr( 0, space_pos );
          key = key.substr( space_pos );
          trim_space( key );
          if( type == "relativepath" )
          {
            trim_space( value );
            this->set( key, config_file_dir_ + "/" + value );
          }
          else
          {
            log_error( "unknown type \"" << type << "\" when processing key \""
                       << key << "\"\n" );
            throw vcl_runtime_error( "failed to parse config file" );
          }
        }
      }
      else // handle control words
      {
        vcl_string control_word =  read_word( "control word" );
        if( control_word[0] == '#' )
        {
          // skip comments.
        }
        else if( control_word == "block" )
        {
          block_names_.push_back( read_word( "block name" ) );
          update_block_name();
        }
        else if( control_word == "endblock" )
        {
          if( block_names_.empty() )
          {
            log_error( "endblock without a \"block\"\n" );
            throw vcl_runtime_error( "failed to parse config file" );
          }
          block_names_.pop_back();
          update_block_name();
        }
        else if (control_word == "include")
        {
          vcl_string filename = read_rest_of_line( "include file name");
          if (!boost::filesystem::path( filename ).is_absolute() )
          {
            filename = config_file_dir_ + "/" + filename;
          }
          vcl_ifstream * inc_file = new vcl_ifstream( filename.c_str() );
          if ( ! *inc_file )
          {
            // error opening file - die a horrible death
            log_error ("Could not open file " << filename
                       << " specified in include control at line "
                       << line_number_ << vcl_endl );
            return false;
          }

          vcl_string dirpath = vul_file::dirname( filename );

          // push file onto stack, we own the file
          include_file_stack.push_back ( file_stack_item_t (inc_file, dirpath, true) );
          istr_ = inc_file; // set to use new include file
          config_file_dir_ = dirpath;
        }
        else
        {
          log_error( "unknown control word \"" << control_word << "\"\n" );
          throw vcl_runtime_error( "failed to parse config file" );
        }
      }
    } // end while

    if( ! block_names_.empty() )
    {
      log_error( "block " << block_names_.back() << " not ended\n" );
      return false;
    }
  }
  catch( vcl_runtime_error& err )
  {
    log_error( err.what() << vcl_endl );
    return checked_bool( err.what() );
  }

  // If we get here, then all was well.
  return true;
}


vcl_string
config_block_parser
::read_rest_of_line( char const* msg )
{
  vcl_string line;

  vcl_getline( line_str_, line );

  line.erase( 0, line.find_first_not_of( " \t" ) );

  if( line.empty() )
  {
    log_error( "Error parsing file " << filename_
               << ":" << line_number_ << ": couldn't read "
               << (msg ? msg : "rest of line") << "\n" );
    throw vcl_runtime_error( "failed to parse config file" );
  }

  return line;
}


// ----------------------------------------------------------------
/** Get next word from input line.
 *
 * The next word is retreived from the input line.
 *
 * @param[in] msg - message to issue if there is no more input.
 *
 * @return The next work in the input line.
 */
vcl_string
config_block_parser
::read_word( char const* msg )
{
  vcl_string word;

  if( ! ( line_str_ >> word ) )
  {
    log_error( "Error parsing file " << filename_
               << ":" << line_number_ << ": couldn't read "
               << (msg ? msg : "next word") << "\n" );
    throw vcl_runtime_error( "failed to parse config file" );
  }

  return word;
}


void
config_block_parser
::update_block_name()
{
  block_name_.clear();
  for( unsigned i = 0; i < block_names_.size(); ++i )
  {
    block_name_ += block_names_[i];
    block_name_ += ':';
  }
}


void
config_block_parser
::set( vcl_string key, vcl_string value )
{
  assert( blk_ != NULL );

  trim_space( key );
  trim_space( value );
  blk_->set( block_name_+key, value );
}


} // end namespace vidtk
