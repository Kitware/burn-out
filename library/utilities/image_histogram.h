/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_IMAGE_HISTOGRAM_H
#define INCL_IMAGE_HISTOGRAM_H

#include <vcl_vector.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

enum Image_Histogram_Type { VIDTK_AUTO, VIDTK_RGB, VIDTK_YUV, VIDTK_GRAYSCALE };

/// \brief Class to compute color and grayscale image histogram.
///
/// 
template < class imT, class maskT >
class image_histogram
{
public:
  image_histogram();

  image_histogram( image_histogram const & );

  image_histogram & operator=( image_histogram const & );

  image_histogram( vil_image_view<imT> const & image,
                   vcl_vector<unsigned> const & n_bins,
                   bool skip_zero = false,
                   Image_Histogram_Type histType = VIDTK_AUTO);

  image_histogram( vil_image_view<imT> const & image,
                   vil_image_view<maskT> const & mask,
                   vcl_vector<unsigned> const & n_bins,
                   bool skip_zero = false,
                   Image_Histogram_Type histType = VIDTK_AUTO );

  ~image_histogram();

  Image_Histogram_Type type() const{ return hist_type_; };

  void set_type(Image_Histogram_Type t){ hist_type_ = t; };

  vil_image_view<double> get_h() const { return h_; }

  void set_h(const vil_image_view<double> &h) { h_ = h; }

  double mass() const { return n_; }

  void set_mass(double mass) { n_ = mass; }

  void add( vil_image_view<imT> const & image,
            vil_image_view<maskT> const * mask = NULL );

  double compare( const vil_image_view<double> & others_h ) const;

  double compare( const image_histogram & other ) const;

  void init( vil_image_view<imT> const & image,
             vcl_vector<unsigned> const & n_bins,
             vil_image_view<maskT> const * mask = NULL );
private:

  void get_bin_location( vcl_vector<unsigned char> const & channels,
                         vcl_vector<unsigned> & bins ) const;

  void rgb2yuv_wrapper( imT R, imT G, imT B, vcl_vector<unsigned char> & yuv ) const;

  /// Mass in the histogram
  double n_;

  /// Accumulator array
  vil_image_view<double> h_;

  /// Should we not count rgb000 pixels?
  bool skip_zero_;

  /// RGB,YUV,etc.
  Image_Histogram_Type hist_type_;
};

} // - end namespace

#endif
