/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_objects_buffer_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{

class frame_objects_buffer_process_impl
{
public:
  //Member functions
  frame_objects_buffer_process_impl( frame_objects_buffer_process * b_ptr )
    : base_process_( b_ptr ),
      curr_ts_( NULL ),
      curr_objs_( NULL ),
      max_length_( 0 )
  {
  } // frame_objects_buffer_process_impl()

  void add_new_data()
  {
    if( data_.size() >= max_length_ )
    {
#ifdef PRINT_DEBUG_INFO
      log_debug( base_process_->name() << ": Erasing "<< data_.begin()->second.size()
        <<" objects on " << data_.begin()->first << " \n" );
#endif
      // This is only for the sake of effeciency.
      data_.erase( data_.begin() );
    }

    log_assert( data_.size() <= max_length_,
      base_process_->name() << ": Buffer overflow!\n" );

#ifdef PRINT_DEBUG_INFO
      log_debug( base_process_->name() << ": Adding "<< curr_objs_->size()
        <<" objects on " << *curr_ts_ << " \n" );
#endif
    data_[ *curr_ts_ ] = *curr_objs_;
  }

  void replace_data()
  {
    if( updated_data_items_.empty() )
      return;

    log_assert( updated_data_items_.find( *curr_ts_ ) == updated_data_items_.end(),
      base_process_->name() << ": Did not expect to see current timestamp in"
      " the updated data from last frame.\n" );

    timestamp const min_ts = data_.begin()->first;
    for( frame_objs_type::const_reverse_iterator i = updated_data_items_.rbegin();
         i != updated_data_items_.rend() &&  min_ts <= i->first ; ++i )
    {
      data_[ i->first ] = i->second;
    }
  }

  // Member variables
  frame_objects_buffer_process * base_process_;

  // I/O data
  timestamp const * curr_ts_;
  objs_type const * curr_objs_;
  frame_objs_type updated_data_items_;

  // Configuration parameters
  config_block config_;
  unsigned max_length_;

  frame_objs_type data_;

}; // class frame_objects_buffer_process_impl

/// ----------------------------------------------------------------------
///   frame_objects_buffer_process function definitions

frame_objects_buffer_process
::frame_objects_buffer_process( vcl_string const& name )
  : process( name, "frame_objects_buffer_process" ),
    impl_( NULL )
{
  impl_ = new frame_objects_buffer_process_impl( this );

  impl_->config_.add_parameter( "length",
    "0",
    "Maximum number of frames to store the data for. This is similar to "
    "the ring buffer length." );
}


frame_objects_buffer_process
::~frame_objects_buffer_process()
{
  delete impl_;
}


config_block
frame_objects_buffer_process
::params() const
{
  return impl_->config_;
}


bool
frame_objects_buffer_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "length", impl_->max_length_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed because "<< e.what() <<"\n" );
    return false;
  }

  impl_->config_.update( blk );
  return true;
}


process::step_status
frame_objects_buffer_process
::step2()
{
  if( !impl_->curr_ts_ || !impl_->curr_objs_ )
  {
    log_error( this->name() << ": Input(s) not available.\n" );
    impl_->updated_data_items_.clear();
    return FAILURE;
  }

  impl_->add_new_data();
  impl_->replace_data();

  // End of step resets
  impl_->curr_ts_ = NULL;
  impl_->curr_objs_ = NULL;
  impl_->updated_data_items_.clear();

  return SUCCESS;
}

//----------------------------- I/O Ports --------------------------------

void
frame_objects_buffer_process
::set_timestamp( timestamp const & ts )
{
  impl_->curr_ts_ = &ts;
}

void
frame_objects_buffer_process
::set_current_objects( objs_type const& objs )
{
  impl_->curr_objs_ = &objs;
}

void
frame_objects_buffer_process
::set_updated_frame_objects( frame_objs_type const& frame_objs )
{
  impl_->updated_data_items_.insert( frame_objs.begin(), frame_objs.end() );
}

frame_objs_type const&
frame_objects_buffer_process
::frame_objects() const
{
  return impl_->data_;
}

} // end namespace vidtk
