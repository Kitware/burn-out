/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor_writer_process.h"

#include <tracking_data/io/raw_descriptor_writer.h>

#include <utilities/config_block.h>

#include <vul/vul_sprintf.h>
#include <vul/vul_file.h>

#include <sstream>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "raw_descriptor_writer_process" );

// Local implementation class
class raw_descriptor_writer_process::impl
{
public:
  impl()
    : opt_disabled( true ),
      opt_overwrite_file( true )
  {}

  ~impl() {}

  raw_descriptor_writer descriptor_writer;

  // Is this process disabled?
  bool opt_disabled;

  // Descriptor writer settings
  std::string opt_filename;
  bool opt_overwrite_file;

  std::ofstream file_stream;;

};


// ----------------------------------------------------------------
// Process Definition
raw_descriptor_writer_process
::raw_descriptor_writer_process( std::string const& _name )
  : process( _name, "raw_descriptor_writer_process" ),
    impl_( new raw_descriptor_writer_process::impl )
{
  std::stringstream format_help;

  format_help << "Available writer formats are:\n"
              << impl_->descriptor_writer.get_writer_formats();

  config_.add_parameter( "format", "csv", format_help.str() );

  config_.add_parameter( "disabled",
                         "false",
                         "Completely disable this process and pass no outputs." );

  config_.add_parameter( "filename",
                         "descriptor_output_data.dat",
                         "Output filename to write descriptor data to." );

  config_.add_parameter( "overwrite",
                         "true",
                         "Should we automatically overwrite existing files." );
}


raw_descriptor_writer_process
::~raw_descriptor_writer_process()
{
  if( !impl_->opt_disabled )
  {
    impl_->descriptor_writer.finalize();
  }

  impl_->file_stream.close();
}


// ----------------------------------------------------------------
config_block raw_descriptor_writer_process
::params() const
{
  return config_;
}


// ----------------------------------------------------------------
bool raw_descriptor_writer_process
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    impl_->opt_disabled = blk.get< bool > ( "disabled" );

    if( impl_->opt_disabled )
    {
      return true;
    }

    std::string fmt_str = blk.get< std::string >( "format" );

    impl_->opt_filename = blk.get< std::string >( "filename" );
    impl_->opt_overwrite_file = blk.get< bool >( "overwrite" );

    if( !impl_->descriptor_writer.set_format( fmt_str ) )
    {
      throw config_block_parse_error( ": unknown format" );
    }
    // impl_->descriptor_writer.set_options( options );

  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  return true;
} // raw_descriptor_writer_process::set_params


// ----------------------------------------------------------------
bool raw_descriptor_writer_process
::initialize()
{
  if( impl_->opt_disabled )
  {
    return true;
  }

  //
  // All file level processing is handled here since the writer only
  // wants an output stream.
  //
  if( ( !impl_->opt_overwrite_file ) && vul_file::exists( impl_->opt_filename ) )
  {
    LOG_ERROR( name() << ": File: \"" << impl_->opt_filename
               << "\" already exists. To overwrite set overwrite_existing flag" );
    return false;
  }

  impl_->file_stream.open( impl_->opt_filename.c_str() );

  if( !impl_->file_stream )
  {
    LOG_ERROR( "Unable to open output file \""
               << impl_->opt_filename << "\"for writing" );
    return false;
  }

  if( !impl_->descriptor_writer.initialize( &impl_->file_stream ) )
  {
    LOG_ERROR( "Failed to open/find directory" );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool raw_descriptor_writer_process
::step()
{
  if( impl_->opt_disabled )
  {
    return true;
  }

  return impl_->descriptor_writer.write( inputs_ );
} // raw_descriptor_writer_process::step


// ----------------------------------------------------------------
// -- input methods --
void raw_descriptor_writer_process
::set_descriptors( raw_descriptor::vector_t const& desc_list )
{
  inputs_ = desc_list;
}


} // end namespace vidtk
