/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_image_process.h"

#include <vcl_algorithm.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{


filter_image_process
::filter_image_process( vcl_string const& name )
  : process( name, "filter_image_process" ),
    src_img_( NULL ),
    mean_( 0.0 ),
    alpha_( 0.0 ),
    //sigma_( 0.0 ),
    threshold_( 0.0 ),
    isgood_( true )
{
  config_.add( "disabled", "true" );
  config_.add( "threshold", "0.075" );
  config_.add( "alpha", "0.01" );
  config_.add( "init_window", "5" );
  //config_.add( "init_sigma", "0.05" );
}


filter_image_process
::~filter_image_process()
{
}


config_block
filter_image_process
::params() const
{
  return config_;
}


bool
filter_image_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    blk.get( "threshold", threshold_ );
    blk.get( "alpha", alpha_ );
    blk.get( "init_window", init_window_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
filter_image_process
::initialize()
{
  init_counter_ = 1;
  return true;
}


bool
filter_image_process
::step()
{
  log_assert( src_img_ != NULL, "Source image not set" );

  if( disabled_ )
  {
    return true;
  }

  unsigned ni = src_img_->ni(),
           nj = src_img_->nj();

  vcl_ptrdiff_t istepA = src_img_->istep(),
                jstepA = src_img_->jstep();

  const bool* planeA = src_img_->top_left_ptr();

  unsigned count = 0;

  const bool * rowA = planeA;
  for (unsigned j=0; j < nj; j++, rowA += jstepA)
  {
    const bool* pixelA = rowA;
    for (unsigned i=0; i < ni; i++, pixelA += istepA)
    {
      if( *pixelA )
      {
        count++;
      }
    } // unsigned i
  } // unsigned j

  double count_frac = count / static_cast<double>( nj * ni );

  if( init_counter_ <= init_window_ && count_frac > 0 )
  {
    init_counter_++;

    mean_ = (1 - 1/init_window_) * mean_ +
                (1/init_window_) * count_frac;
  }

  double z = count_frac - mean_;
  if( z > threshold_ )
  {
    isgood_ = false;
    log_info( this->name() <<": Detected a noisy foreground image (frac_fg: "
                           << count_frac << ", mean: " << mean_
                           <<", z: "<< z << ")\n" );
  }
  else
  {
    isgood_ = true;

    if( init_counter_ > init_window_ )
    {
      mean_ = (1 - alpha_) * mean_ +
                   alpha_  * count_frac;
    }
  }

  src_img_ = NULL; // prepare for the next set

  return true;
}


void
filter_image_process
::set_source_image( vil_image_view< bool > const& src )
{
  src_img_ = &src;
}


bool
filter_image_process
::isgood() const
{
  return isgood_;
}

} // end namespace vidtk
