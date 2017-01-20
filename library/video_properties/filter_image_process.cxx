/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_image_process.h"

#include <algorithm>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_filter_image_process_cxx__
VIDTK_LOGGER("filter_image_process_cxx");


namespace vidtk
{


filter_image_process
::filter_image_process( std::string const& _name )
  : process( _name, "filter_image_process" ),
    src_img_( NULL ),
    mean_pcent_fg_pixels_( 0.0 ),
    alpha_( 0.0 ),
    threshold_( 0.0 ),
    isgood_( true )
{
  config_.add_parameter( "disabled", "true", "UNDOCUMENTED" );
  config_.add_parameter( "threshold", "0.075", "UNDOCUMENTED" );
  config_.add_parameter( "alpha", "0.01", "UNDOCUMENTED" );
  config_.add_parameter( "init_window", "5", "UNDOCUMENTED" );
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
    disabled_ = blk.get<bool>( "disabled" );
    threshold_ = blk.get<double>( "threshold" );
    alpha_ = blk.get<double>( "alpha" );
    init_window_ = blk.get<unsigned>( "init_window" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
filter_image_process
::initialize()
{
  mean_pcent_fg_pixels_ = 0.0;
  init_counter_ = 1;
  return true;
}


bool
filter_image_process
::step()
{
  LOG_ASSERT( src_img_ != NULL, "Source image not set" );

  if( disabled_ )
  {
    return true;
  }

  unsigned ni = src_img_->ni(),
           nj = src_img_->nj();

  std::ptrdiff_t istepA = src_img_->istep(),
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

  double fg_pixels_pcent = count / static_cast<double>( nj * ni );

  if( init_counter_ <= init_window_ && fg_pixels_pcent > 0 )
  {
    mean_pcent_fg_pixels_ = (1 - 1/init_counter_) * mean_pcent_fg_pixels_ +
                                (1/init_counter_) * fg_pixels_pcent;
    init_counter_++;
  }

  double z = fg_pixels_pcent - mean_pcent_fg_pixels_;
  if( z > threshold_ )
  {
    isgood_ = false;
    LOG_INFO( this->name() <<": Detected a noisy foreground image (frac_fg: "
                           << fg_pixels_pcent << ", mean: " << mean_pcent_fg_pixels_
                           <<", z: "<< z << ")" );
  }
  else
  {
    isgood_ = true;

    if( init_counter_ > init_window_ )
    {
      mean_pcent_fg_pixels_ = (1 - alpha_) * mean_pcent_fg_pixels_ +
                   alpha_  * fg_pixels_pcent;
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
