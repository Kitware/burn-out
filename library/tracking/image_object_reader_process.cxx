/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_reader_process.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.hxx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.hxx>
#include <utilities/vsl/timestamp_io.h>
#include <tracking/vsl/image_object_io.h>

namespace vidtk
{


image_object_reader_process
::image_object_reader_process( vcl_string const& name )
  : process( name, "image_object_reader_process" ),
    disabled_( false ),
    format_( unsigned(-1) ),
    inp_ts_( NULL ),
    bfstr_( NULL ),
    b_new_frame_started( false ),
    last_frame_num_( 0 )
{
  config_.add( "disabled", "false" );
  config_.add( "format", "vsl" );
  config_.add( "filename", "" );
}


image_object_reader_process
::~image_object_reader_process()
{
  delete bfstr_;
}


config_block
image_object_reader_process
::params() const
{
  return config_;
}


bool
image_object_reader_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    vcl_string fmt_str;
    blk.get( "format", fmt_str );
    blk.get( "filename", filename_ );

    format_ = unsigned(-1);
    if( fmt_str == "vsl" )
    {
      format_ = 1;
    }
    else if( fmt_str == "0")
    {
      format_ = 0;
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
image_object_reader_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  if( format_ == 1 )
  {
    bfstr_ = new vsl_b_ifstream( filename_ );
    if( ! *bfstr_ )
    {
      log_error( name() << "Couldn't open "
                 << filename_ << " for reading.\n" );
      delete bfstr_;
      bfstr_ = NULL;
      return false;
    }

    vcl_string header;
    vsl_b_read( *bfstr_, header );
    if( header != "image_object_vsl_stream" )
    {
      log_error( name() << ": incorrect header. Are you sure this is an image object vsl stream?\n" );
      return false;
    }
    int version;
    vsl_b_read( *bfstr_, version );
    if( version != 1 )
    {
      log_error( name() << ": unknown format version " << version << "\n" );
      return false;
    }
  }
  else if( format_ == 0 )
  {
    txtfstr_.open( filename_.c_str() );
    if( !txtfstr_ )
    {
//      log_error( "Could not open \"" << txtfstr_
//:        << "\" for reading; check config file \n" );
      return false;
    }
    
    // TODO: add header support.
  }

  return true;
}


bool
image_object_reader_process
::step()
{
  if( disabled_ )
  {
    return true;
  }

  objs_.clear();

  bool good = false;
  switch( format_ )
  {
  case 0:   good = read_format_0(); break;
  case 1:   good = read_format_vsl(); break;
  default:  ;
  }

  inp_ts_ = NULL;

  return good;
}


void
image_object_reader_process
::set_timestamp( vidtk::timestamp const& ts )
{
  inp_ts_ = &ts;
}


vidtk::timestamp const&
image_object_reader_process
::timestamp() const
{
  return read_ts_;
}


vcl_vector< image_object_sptr > const&
image_object_reader_process
::objects() const
{
  return objs_;
}


bool
image_object_reader_process
::is_disabled() const
{
  return disabled_;
}


/// This is a binary format using the vsl library.
///
bool
image_object_reader_process
::read_format_vsl()
{
  bfstr_->clear_serialisation_records();

  vsl_b_read( *bfstr_, read_ts_ );
  vsl_b_read( *bfstr_, objs_ );

  // return true if the stream is still good
  return bfstr_->is().good();
}


// Currently using -1 as "No objects detected in the current timestamp". 
bool
image_object_reader_process
::read_format_0()
{
  if( txtfstr_.eof() )
  {
    return true;
  }
  unsigned frame_num;
  double first_time;

  //for all the valid objects in this frame.
  while( !txtfstr_.eof() )
  {
    //1. Read the next timestamp.
    
    if( !b_new_frame_started )
    {
      txtfstr_ >> frame_num;

      if(frame_num > last_frame_num_)
      {
        //New frame started here.
        last_frame_num_ = frame_num;
        b_new_frame_started = true;
        break;
      }
    }
    else
    {
      frame_num = last_frame_num_;
      b_new_frame_started = false;
    }

    txtfstr_ >> first_time;

    //TODO: see if this needs to be integerated 'properly'
    if( inp_ts_->frame_number() != frame_num )
    {
      log_error(" The frame numbers mismatch between frame "<<inp_ts_->frame_number()<<
        " and "<< frame_num<<"\n" );

      return false;
    }

    //Update the timestamp.
    read_ts_.set_frame_number( frame_num );
    if( first_time != 0 ) //is valid
    {
      read_ts_.set_time( first_time );
    }

    //2. Read all the (valid) objects with next timestamp.
    //   -1 being invalid values. 

    image_object_sptr obj = new image_object();

    //World loc (x,y,z)
    txtfstr_ >> obj->world_loc_[0];
    txtfstr_ >> obj->world_loc_[1];
    txtfstr_ >> obj->world_loc_[2];


    //Img loc(x,y)
    txtfstr_ >> obj->img_loc_[0];
    txtfstr_ >> obj->img_loc_[1];
    
    //Area 
    txtfstr_ >> obj->area_;
  
    unsigned tmp;
    
    //Bbox (l,t,r,b)
    txtfstr_ >> tmp;
    obj->bbox_.set_min_x( tmp );
    txtfstr_ >> tmp;
    obj->bbox_.set_min_y( tmp );
    txtfstr_ >> tmp;
    obj->bbox_.set_max_x( tmp );
    txtfstr_ >> tmp;
    obj->bbox_.set_max_y( tmp );

    if( obj->img_loc_[0] != -1.0 )
    {
      //Only pass on the object if it is 'valid'
      objs_.push_back( obj );
    }
  }

  vcl_cout<<"Read "<<objs_.size()<<" object(s) from file."<<vcl_endl;


  return true;
}


} // end namespace vidtk
