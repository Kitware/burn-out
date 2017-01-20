/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "illumination_normalization.h"

#include <vil/vil_math.h>
#include <vil/vil_rgb.h>
#include <vil/vil_transform.h>

#include <algorithm>
#include <limits>

#include <video_transforms/video_transform_helper_functors.h>
#include <video_transforms/automatic_white_balancing.h>

namespace vidtk
{

// Median normalizer
template<class PixType>
double
median_illumination_normalization<PixType>
::calculate_median( vil_image_view<PixType> const & img ) const
{
  std::vector<PixType> medians(img.nplanes(),0);
  for(unsigned int i = 0; i < img.nplanes(); ++i)
  {
    vil_math_median(medians[i], img, i);
  }
  double m = 0;
  if(medians.size() == 3) //assuming RGB image
  {
    m = vil_rgb<PixType>(medians[0],medians[1],medians[2]).grey();
  }
  else
  {
    for(unsigned int i = 0; i < medians.size();++i)  //All others just take the average
    {
      m+= medians[i];
    }
    m = m/medians.size();
  }
  return m;
}

template<class PixType>
vil_image_view<PixType>
median_illumination_normalization<PixType>
::operator()( vil_image_view<PixType> const & img, bool deep_copy )
{
  vil_image_view<PixType> result;
  if(deep_copy)
  {
    result.deep_copy(img);
  }
  else
  {
    result = img;
  }

  double median = calculate_median( img );
  if(use_fix_median_)
  {
    double scale = reference_median_/median;
    vil_transform(result,illum_scale_functor(scale));
  }
  else if( reference_median_set_ )
  {
    double scale = reference_median_/median;
    vil_transform(result,illum_scale_functor(scale));
  }
  else
  {
    reference_median_set_ = true;
    reference_median_ = median;
  }
  return result;
}

template<class PixType>
void
median_illumination_normalization<PixType>
::reset()
{
  reference_median_set_ = false;
}


// Mean normalizer
template<class PixType>
double
mean_illumination_normalization<PixType>
::calculate_mean( vil_image_view<PixType> const & img ) const
{
  // Downsample the image
  vil_image_view<PixType> downsampled( img.top_left_ptr(),
                                       1+(img.ni()-1)/sampling_rate_,
                                       1+(img.nj()-1)/sampling_rate_,
                                       img.nplanes(),
                                       sampling_rate_ * img.istep(),
                                       sampling_rate_ * img.jstep(),
                                       img.planestep() );

  // Special case for very small images, ignore them.
  if( downsampled.size() == 0 )
  {
    return 0.0;
  }

  const double pixels_per_plane = static_cast<double>(downsampled.ni()*downsampled.nj());

  std::vector< double > chan_avg( downsampled.nplanes(), 0.0 );

  for( unsigned p=0;p<img.nplanes();++p )
  {
    vil_math_sum( chan_avg[p], downsampled, p );
  }

  // Special case for RGB, use standard intensity
  if( img.nplanes() == 3 )
  {
    return vil_rgb<double>(chan_avg[0], chan_avg[1], chan_avg[2]).grey() / pixels_per_plane;
  }

  // For all other channel amounts average each channel
  for( unsigned p=1; p<img.nplanes(); ++p )
  {
    chan_avg[0] += chan_avg[p];
  }
  return chan_avg[0] / ( pixels_per_plane*img.nplanes() );
}

template<class PixType>
vil_image_view<PixType>
mean_illumination_normalization<PixType>
::operator()( vil_image_view<PixType> const & img, bool deep_copy )
{
  vil_image_view<PixType> result;
  if( deep_copy )
  {
    result.deep_copy(img);
  }
  else
  {
    result = img;
  }

  // Calculate current illumination
  double est = this->calculate_mean( img );
  illum_history_.insert( est );

  // Calculate desired average illumination
  double avg = 0.0;
  for( unsigned i = 0; i < illum_history_.size(); i++ )
  {
    avg += illum_history_.datum_at( i );
  }
  avg = avg / illum_history_.size();

  // Threshold average illumination based on type
  double min_threshold = min_illum_allowed_ * default_white_point<PixType>::value();
  double max_threshold = max_illum_allowed_ * default_white_point<PixType>::value();

  if( avg < min_threshold )
  {
    avg = min_threshold;
  }
  if( avg > max_threshold )
  {
    avg = max_threshold;
  }

  // If empty frames received, simply reset and exit
  if( avg == 0.0 || est == 0.0 )
  {
    this->reset();
    return result;
  }

  // Adjust image brightness: note this is a relatively fast, but not a
  // great approach when the variance between the current and average
  // illumination is large, in which case it will result in over/under
  // saturation and greater error in normalization.
  vil_transform( result, illum_scale_functor(avg/est) );
  return result;
}

template<class PixType>
void
mean_illumination_normalization<PixType>
::reset()
{
  illum_history_.clear();
  illum_history_.set_capacity( window_length_ );
}

}
