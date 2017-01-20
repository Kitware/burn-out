/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/config_block_parser.h>

#include <cassert>
#include <vul/vul_file.h>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/filesystem/path.hpp>

#include <logger/logger.h>
VIDTK_LOGGER("config_block_parser");


namespace {


/// Trim whitespace from the left and right of \a str.
void
trim_space( std::string& str )
{
  typedef std::string::size_type size_type;

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
  : istr_( NULL ),
    parse_error_found_( false )
{
}

config_block_parser
::~config_block_parser()
{
  if( include_file_stack_.size() != 0 )
  {
    LOG_ERROR("FILE STACK IS NOT EMPTY: this would mean some files where not properly parsed");
    for( unsigned int i = 0; i < include_file_stack_.size(); ++i )
    {
      file_stack_item_t fh = include_file_stack_[i];
      if (fh.get<2>())
      {
        delete fh.get<0>();
      }
    }
  }
}


// ------------------------------------------------------------------
// Sets the initial file name for parsing a config.
checked_bool
config_block_parser
::set_filename( std::string const& filename )
{
  filename_ = filename;
  line_number_ = 0;
  block_names_.clear();


  std::ifstream * fstr = new std::ifstream (filename_.c_str());
  if( ! *fstr )
  {
    LOG_ERROR( "config_block_parser: failed to open \""
               << filename_ << "\" for reading" );
    delete fstr;
    return false;
  }
  else
  {
    istr_ = fstr;
    // indicate we own the file - will be deleted at EOF
    // The stack top is our file context
    include_file_stack_.push_back (file_stack_item_t(istr_, vul_file::dirname(filename_), true, 0, filename) );
    return true;
  }
}


// ------------------------------------------------------------------
// Set input stream to parse
void
config_block_parser
::set_input_stream( std::istream& istr )
{
  istr_ = &istr;
  // indicate we do not own the stream.
  include_file_stack_.push_back (file_stack_item_t(istr_, vul_file::get_cwd(), false, 0, "??") );
}


// ----------------------------------------------------------------
/** Parse config file.
 *
 * set_filename() or set_input_stream() must have been previously
 * called to set up the initial file stream.
 */
checked_bool
config_block_parser
::parse( config_block& in_blk )
{

  assert( istr_ != NULL );

  config_file_dir_ = include_file_stack_.back().get<1>();

  blk_ = &in_blk;

  update_block_name();

  try
  {
    std::string line;
    while( 1 )
    {
      if ( ! std::getline( *istr_, line ) ) // end of file
      {
        // Pop current file off of stack.
        file_stack_item_t fh = include_file_stack_.back();
        if (fh.get<2>())
        {
          delete fh.get<0>();
        }

        // Get rid of old file entry
        include_file_stack_.pop_back();

        // If there are no more elements on the stack, then this is
        // the end of the top level file - we are done.
        if (include_file_stack_.empty() )
          {
            break;
          }

        // Resume reading where we left off on the previous file.
        istr_ = include_file_stack_.back().get<0>();
        config_file_dir_ = include_file_stack_.back().get<1>();
        line_number_ = include_file_stack_.back().get<3>();
        filename_ = include_file_stack_.back().get<4>();
        continue; // restart while loop
      }

      ++line_number_;
      line_str_.str( line );
      line_str_.clear(); // clear old status flags

      trim_space( line );

      // Skip blank lines and comment lines.
      std::string::size_type first_char = line.find_first_not_of( " \t" );
      if( first_char == line.npos || line[first_char] == '#' )
      {
        continue;
      }

      std::string::size_type equal_pos = line.find_first_of( "=" );

      // The line has an equal sign, assume it is a key-value pair.
      // Otherwise, see if it can parsed as a control string.
      //
      if( equal_pos != line.npos )
      {
        std::string key = line.substr( 0, equal_pos );
        std::string value = line.substr( equal_pos+1 );

        trim_space( key );
        std::string::size_type space_pos = key.find_first_of( " \t" );
        if( space_pos == key.npos )
        {
          this->set( key, value );
        }
        else
        {
          std::string type = key.substr( 0, space_pos );
          key = key.substr( space_pos );
          trim_space( key );
          if( type == "relativepath" )
          {
            trim_space( value );
            this->set( key, config_file_dir_ + "/" + value );
          }
          else
          {
            LOG_ERROR( "unknown type \"" << type << "\" when processing key \""
                       << key << "\"" );

            this->parse_error_found_ = true;
            continue;
          }
        }
      }
      else // handle control words
      {
        std::string control_word =  read_word( "control word" );
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
            LOG_ERROR( "endblock without a \"block\"" );

            this->parse_error_found_ = true;
            continue;
          }
          block_names_.pop_back();
          update_block_name();
        }
        else if (control_word == "include")
        {
          std::string filename = read_rest_of_line( "include file name");
          if (!boost::filesystem::path( filename ).is_absolute() )
          {
            filename = config_file_dir_ + "/" + filename;
          }
          std::ifstream * inc_file = new std::ifstream( filename.c_str() );
          if ( ! *inc_file )
          {
            // error opening file - die a horrible death
            LOG_ERROR ("Could not open file " << filename
                       << " specified in include control at line "
                       << line_number_ );
            delete inc_file;

            this->parse_error_found_ = true;
            continue;
          }

          std::string dirpath = vul_file::dirname( filename );

          // Save current line number on our stack entry
          include_file_stack_.back().get<3>() = line_number_;

          // push file onto stack, we own the file
          include_file_stack_.push_back ( file_stack_item_t (inc_file, dirpath, true, 0, filename ));
          istr_ = inc_file; // set to use new include file
          config_file_dir_ = dirpath;
          line_number_ = 0;
          filename_ = filename;
        }
        else
        {
          LOG_ERROR( "Unknown control word \"" << control_word << "\"" );

          this->parse_error_found_ = true;
          continue;
        }
      }
    } // end while

    if( ! block_names_.empty() )
    {
      LOG_ERROR( "block " << block_names_.back() << " not ended" );
      return false;
    }
  }
  catch( std::runtime_error& err )
  {
    LOG_ERROR( err.what() );
    return checked_bool( err.what() );
  }

