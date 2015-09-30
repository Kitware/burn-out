/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/image_histogram.h>

#include <utilities/rgb2yuv.h>
#include <utilities/log.h>
#include <vil/vil_math.h>
#include <vnl/vnl_math.h>
#include <vcl_algorithm.h>

namespace vidtk
{

template< class imT, class maskT>
image_histogram<imT,maskT>
::image_histogram( )
  : n_(0),
    h_(),
    skip_zero_( false ),
    hist_type_( VIDTK_AUTO )
{
}

template< class imT, class maskT >
image_histogram<imT,maskT>
::image_histogram( const image_histogram& that)
{
  *this = that;
}

template< class imT, class maskT >
image_histogram<imT,maskT> &
image_histogram<imT,maskT>
::operator=( image_histogram const & that)
{
  this->h_.clear();
  this->h_.deep_copy ( that.h_ );
  this->n_ = that.n_;
  this->skip_zero_ = that.skip_zero_;

  return *this;
}

template< class imT, class maskT>
image_histogram<imT,maskT>
::image_histogram( vil_image_view<imT> const & image,
                   vcl_vector<unsigned> const & n_bins,
                   bool skip_zero,
                   Image_Histogram_Type h_t )
: n_( 0 ),
  skip_zero_( skip_zero ),
  hist_type_( h_t )
{
  this->init( image, n_bins, NULL );
}

template< class imT, class maskT >
image_histogram<imT,maskT>
::image_histogram( vil_image_view<imT> const& image,
                   vil_image_view<maskT> const& mask,
                   vcl_vector<unsigned> const & n_bins,
                   bool skip_zero,
                   Image_Histogram_Type h_t )
: n_(0),
  skip_zero_( skip_zero ),
  hist_type_( h_t )
{
  this->init( image, n_bins, &mask );
}

template< class imT, class maskT >
image_histogram<imT,maskT>
::~image_histogram()
{
  this->h_.clear();
}


template< class imT, class maskT >
void
image_histogram<imT,maskT>
::add( vil_image_view<imT> const& image,
       vil_image_view<maskT> const * mask )
{
  log_assert( this->hist_type_ != VIDTK_AUTO,
              "Hsitogram has not been initialized. Call \n" );

  log_assert( (image.nplanes() == 3 && this->h_.nplanes() > 1) ||
              (image.nplanes() == this->h_.nplanes()) ,
              "Number of channels in the input image and histogram bins "
              "are inconsistent.\n" );

  // if present, mask should be the same size as image
  log_assert( !mask ||
              (mask->ni() == image.ni() && mask->nj() == image.nj()),
              "Mask and image sizes are inconsistent.\n" );

  // populate the histogram

  vcl_vector<unsigned> bins( 3, 0 );
  vcl_vector<unsigned char> channels( image.nplanes() );
  for (unsigned i=0; i<image.ni(); i++)
  {
    for (unsigned j=0; j<image.nj(); j++)
    {
      if((!mask) || static_cast<float>((*mask)(i,j))>0 )
      {
        bool is_black_pix = true;
        // only run the loop if skip_zero_ == true
        for( unsigned p=0; p<image.nplanes() && skip_zero_; p++ )
        {
          is_black_pix = is_black_pix && (image(i,j,p) == 0);
        }

        if( !skip_zero_ || !is_black_pix )
        {
          if( this->hist_type_ == VIDTK_GRAYSCALE )
          {
            channels[0] = image(i,j);
          }
          else
          {
            if( this->hist_type_ == VIDTK_YUV )
            {
              rgb2yuv_wrapper( image(i,j,0), image(i,j,1), image(i,j,2),
                               channels );
            }
            else
            {
              channels[0] = image(i,j,0);
              channels[1] = image(i,j,1);
              channels[2] = image(i,j,2);
            }
          }
          get_bin_location( channels, bins );
          double &bin_value = h_( bins[0], bins[1], bins[2] );
          if( mask )
          {
            if( vnl_math::isnan( static_cast<float>( (*mask)(i,j) ) ) )
            {
              log_error( "The mask used for computing histogram contains NaNs.\n" );
            }
            else
            {
              bin_value += (double)(*mask)(i,j);
              n_ += (double)(*mask)(i,j);
            }
          }
          else
          {
            bin_value++;
            n_++;
          }
        } // if not balck pixel
      } // if no mask or *good* value
    } // for nj
  }// for ni

  //Normalize the histogram, i.e. h_ /= n_;
  if( n_ > 0 )
    vil_math_scale_values( h_, 1/n_ );
}

template< class imT, class maskT >
void
image_histogram<imT,maskT>
::init( vil_image_view<imT> const & image,
        vcl_vector<unsigned> const & n_bins,
        vil_image_view<maskT> const * mask )
{
  if( this->hist_type_ == VIDTK_AUTO )
  {
    if( image.nplanes() == 1 )
    {
      this->hist_type_ = VIDTK_GRAYSCALE;
    }
    else
    {
      this->hist_type_ = VIDTK_YUV;
    }
  }

  log_assert( image.nplanes() == n_bins.size(),
              "Channels in the input image and dimensionality of the "
              "histogram are inconsistent.\n" );

  log_assert( (this->hist_type_ == VIDTK_GRAYSCALE && n_bins.size() == 1)
           || (this->hist_type_ == VIDTK_YUV && n_bins.size() == 3)
           || (this->hist_type_ == VIDTK_RGB && n_bins.size() == 3),
              "Histogram type and dimensionality of the histogram "
              " are inconsistent\n." );

  if( this->hist_type_ == VIDTK_GRAYSCALE )
  {
    h_.set_size( n_bins[0], 1, 1);
  }
  else
  {
    h_.set_size( n_bins[0], n_bins[1], n_bins[2]);
  }
  h_.fill( 0 );
  n_ = 0;

  this->add( image, mask );
}


template< class imT, class maskT >
void
image_histogram<imT,maskT>
::get_bin_location( vcl_vector<unsigned char> const & channels,
                    vcl_vector<unsigned> & bins ) const
{
  bins[0] = static_cast<unsigned>( (double) channels[0] / 255 * (h_.ni()-1) );
  if( this->hist_type_ != VIDTK_GRAYSCALE )
  {
    bins[1] = static_cast<unsigned>( (double) channels[1] / 255 * (h_.nj()-1) );
    bins[2] = static_cast<unsigned>( (double) channels[2] / 255 * (h_.nplanes()-1) );
  }
}

template < class imT, class maskT >
double image_histogram<imT,maskT>
::compare( image_histogram<imT, maskT> const & other ) const
{
  if((this->mass() == 0) || (other.mass() == 0) )
    return 0.0;
  if( this->hist_type_ != other.type() )
    return 0.0;

  return this->compare( other.get_h() );
}

template < class imT, class maskT >
double image_histogram<imT,maskT>
::compare( vil_image_view<double> const & others_h ) const
{
  // try the intersection distance:
  // --  M. J. Swain and D. H. Ballard. "Color indexing", IJCV, 7:1 1991.
  assert( this->h_.ni() == others_h.ni() &&
          this->h_.nj() == others_h.nj() &&
          this->h_.nplanes() == others_h.nplanes() );

  double sum=0;
  for (unsigned k=0; k<this->h_.nplanes(); k++) {
    for (unsigned i=0; i<this->h_.ni(); i++) {
      for (unsigned j=0; j<this->h_.nj(); j++) {
        sum += vcl_min( this->h_(i,j,k), others_h(i,j,k) );
      }
    }
  }

//  return 1.0*sum / vcl_min( this->mass(), other.mass() );
  return sum ;
}


template<class imT, class maskT >
void
image_histogram<imT, maskT >
::rgb2yuv_wrapper( imT R, imT G, imT B, vcl_vector<unsigned char> & yuv ) const
{
  log_assert( yuv.size() == 3, "Unexpected YUV vector size.\n" );

  unsigned char yuv_chars[3];
  rgb2yuv(R,G, B, yuv_chars);
  yuv[0] = yuv_chars[0];
  yuv[1] = yuv_chars[1];
  yuv[2] = yuv_chars[2];
}

} // namespace

