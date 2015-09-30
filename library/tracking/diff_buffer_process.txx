/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/diff_buffer_process.h>
#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <vcl_cassert.h>
#include <vcl_algorithm.h>

namespace vidtk
{

template <class PixType>
diff_buffer_process<PixType>
::diff_buffer_process( vcl_string const& name )
  : process( name, "diff_buffer_process" ),
    disable_capacity_error_( false )
{
  config_.add_parameter(
    "spacing",
    "2",
    "The spacing between the images to be differenced, in frames. "
    "This spacing should be such that the object moves more than twice "
    "its size between the current frame and the frame \"spacing\" units "
    "prior.  This will allow the \"ghost\" image to be removed."
  );
}


template <class PixType>
diff_buffer_process<PixType>
::~diff_buffer_process()
{
}


template <class PixType>
config_block
diff_buffer_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
diff_buffer_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "spacing", spacing_ );

    img_buffer_.set_capacity( spacing_ + 1 );
    homog_buffer_.set_capacity( spacing_ + 1 );
  }
  catch( unchecked_return_value& )
  {
    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
diff_buffer_process<PixType>
::initialize()
{
  img_buffer_.clear();
  homog_buffer_.clear();
  return true;
}


template <class PixType>
bool
diff_buffer_process<PixType>
::reset()
{
  return this->initialize();
}

template <class PixType>
bool
diff_buffer_process<PixType>
::step()
{
  // Check to make sure buffers are non-empty
  if( homog_buffer_.size() == 0 || img_buffer_.size() == 0 ) {
    log_error( "Input Buffer is Empty" );
    return false;
  }

  // Formulate desired outputs
  current_end_idx_ = vcl_min( img_buffer_.size()-1, spacing_ );
  first_to_ref_ = homog_buffer_.datum_at( current_end_idx_ );
  second_to_ref_ = homog_buffer_.datum_at( current_end_idx_ / 2 );
  third_to_ref_ = homog_buffer_.datum_at( 0 );
  third_to_first_ = first_to_ref_.get_inverse() * third_to_ref_;
  third_to_second_ = second_to_ref_.get_inverse() * third_to_ref_;
  return true;
}

// I/O Functions

template <class PixType>
void
diff_buffer_process<PixType>
::add_source_img( vil_image_view<PixType> const& item )
{
  // Make sure image is non-empty
  if( !item ) {
    log_error( "Input Image to Buffer is Empty" );
  }

  // Add image view to internal buffer
  img_buffer_.insert( item );
}

template <class PixType>
void
diff_buffer_process<PixType>
::add_source_homog( vgl_h_matrix_2d<double> const& item )
{
  homog_buffer_.insert( item );
}

template <class PixType>
vil_image_view<PixType> const&
diff_buffer_process<PixType>
::get_first_image() const
{
  return img_buffer_.datum_at(current_end_idx_);
}

template <class PixType>
vil_image_view<PixType> const&
diff_buffer_process<PixType>
::get_second_image() const
{
  return img_buffer_.datum_at(current_end_idx_/2);
}

template <class PixType>
vil_image_view<PixType> const&
diff_buffer_process<PixType>
::get_third_image() const
{
  return img_buffer_.datum_at(0);
}

template <class PixType>
vgl_h_matrix_2d<double> const&
diff_buffer_process<PixType>
::get_third_to_first_homog() const
{
  return third_to_first_;
}

template <class PixType>
vgl_h_matrix_2d<double> const&
diff_buffer_process<PixType>
::get_third_to_second_homog() const
{
  return third_to_second_;
}

} // end namespace vidtk