  // If we get here, report error status.
  if ( this->parse_error_found_ )
  {
    return checked_bool( "Error parsing config" );
  }
  return true;
}


// ------------------------------------------------------------------
std::string
config_block_parser
::read_rest_of_line( char const* msg )
{
  std::string line;

  std::getline( line_str_, line );

  line.erase( 0, line.find_first_not_of( " \t" ) );

  if( line.empty() )
  {
    LOG_ERROR( "Error parsing file " << filename_
               << ":" << line_number_ << ": couldn't read "
               << (msg ? msg : "rest of line") );
    throw std::runtime_error( "failed to parse config file" );
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
std::string
config_block_parser
::read_word( char const* msg )
{
  std::string word;

  if( ! ( line_str_ >> word ) )
  {
    LOG_ERROR( "Error parsing file " << filename_
               << ":" << line_number_ << ": couldn't read "
               << (msg ? msg : "next word") );
    throw std::runtime_error( "failed to parse config file" );
  }

  return word;
}


// ------------------------------------------------------------------
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


// ------------------------------------------------------------------
void
config_block_parser
::set( std::string key, std::string value )
{
  assert( blk_ != NULL );

  trim_space( key );
  trim_space( value );
  checked_bool status = blk_->set( block_name_ + key, value );
  // test for set failure and report source file name and line number
  if (! status )
  {
    std::stringstream msg;

    LOG_ERROR( "Error parsing file "<< filename_
               <<  ":" << line_number_ << " - "
               << status.message() );

    this->parse_error_found_ = true;
  }
}

} // end namespace vidtk
