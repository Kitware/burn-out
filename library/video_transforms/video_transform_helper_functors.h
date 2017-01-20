/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//video_transforms/video_transform_helper_functors.h

#include <cmath>
#include <limits>
#include <algorithm>

namespace vidtk
{

// Simple scaling functor for increasing/decreasing image brightness on
// a per-type basis
class illum_scale_functor
{
  double s_;

public:

  illum_scale_functor(double s) : s_(s) {}

  // If scaling a byte-wise or 16-bit image, threshold at max value
  #define scaling_function(TYPE)\
  TYPE operator()(TYPE x) const\
  {\
  double temp_ = 0.5 + s_*x;\
  if( temp_ > std::numeric_limits<TYPE>::max() )\
  {\
  return std::numeric_limits<TYPE>::max();\
  }\
  return static_cast<TYPE>(temp_);\
  }
  scaling_function(vxl_byte)
  scaling_function(vxl_uint_16)

  // If scaling floating point image, threshold at 1.0
  float operator()(float x) const
  {
    return std::min( 1.0f, static_cast<float>(s_*x) );
  }
  double operator()(double x) const
  {
    return std::min( 1.0, s_*x );
  }

  // For all other ops perform standard scaling (likely unused)
  vxl_uint_32 operator()(vxl_uint_32 x) const
  {
    return static_cast<vxl_uint_32>(0.5+s_*x);
  }
  vxl_int_32 operator()(vxl_int_32 x) const
  {
    double r=s_*x;
    return static_cast<vxl_int_32>(r<0?r-0.5:r+0.5);
  }
  std::complex<double> operator()(std::complex<double> x) const
  {
    return s_*x;
  }
};

template<class PixType>
class offset_functor
{
  double image_median_, reference_median_;

public:

  offset_functor(double im, double rm) : image_median_(im), reference_median_(rm)
  {}

  // If scaling a byte-wise or 16-bit image, threshold at max value
  PixType operator()(PixType x) const
  {
    double r = x - image_median_ + reference_median_;
    if(r > std::numeric_limits<PixType>::max())
    {
      return std::numeric_limits<PixType>::max();
    }
    else if( r < std::numeric_limits<PixType>::min() )
    {
      return std::numeric_limits<PixType>::min();
    }
    return static_cast<PixType>(r);
  }
};

}//namespace vidtk
