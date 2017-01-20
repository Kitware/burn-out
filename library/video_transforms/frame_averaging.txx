/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_averaging.h"

#include <vil/vil_convert.h>
#include <vil/vil_math.h>

#include <logger/logger.h>

VIDTK_LOGGER("frame_averager_txx");

namespace vidtk
{


// Shared functionality - process a frame while computing variance
template <typename PixType>
void online_frame_averager<PixType>
::process_frame( const vil_image_view< PixType >& input,
                 vil_image_view< PixType >& average,
                 vil_image_view< double >& variance )
{
  // Check if this is the first time we have processed a frame of this size
  if( dev1_tmp_space_.ni() != input.ni() ||
      dev1_tmp_space_.nj() != input.nj() ||
      dev1_tmp_space_.nplanes() != input.nplanes() )
  {
    dev1_tmp_space_.set_size( input.ni(), input.nj(), input.nplanes() );
    dev2_tmp_space_.set_size( input.ni(), input.nj(), input.nplanes() );
    variance.set_size( input.ni(), input.nj(), input.nplanes() );
    variance.fill( 0.0 );
    this->process_frame( input, average );
    return;
  }

  // Calculate difference from last average
  vil_math_image_difference( input, this->last_average_, dev1_tmp_space_ );

  // Update internal average
  this->process_frame( input, average );

  // Update the variance
  vil_math_image_difference( input, average, dev2_tmp_space_ );
  vil_math_image_product( dev1_tmp_space_, dev2_tmp_space_, dev1_tmp_space_ );
  variance.deep_copy( dev1_tmp_space_ );
}

template <typename PixType>
bool online_frame_averager<PixType>
::has_resolution_changed( const vil_image_view< PixType >& input )
{
  return ( input.ni() != this->last_average_.ni() ||
           input.nj() != this->last_average_.nj() ||
           input.nplanes() != this->last_average_.nplanes() );
}

// Helper function to allocate a completely new image, and cast the input image
// to whatever specified type the output image is, scaling by some factor if set
// and rounding if enabled, in one pass.
template <typename inT, typename outT>
void copy_cast_and_scale( const vil_image_view< inT >& input,
                          vil_image_view< outT >& output,
                          bool rounding_enabled,
                          const inT scale_factor )
{
  // Just deep copy if pixel formats equivalent and there is no scale factor
  if( vil_pixel_format_of(inT()) == vil_pixel_format_of(outT())
      && scale_factor == 1.0 )
  {
    output.deep_copy( input );
    return;
  }

  // Determine if any rounding would even be beneficial based on source types
  if( rounding_enabled )
  {
    rounding_enabled = !std::numeric_limits<inT>::is_integer &&
                       std::numeric_limits<outT>::is_integer;
  }

  // Resize, cast and copy
  unsigned ni = input.ni(), nj = input.nj(), np = input.nplanes();
  output.set_size( ni, nj, np );

  // s* = source properties, d* = dest
  std::ptrdiff_t sistep=input.istep(), sjstep=input.jstep(), spstep=input.planestep();
  std::ptrdiff_t distep=output.istep(), djstep=output.jstep(), dpstep=output.planestep();

  // Iterate through image and perform cast and any required ops. We could
  // shorten this code using vil_transform and functors (binding arguments),
  // but just to ensure they get inlined...
  if( scale_factor == 1.0 && !rounding_enabled )
  {
    vil_convert_cast( input, output );
  }
  else if( !rounding_enabled )
  {
    const inT* splane = input.top_left_ptr();
    outT* dplane = output.top_left_ptr();
    for (unsigned p=0;p<np;++p,splane += spstep,dplane += dpstep)
    {
      const inT* srow = splane;
      outT* drow = dplane;
      for (unsigned j=0;j<nj;++j,srow += sjstep,drow += djstep)
      {
        const inT* spixel = srow;
        outT* dpixel = drow;
        for (unsigned i=0;i<ni;++i,spixel+=sistep,dpixel+=distep)
        {
          *dpixel = static_cast<outT>(*spixel * scale_factor);
        }
      }
    }
  }
  else
  {
    const inT* splane = input.top_left_ptr();
    outT* dplane = output.top_left_ptr();
    for (unsigned p=0;p<np;++p,splane += spstep,dplane += dpstep)
    {
      const inT* srow = splane;
      outT* drow = dplane;
      for (unsigned j=0;j<nj;++j,srow += sjstep,drow += djstep)
      {
        const inT* spixel = srow;
        outT* dpixel = drow;
        for (unsigned i=0;i<ni;++i,spixel+=sistep,dpixel+=distep)
        {
          *dpixel = static_cast<outT>(*spixel * scale_factor + 0.5);
        }
      }
    }
  }
}

// Cumulative averager
template <typename PixType>
cumulative_frame_averager<PixType>
::cumulative_frame_averager( const bool should_round )
{
  this->should_round_ = should_round;
  this->reset();
}

template <typename PixType>
void cumulative_frame_averager<PixType>
::process_frame( const vil_image_view< PixType >& input,
                 vil_image_view< PixType >& average )
{
  if( this->has_resolution_changed( input ) )
  {
    this->reset();
  }

  // If this is the first frame observed or there was an indicated reset
  if( this->frame_count_ == 0 )
  {
    vil_convert_cast( input, this->last_average_ );
  }
  // Standard update case
  else
  {
    // Calculate new average - TODO: Non-exponential cumulative average can be modified
    // to be more efficient and prevent precision losses by not using math_add_fraction
    // function. Can also be optimized in the byte case to use integer instead of double
    // operations, but it's good enough for now.
    double scale_factor = 1.0 / ( this->frame_count_ + 1 );

    vil_math_add_image_fraction( this->last_average_,
                                 1.0 - scale_factor,
                                 input,
                                 scale_factor );
  }

  // Copy a completely new image in case we are running in async mode
  copy_cast_and_scale( this->last_average_,
                       average,
                       this->should_round_,
                       1.0 );

  // Increase observed frame count
  this->frame_count_++;
}

template <typename PixType>
void cumulative_frame_averager<PixType>
::reset()
{
  this->frame_count_ = 0;
}

// Exponential averager
template <typename PixType>
exponential_frame_averager<PixType>
::exponential_frame_averager( const bool should_round,
                              const double new_frame_weight )
{
  if( new_frame_weight < 0.0 || new_frame_weight > 1.0 )
  {
    LOG_WARN( "Exponential averaging weights should usually be between 0.0 and 1.0" );
  }

  this->should_round_ = should_round;
  this->new_frame_weight_ = new_frame_weight;
  this->reset();
}

template <typename PixType>
void exponential_frame_averager<PixType>
::process_frame( const vil_image_view< PixType >& input,
                 vil_image_view< PixType >& average )
{
  if( this->has_resolution_changed( input ) )
  {
    this->reset();
  }

  // If this is the first frame observed or there was an indicated reset
  if( this->frame_count_ == 0 )
  {
    vil_convert_cast( input, this->last_average_ );
  }
  // Standard update case
  else
  {
    vil_math_add_image_fraction( this->last_average_,
                                 1.0-new_frame_weight_,
                                 input,
                                 new_frame_weight_ );
  }

  // Copy a completely new image in case we are running in async mode
  copy_cast_and_scale( this->last_average_,
                       average,
                       this->should_round_,
                       1.0 );

  // Increase observed frame count
  this->frame_count_++;
}

template <typename PixType>
void exponential_frame_averager<PixType>
::reset()
{
  this->frame_count_ = 0;
}

// Windowed averager
template <typename PixType>
windowed_frame_averager<PixType>
::windowed_frame_averager( const bool should_round,
                           const unsigned window_length )
{
  this->window_buffer_.set_capacity( window_length );
  this->should_round_ = should_round;
  this->reset();
}

template <typename PixType>
void windowed_frame_averager<PixType>
::process_frame( const vil_image_view< PixType >& input,
                 vil_image_view< PixType >& average )
{
  if( this->has_resolution_changed( input ) )
  {
    this->reset();
  }

  // Early exit cases: the buffer is currently filling
  const unsigned window_buffer_size = window_buffer_.size();
  if( window_buffer_size == 0 )
  {
    vil_convert_cast( input, this->last_average_ );
  }
  else if( window_buffer_size < window_buffer_.capacity() )
  {
    double src_weight = 1.0/(window_buffer_size+1.0);
    vil_math_add_image_fraction( this->last_average_, 1.0-src_weight, input, src_weight );
  }
  // Standard case, buffer is full
  else
  {
    // Scan image subtracting the last frame, and adding the new one from
    // the previous average
    const unsigned ni = input.ni();
    const unsigned nj = input.nj();
    const unsigned np = input.nplanes();

    // Image A = Removed Entry, B = Added Entry, C = The Average Calculation
    const double scale = 1.0/window_buffer_size;

    const input_type& tmpA = window_buffer_.datum_at( window_buffer_size-1 );
    const input_type* imA = &tmpA;
    const input_type* imB = &input;
    vil_image_view< double >* imC = &(this->last_average_);

    std::ptrdiff_t istepA=imA->istep(),jstepA=imA->jstep(),pstepA=imA->planestep();
    std::ptrdiff_t istepB=imB->istep(),jstepB=imB->jstep(),pstepB=imB->planestep();
    std::ptrdiff_t istepC=imC->istep(),jstepC=imC->jstep(),pstepC=imC->planestep();

    const PixType* planeA = imA->top_left_ptr();
    const PixType* planeB = imB->top_left_ptr();
    double*        planeC = imC->top_left_ptr();

    for( unsigned p=0;p<np;++p,planeA+=pstepA,planeB+=pstepB,planeC+=pstepC )
    {
      const PixType* rowA = planeA;
      const PixType* rowB = planeB;
      double*        rowC = planeC;

      for( unsigned j=0;j<nj;++j,rowA+=jstepA,rowB+=jstepB,rowC+=jstepC )
      {
        const PixType* pixelA = rowA;
        const PixType* pixelB = rowB;
        double*        pixelC = rowC;

        for( unsigned i=0;i<ni;++i,pixelA+=istepA,pixelB+=istepB,pixelC+=istepC )
        {
          *pixelC += scale*(static_cast<double>(*pixelB)-static_cast<double>(*pixelA));
        }
      }
    }
  }

  // Add to buffer
  this->window_buffer_.insert( input );

  // Copy into output
  copy_cast_and_scale( this->last_average_,
                       average,
                       this->should_round_,
                       1.0 );

}

template <typename PixType>
void windowed_frame_averager<PixType>
::reset()
{
  this->window_buffer_.clear();
}

template <typename PixType>
unsigned windowed_frame_averager<PixType>
::frame_count() const
{
  return this->window_buffer_.size();
}


} // end namespace vidtk
