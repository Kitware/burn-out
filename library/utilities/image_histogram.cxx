/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/image_histogram.h>

#include <utilities/rgb2yuv.h>

#include <vil/vil_math.h>
#include <vnl/vnl_math.h>
#include <algorithm>
#include <limits>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_image_histogram_cxx__
VIDTK_LOGGER("image_histogram_cxx");


namespace vidtk
{

image_histogram
::image_histogram( bool skip_zero,
                   Image_Histogram_Type histType)
  : n_(0), h_(),
    skip_zero_( skip_zero ),
    hist_type_( histType )
{
}

image_histogram
::image_histogram( const image_histogram& that)
{
  *this = that;
}

image_histogram &
image_histogram
::operator=( image_histogram const & that)
{
  this->h_.clear();
  this->h_.deep_copy ( that.h_ );
  this->n_ = that.n_;
  this->skip_zero_ = that.skip_zero_;

  return *this;
}

image_histogram
::~image_histogram()
{
  this->h_.clear();
}

void
image_histogram
::normalize()
{
  //Normalize the histogram, i.e. h_ /= n_;
  if( n_ > 0 )
    vil_math_scale_values( h_, 1/n_ );
}

void
image_histogram
::init_bins(unsigned int number_of_planes, std::vector<unsigned> const & n_bins)
{
  if( this->hist_type_ == VIDTK_AUTO )
  {
    if( number_of_planes == 1 )
    {
      this->hist_type_ = VIDTK_GRAYSCALE;
    }
    else
    {
      this->hist_type_ = VIDTK_YUV;
    }
  }
  LOG_ASSERT( number_of_planes == n_bins.size(),
              "Channels in the input image and dimensionality of the "
              "histogram are inconsistent." );
  LOG_ASSERT( (this->hist_type_ == VIDTK_GRAYSCALE && n_bins.size() == 1)
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
}

void
image_histogram
::get_bin_location( std::vector<double> const & channels,
                    unsigned * bins ) const
{
  assert( channels[0]<=1.0 && channels[0]>=0.0 );
  bins[0] = static_cast<unsigned>( channels[0] * (h_.ni()-1) );
  if( this->hist_type_ != VIDTK_GRAYSCALE )
  {
    assert( channels[1]<=1.0 && channels[1]>=0.0 );
    assert( channels[2]<=1.0 && channels[2]>=0.0 );
    bins[1] = static_cast<unsigned>( channels[1] * (h_.nj()-1) );
    bins[2] = static_cast<unsigned>( channels[2] * (h_.nplanes()-1) );
  }
}

void
image_histogram
::update(std::vector<double> const & channels, double weight)
{
  unsigned bins[] = {0,0,0};
  this->get_bin_location( channels, bins );
  double &bin_value = h_( bins[0], bins[1], bins[2] );
  if( vnl_math::isnan( weight ) )
  {
    LOG_ERROR( "The mask used for computing histogram contains NaNs." );
  }
  else
  {
    bin_value += weight;
    n_ += weight;
  }
}

double image_histogram
::compare( image_histogram const & other ) const
{
  if((this->mass() == 0) || (other.mass() == 0) )
    return 0.0;
  if( this->hist_type_ != other.type() )
    return 0.0;

  return this->compare( other.get_h() );
}

double image_histogram
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
        sum += std::min( this->h_(i,j,k), others_h(i,j,k) );
      }
    }
  }

//  return 1.0*sum / std::min( this->mass(), other.mass() );
  return sum ;
}

} // namespace
