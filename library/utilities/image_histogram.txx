/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_histogram_txx_
#define vidtk_image_histogram_txx_

#include <utilities/image_histogram.h>

#include <utilities/rgb2yuv.h>

#include <vil/vil_math.h>
#include <vnl/vnl_math.h>
#include <algorithm>
#include <limits>

#include <logger/logger.h>
VIDTK_LOGGER("image_histogram_txx");


namespace vidtk
{

template< class imT, class maskT >
image_histogram
image_histogram_functions<imT,maskT>
::generate( vil_image_view<imT> const & image,
            std::vector<unsigned> const & n_bins,
            bool skip_zero,
            Image_Histogram_Type histType )
{
  image_histogram result( skip_zero, histType );
  result.init_bins(image.nplanes(),n_bins);
  image_histogram_functions<imT,maskT>::add(result,image);
  return result;
}

template< class imT, class maskT >
image_histogram
image_histogram_functions<imT,maskT>
::generate( vil_image_view<imT> const & image,
            vil_image_view<maskT> const & mask,
            std::vector<unsigned> const & n_bins,
            bool skip_zero,
            Image_Histogram_Type histType)
{
  image_histogram result( skip_zero, histType );
  result.init_bins(image.nplanes(),n_bins);
  image_histogram_functions<imT,maskT>::add(result,image,&mask);
  return result;
}

template< class imT, class maskT >
void
image_histogram_functions<imT,maskT>
::init( image_histogram & hist,
        vil_image_view<imT> const & image,
        std::vector<unsigned> const & n_bins )
{
  hist.init_bins(image.nplanes(),n_bins);
}

template< class imT, class maskT >
void
image_histogram_functions<imT,maskT>
::rgb2yuv_wrapper( imT R, imT G, imT B, std::vector<double> & yuv )
{
  LOG_ASSERT( yuv.size() == 3, "Unexpected YUV vector size." );

  imT yuv_imT[3];
  rgb2yuv(R,G, B, yuv_imT);
  yuv[0] = static_cast<double>(yuv_imT[0]);
  yuv[1] = static_cast<double>(yuv_imT[1]);
  yuv[2] = static_cast<double>(yuv_imT[2]);
}

template< class imT, class maskT >
void image_histogram_functions<imT,maskT>
::add( image_histogram & hist,
       vil_image_view<imT> const & image,
       vil_image_view<maskT> const * mask )
{
  LOG_ASSERT( hist.hist_type_ != VIDTK_AUTO,
              "Histogram has not been initialized." );

  LOG_ASSERT( (image.nplanes() == 3 && hist.h_.nplanes() > 1) ||
              (image.nplanes() == hist.h_.nplanes()) ,
              "Number of channels in the input image and histogram bins "
              "are inconsistent." );

  // if present, mask should be the same size as image
  LOG_ASSERT( !mask ||
              (mask->ni() == image.ni() && mask->nj() == image.nj()),
              "Mask and image sizes are inconsistent." );

  //populate the histogram
  std::vector<unsigned> bins( 3, 0 );
  std::vector<double> channels( image.nplanes(), 0 );
  for (unsigned i=0; i<image.ni(); i++)
  {
    for (unsigned j=0; j<image.nj(); j++)
    {
      if((!mask) || static_cast<float>((*mask)(i,j))>0 )
      {
        bool is_black_pix = true;
        // only run the loop if skip_zero_ == true
        for( unsigned p=0; p<image.nplanes() && hist.skip_zero_; p++ )
        {
          is_black_pix = is_black_pix && (image(i,j,p) == 0);
        }

        static const double range = static_cast<double>(std::numeric_limits<imT>::max()-std::numeric_limits<imT>::min());

        if( !hist.skip_zero_ || !is_black_pix )
        {
          switch(hist.hist_type_)
          {
            case VIDTK_GRAYSCALE:
              channels[0] = (static_cast<double>(image(i,j)) - std::numeric_limits<imT>::min()) / range;
              break;
            case VIDTK_YUV:
              image_histogram_functions<imT,maskT>::rgb2yuv_wrapper( image(i,j,0),
                                                                     image(i,j,1),
                                                                     image(i,j,2),
                                                                     channels );
              channels[0] = (channels[0] - std::numeric_limits<imT>::min()) / range;
              channels[1] = (channels[1] - std::numeric_limits<imT>::min()) / range;
              channels[2] = (channels[2] - std::numeric_limits<imT>::min()) / range;
              break;
            case VIDTK_RGB:
              channels[0] = (static_cast<double>(image(i,j,0)) - std::numeric_limits<imT>::min()) / range;
              channels[1] = (static_cast<double>(image(i,j,1)) - std::numeric_limits<imT>::min()) / range;
              channels[2] = (static_cast<double>(image(i,j,2)) - std::numeric_limits<imT>::min()) / range;
              break;
            default:
              LOG_ERROR( "Invalid histogram type" );
              continue;
          }
          hist.update(channels, (mask?(static_cast<double>( (*mask)(i,j) )):(1.0)));
        } // if not balck pixel
      } // if no mask or *good* value
    } // for nj
  }// for ni

  //Normalize the histogram.
  hist.normalize();
}

} // namespace

#endif
