/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_descriptor_vxl_to_vl_wrappers
#define vidtk_descriptor_vxl_to_vl_wrappers

#include <vector>
#include <string>
#include <cassert>

#include <vgl/vgl_box_2d.h>
#include <vil/vil_memory_chunk.h>
#include <vil/vil_image_view.h>
#include <vil/vil_math.h>

namespace vidtk
{

/// Create an output image that VLFeat will accept as input for most functions.
/// This wrapped image will simply be a shallow copy if possible, but if the
/// input is non-contiguous, has non-interlaced planes, or we want to force
/// a single channel image as output, it will be deep copied.
template< typename PixType >
vil_image_view< PixType > forced_vl_conversion( const vil_image_view< PixType >& image,
                                                const bool force_single_channel = true )
{
  if( image.nplanes() == 1 )
  {
    if( image.is_contiguous() )
    {
      return image;
    }
    else
    {
      vil_image_view< PixType > output( image.ni(), image.nj() );
      output.deep_copy( image );
      return output;
    }
  }

  if( force_single_channel )
  {
    vil_image_view< PixType > output( image.ni(), image.nj() );
    vil_math_mean_over_planes( image, output );
    return output;
  }

  if( image.istep() == image.nplanes() &&
      image.is_contiguous() &&
      image.planestep() == image.ni() * image.nj() )
  {
    return image;
  }

  vil_image_view< PixType > output( image.ni(), image.nj(), 1, image.nplanes() );
  output.deep_copy( image );
  return output;
}


/// Several VLFeat algorithms require either a contiguous array of single
/// channel data, or a contiguous array of interlaced multi-channel data,
/// which the below class enforces on construction.
///
/// This immutable wrapped image will simply be a shallow copy if possible,
/// but if the input image is non-contiguous, or contains multiple planes
/// it will be deep copied into a single-planed image.
template< typename PixType >
class vl_compatible_view
{
public:

  // Raw data accessors, does not allow modification of contents.
  const PixType* data_ptr() const { return data_.top_left_ptr(); }
  int height() const { return data_.nj(); }
  int width() const { return data_.ni(); }
  int channels() const { return data_.nplanes(); }

protected:

  // Internal image view in the correct format, containing sptr to data.
  vil_image_view< PixType > data_;

public:

  // Constructor, enforcing correct format.
  vl_compatible_view( const vil_image_view< PixType >& input,
                      const bool force_single_channel = true )
  {
    data_ = forced_vl_conversion( input, force_single_channel );
  }

  virtual ~vl_compatible_view() {}
};

}

#endif
