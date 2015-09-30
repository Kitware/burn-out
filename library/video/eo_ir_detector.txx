/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "eo_ir_detector.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

template < class PixType >
eo_ir_detector< PixType >
::eo_ir_detector( PixType rg_diff,
                  PixType rb_diff,
                  PixType gb_diff,
                  float eo_ir_sample_size,
                  float eo_multiplier )
  : rg_diff_(rg_diff),
    rb_diff_(rb_diff),
    gb_diff_(gb_diff),
    eo_ir_sample_size_(eo_ir_sample_size),
    eo_multiplier_(eo_multiplier)
{
  // Add config parameters
  config_.add_parameter( "eo_multiplier", "4",
                         "Multiplier used to help determin if "
                         "the video is in eo or ir." );

  config_.add_parameter( "rg_diff", "8",
                         "Difference threshold for red and green."
                         "  Used when determining eo or ir." );

  config_.add_parameter( "rb_diff", "8",
                         "Difference threshold for red and blue."
                         "  Used when determining eo or ir." );

  config_.add_parameter( "gb_diff", "8",
                         "Difference threshold for green and blue."
                         "  Used when determining eo or ir." );
}


// ----------------------------------------------------------------
/** Accept new parameters.
 *
 *
 */
template < class PixType >
bool
eo_ir_detector< PixType >
::set_params( config_block const& cfg )
{
  try
  {
    // extract relevant parameters
    this->eo_multiplier_ = cfg.get<float>("eo_multiplier");
    this->rg_diff_ = cfg.get<unsigned int>("rg_diff");
    this->rb_diff_ = cfg.get<unsigned int>("rb_diff");
    this->gb_diff_ = cfg.get<unsigned int>("gb_diff");
  }
  catch( unchecked_return_value const& e )
  {
    log_error( "Could not set parameters: "<< e.what() <<".\n" );
    return false;
  }

  config_.update(cfg);
  return true;
}


// ----------------------------------------------------------------
/** Is image an IR image?
 *
 *
 */
template < class PixType >
bool
eo_ir_detector< PixType >
::is_ir( vil_image_view< PixType > const & img ) const
{
  log_assert( img.nplanes() == 3, "eo_ir_detector: Assumes there are 3 planes, at least for now" );
  vxl_uint_32 res_ir = 0;
  vxl_uint_32 res_eo = 0;

  //Sample .5% of the image for EO and IR
  unsigned int i_ss = img.ni()/unsigned(img.ni()*eo_ir_sample_size_);
  unsigned int j_ss = img.nj()/unsigned(img.nj()*eo_ir_sample_size_);
  //log_debug( i_ss << " " << j_ss << vcl_endl);
  for( unsigned int i = 0; i < img.ni(); i+=i_ss)
  {
    for(unsigned int j = 0; j < img.nj(); j+=j_ss)
    {
      vxl_sint_32 r = (vxl_sint_32) img(i,j,0);
      vxl_sint_32 g = (vxl_sint_32) img(i,j,1);
      vxl_sint_32 b = (vxl_sint_32) img(i,j,2);

      //if the pixels are all close to the same color they are ir
      if( (vcl_abs(r-g) < rg_diff_) && (vcl_abs(r-b) < rb_diff_) && (vcl_abs(g-b) < gb_diff_) )
      {
        res_ir++;
      }
      else
      {
        res_eo++;
      }
    }
  }

  //if there are many more ir frames than eo frames it is ir, otherwise it is probably eo
  bool result = res_ir > res_eo*eo_multiplier_;

  log_debug( "\nImage Type: " << ((result)?"IR":"EO") << "\n"
             << "Total Sample Size: " << res_ir+res_eo << " out of " << img.size()/3 << "\n"
             << "Percent Sampled: "<<eo_ir_sample_size_ << "%\n"
             << "EO Threshold: " << (vxl_uint_32) eo_multiplier_ << "\n"
             << "PXLs IR: "<<res_ir<<"\n"
             << "PXLs EO: "<<res_eo<<"\n" );
  return result;
}


// ----------------------------------------------------------------
/** Is image an IR image?
 *
 *
 */
template < class PixType >
bool
eo_ir_detector< PixType >
::is_eo( vil_image_view< PixType > const & img ) const
{
  return !this->is_ir(img);
}

} // end namespace
