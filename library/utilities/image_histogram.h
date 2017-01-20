/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_IMAGE_HISTOGRAM_H
#define INCL_IMAGE_HISTOGRAM_H

#include <vector>
#include <vil/vil_image_view.h>

namespace vidtk
{

enum Image_Histogram_Type { VIDTK_AUTO, VIDTK_RGB, VIDTK_YUV, VIDTK_GRAYSCALE };

class image_histogram;

template< class imT, class maskT >
class image_histogram_functions
{
public:
  static image_histogram generate(vil_image_view<imT> const & image,
                                  std::vector<unsigned> const & n_bins,
                                  bool skip_zero = false,
                                  Image_Histogram_Type histType = VIDTK_AUTO);
  static image_histogram generate(vil_image_view<imT> const & image,
                                  vil_image_view<maskT> const & mask,
                                  std::vector<unsigned> const & n_bins,
                                  bool skip_zero = false,
                                  Image_Histogram_Type histType = VIDTK_AUTO);
  static void init( image_histogram & hist,
                    vil_image_view<imT> const & image,
                    std::vector<unsigned> const & n_bins );
  static void add( image_histogram & hist,
                   vil_image_view<imT> const & image,
                   vil_image_view<maskT> const * mask = NULL );
  static void rgb2yuv_wrapper( imT R, imT G, imT B,
                               std::vector<double> & yuv );
};



/// \brief Class to compute color and grayscale image histogram.
///
///
class image_histogram
{
  template<class imT, class maskT > friend class image_histogram_functions;
public:
   image_histogram( bool skip_zero = false,
                    Image_Histogram_Type histType = VIDTK_AUTO);

  image_histogram( image_histogram const & );

  image_histogram & operator=( image_histogram const & );

  ~image_histogram();

  Image_Histogram_Type type() const{ return hist_type_; };

  void set_type(Image_Histogram_Type t){ hist_type_ = t; };

  vil_image_view<double> get_h() const { return h_; }

  void set_h(const vil_image_view<double> &h) { h_ = h; }

  double mass() const { return n_; }

  void set_mass(double _mass) { n_ = _mass; }

  double compare( const vil_image_view<double> & others_h ) const;

  double compare( const image_histogram & other ) const;

private:

  void get_bin_location( std::vector<double> const & channels,
                         unsigned * bins ) const;

  void init_bins(unsigned int number_of_planes, std::vector<unsigned> const & n_bins);

  void update(std::vector<double> const & channels, double weight);

  void normalize();

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
