/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_difference_process.h"

#include <vcl_cmath.h>
#include <utilities/log.h>

namespace
{

template < class PixType >
void
compute_diff_image( double threshold,
		    vil_image_view<PixType> const& image_1,
		    vil_image_view<PixType> const& image_2,
                    vil_image_view<bool>& ret )
{
  unsigned const ni = image_1.ni();
  unsigned const nj = image_1.nj();
  unsigned const np = image_1.nplanes();

  log_assert( (image_2.ni() == ni) && (image_2.nj() == nj) &&
	      (image_2.nplanes() == np),
	      "image_difference_process requires constant size frames" );

  ret.set_size( ni, nj );
  for ( unsigned i=0; i<ni; ++i )
  {
    for ( unsigned j=0; j<nj; ++j )
    {
      // should generalize to plug-in difference functions
      double sum_1=0.0, sum_2=0.0;
      for ( unsigned p=0; p<np; ++p )
      {
	sum_1 += image_1( i, j, p );
	sum_2 += image_2( i, j, p );
      }
      sum_1 /= np;
      sum_2 /= np;
      double diff = vcl_fabs( sum_1 - sum_2 );

      ret( i, j ) = (diff >= threshold);
    }
  }
}

} // anonymous namespace




namespace vidtk
{

template < class PixType >
image_difference_process<PixType>
::image_difference_process( vcl_string const& name )
  : fg_image_process<PixType>( name, "image_difference_process" ),
    param_( new image_difference_process_params )
{
  config_.add( "image_spacing", "20" );
  config_.add( "diff_threshold", "128" );
  config_.add( "shadow_removal_with_second_diff", "true" );
}

template < class PixType >
void
image_difference_process<PixType>
::clear_buffer()
{
  for (unsigned i=0; i<buffer_.size(); i++)
  {
    delete buffer_[i];
  }
  buffer_.clear();
}

template < class PixType >
image_difference_process<PixType>
::~image_difference_process()
{
  clear_buffer();
  delete param_;
}


template < class PixType >
bool
image_difference_process<PixType>
::set_params( config_block const& blk )
{
  // an image_spacing of 1 means that frame N is differenced with
  // frame N-1, a spacing of 5 means that frame N is differenced with
  // frame N-5, and so on.
  param_->image_spacing = blk.get<unsigned>( "image_spacing" );
  param_->diff_threshold = blk.get<double>( "diff_threshold" );
  param_->shadow_removal = blk.get<bool>( "shadow_removal_with_second_diff" );

  if (param_->image_spacing < 1)
  {
    log_error( this->name() << ": image_spacing must be >= 1" );
    // revert parameters
    this->set_params( this->config_ );
    return false;
  }

  if (param_->shadow_removal && param_->image_spacing < 2)
  {
    log_error( this->name() << ": image_spacing must be >= 2 if shadow removal is enabled" );
    // revert parameters
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}

template < class PixType >
config_block
image_difference_process<PixType>
::params() const
{
  return config_;
}

template < class PixType >
bool
image_difference_process<PixType>
::initialize()
{
  clear_buffer();

  // hmm, diff image can't be sized until the first set_source_image

  src_img1_ = NULL;
  src_img2_ = NULL;
  src_img3_ = NULL;

  return true;
}

template < class PixType >
vil_image_view<bool> const&
image_difference_process<PixType>
::fg_image() const
{
  return diff_image_;
}

template < class PixType >
vil_image_view<PixType>
image_difference_process<PixType>
::bg_image() const
{
  return *(buffer_[ buffer_.size()-1 ]);
}


template < class PixType >
void
image_difference_process<PixType>
::set_source_image( vil_image_view<PixType> const& src_image )
{
  vil_image_view<PixType>* copy = new vil_image_view<PixType>;
  copy->deep_copy( src_image );

  unsigned n = buffer_.size();
  if (n == param_->image_spacing+1)
  {
    for (unsigned i=0; i<n-1; i++)
    {
      buffer_[i] = buffer_[i+1];
    }
    buffer_[n-1] = copy;
  }
  else
  {
    buffer_.push_back( copy );
  }
}


template< class PixType >
void
image_difference_process<PixType>::
set_latest_image( vil_image_view<PixType> const& img )
{
  src_img1_ = &img;
}


template< class PixType >
void
image_difference_process<PixType>::
set_second_image( vil_image_view<PixType> const& img )
{
  src_img2_ = &img;
}


template< class PixType >
void
image_difference_process<PixType>::
set_third_image( vil_image_view<PixType> const& img )
{
  src_img3_ = &img;
}


template < class PixType >
bool
image_difference_process<PixType>
::step()
{
  // If src_img1_ is set, then the contract of this class requires
  // that src_img2_ is also set.
  log_assert( src_img1_ == NULL || src_img2_ != NULL,
              this->name() << ": if the first image is set, the second image must be set" );

  unsigned const n = buffer_.size();

  // If we aren't given the two images directly, then we can retrieve
  // them from the buffer.
  if( src_img1_ == NULL )
  {
    log_assert( src_img2_ == NULL,
                this->name() << ": can't set second image without setting first image" );
    log_assert( src_img3_ == NULL,
                this->name() << ": can't set third image without setting first image" );
    // If we are to retrieve from the buffer, we must have at least one entry.
    log_assert( n > 0,
                this->name() << ": need to provide at least one image" );
    src_img1_ = buffer_[n-1];
    if (n > param_->image_spacing)
    {
      src_img2_ = buffer_[0];
      src_img3_ = buffer_[n/2];
    }
  }


  // We can size the output image now.  This'll be cheap the next time
  // around, because the size will already be correct.
  diff_image_.set_size( src_img1_->ni(), src_img1_->nj() );

  // If we don't have a second image at this point, then we don't have enough images
  // in the buffer.
  if( src_img2_ == NULL )
  {
    diff_image_.fill( false );
    src_img1_ = NULL;
    src_img2_ = NULL;
    src_img3_ = NULL;
    return true;
  }

  // Okay, now we have two images to actually difference!  Compute the
  // difference between the top and bottom images...
  compute_diff_image( param_->diff_threshold, *src_img1_, *src_img2_, diff_image_);

  if( param_->shadow_removal )
  {
    log_assert( src_img3_ != NULL,
                this->name() << ": shadow removal requires the third image to be set" );

    // Compute the difference between the latest image and middle image
    compute_diff_image( param_->diff_threshold, *src_img1_, *src_img3_, diff_short_);

    // zero out all changes in diff_image_ which were not also changes in diff_short_
    for ( unsigned i=0; i<diff_image_.ni(); ++i )
    {
      for ( unsigned j=0; j<diff_image_.nj(); ++j )
      {
        if ( ! (diff_image_( i, j ) && diff_short_( i, j ) ) )
        {
          diff_image_( i, j ) = false;
        }
      }
    }
  }

  // Mark the images as "used"
  src_img1_ = NULL;
  src_img2_ = NULL;
  src_img3_ = NULL;

  return true;
}

} // namespace vidtk
