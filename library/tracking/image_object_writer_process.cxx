/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_writer_process.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <vul/vul_file.h>
#include <utilities/vsl/timestamp_io.h>
#include <tracking/vsl/image_object_io.h>

namespace vidtk
{


image_object_writer_process
::image_object_writer_process( vcl_string const& name )
  : process( name, "image_object_writer_process" ),
    disabled_( true ),
    format_( 0 ),
    allow_overwrite_( false ),
    src_objs_( NULL ),
    bfstr_( NULL )
{
  config_.add( "disabled", "true" );
  config_.add( "format", "0" );
  config_.add( "filename", "" );
  config_.add( "overwrite_existing", "false" );
}


image_object_writer_process
::~image_object_writer_process()
{
  delete bfstr_;
}


config_block
image_object_writer_process
::params() const
{
  return config_;
}


bool
image_object_writer_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    vcl_string fmt_str;
    blk.get( "format", fmt_str );
    blk.get( "filename", filename_ );
    blk.get( "overwrite_existing", allow_overwrite_ );

    format_ = unsigned(-1);
    if( fmt_str == "0" )
    {
      format_ = 0;
    }
    else if( fmt_str == "vsl" )
    {
      format_ = 1;
    }
    else
    {
      log_error( name() << ": unknown format " << fmt_str << "\n" );
    }

    // Validate the format
    if( format_ > 1 )
    {
      throw unchecked_return_value("");
    }
  }
  catch( unchecked_return_value& )
  {
    // Reset to old values
    this->set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
image_object_writer_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  if( format_ == 0 )
  {
    if( fail_if_exists( filename_ ) )
    {
      return false;
    }

    fstr_.open( filename_.c_str() );
    if( ! fstr_ )
    {
      log_error( name() << "Couldn't open "
                 << filename_ << " for writing.\n" );
      return false;
    }

    fstr_ << "# 1:Frame-number  2:Time  3-5:World-loc(x,y,z)  6-7:Image-loc(i,j)  8:Area   9-12:Img-bbox(i0,j0,i1,j1)\n";
  }
  else if( format_ == 1 )
  {
    if( fail_if_exists( filename_ ) )
    {
      return false;
    }

    bfstr_ = new vsl_b_ofstream( filename_ );
    if( ! *bfstr_ )
    {
      log_error( name() << "Couldn't open "
                 << filename_ << " for writing.\n" );
      delete bfstr_;
      bfstr_ = NULL;
      return false;
    }

    vsl_b_write( *bfstr_, vcl_string("image_object_vsl_stream") );
    vsl_b_write( *bfstr_, 1 ); // version number
  }

  return true;
}


bool
image_object_writer_process
::step()
{
  if( disabled_ )
  {
    return false;
  }

  log_assert( src_objs_ != NULL, "Source not specified" );

  switch( format_ )
  {
  case 0:   write_format_0(); break;
  case 1:   write_format_vsl(); break;
  default:  ;
  }

  src_objs_ = NULL;
  ts_ = timestamp();

  return true;
}


void
image_object_writer_process
::set_image_objects( vcl_vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}


void
image_object_writer_process
::set_timestamp( timestamp const& ts )
{
  ts_ = ts;
}


bool
image_object_writer_process
::is_disabled() const
{
  return disabled_;
}


/// \internal
///
/// Returns \c true if the file \a filename exists and we are not
/// allowed to overwrite it (governed by allow_overwrite_).
bool
image_object_writer_process
::fail_if_exists( vcl_string const& filename )
{
  return ( ! allow_overwrite_ ) && vul_file::exists( filename );
}


/// Format 0.
///
/// The first line is a header, and begins with "#".  Each subsequent
/// line has 12 columns containing the following data:
///
/// \li Column 1: Frame number (-1 if not available)
/// \li Column 2: Time (-1 if not available)
/// \li Columns 3-5: World location (x,y,z)
/// \li Columns 6-7: Image location (i,j)
/// \li Column 8: Area
/// \li Columns 9-12: Bounding box in image coordinates (i0,j0,i1,j1)
///
/// See image_object for more details about these values.
void
image_object_writer_process
::write_format_0()
{
  // if you change this format, change the documentation above and change the header output in initialize();

  unsigned N = src_objs_->size();
  for( unsigned i = 0; i < N; ++i )
  {
    image_object const& o = *(*src_objs_)[i];
    if( ts_.has_frame_number() )
    {
      fstr_ << ts_.frame_number() << " ";
    }
    else
    {
      fstr_ << "-1 ";
    }
    if( ts_.has_time() )
    {
      fstr_ << ts_.time() << " ";
    }
    else
    {
      fstr_ << "-1 ";
    }
    fstr_ << o.world_loc_ << " "
          << o.img_loc_ << " "
          << o.area_ << " "
          << o.bbox_.min_x() << " "
          << o.bbox_.min_y() << " "
          << o.bbox_.max_x() << " "
          << o.bbox_.max_y() << "\n";
  }
  fstr_.flush();
}


/// This is a binary format using the vsl library.
///
void
image_object_writer_process
::write_format_vsl()
{
  // each step() should result in an independent set of objects.
  bfstr_->clear_serialisation_records();

  vsl_b_write( *bfstr_, ts_ );
  vsl_b_write( *bfstr_, *src_objs_ );
}


} // end namespace vidtk
