/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_image_view.h>

#include <utilities/ring_buffer.h>

/// @file illumination_normalization.h
/// @brief Functions for performing simple frame-to-frame illumination normalization


namespace vidtk
{


/** Abstract base class for simple illumination normalizers */
template < class PixType >
class illumination_normalization
{

public:

  illumination_normalization() {}
  virtual ~illumination_normalization() {}

  /** Process a new image, and return the normalized result.
   * @param img The input image
   * @param deep_copy Should the be deep copied?
   * @return The normalized image */
  virtual vil_image_view<PixType> operator()( vil_image_view<PixType> const& img,
                                              bool deep_copy = false ) = 0;

  /** Reset the normalizer.*/
  virtual void reset() = 0;
};



/**Normalizes the illumination of an image based on the median of the pixel values.
 * This assumes the background makes up the majority of the intensity data and that
 * background content does not change much.  This assumption allows us to assume
 * the difference medians is related to differences in illumination.*/
template < class PixType >
class median_illumination_normalization : public illumination_normalization< PixType >
{
public:

  /** Constructor, sets internal settings for the median illumination normalizer.
   * @param use_fix_median Should we normalize the median illumination to
   * a fixed value, or the value from the first input frame?
   * @param fix_reference_median The fixed reference value, if enabled. */
  median_illumination_normalization( bool use_fix_median = false, double fix_reference_median = 0 )
  : reference_median_set_(false),
    reference_median_(fix_reference_median),
    use_fix_median_(use_fix_median)
  {
    this->reset();
  }

  vil_image_view<PixType> operator()( vil_image_view<PixType> const& img, bool deep_copy = false );

  void reset();

  double calculate_median( vil_image_view<PixType> const & img ) const;

protected:

  bool reference_median_set_;
  double reference_median_;
  //When enabled, we relight all images to have this median.
  //When disable, we use the first image after a reset called.
  bool use_fix_median_;
};



/**Normalizes the illumination of an image based on the mean of the pixel values
 * and on a windowed moving average of this mean. This assumes the background makes
 * up the majority of the intensity data and that background content only changes
 * very slowly.*/
template < class PixType >
class mean_illumination_normalization : public illumination_normalization< PixType >
{
public:

  /** Constructor, sets internal settings for the mean illumination normalizer.
   * @param window_length Size of the moving average window
   * @param sampling_rate Only sample every 1 in x pixels when computing image mean
   * @param min_illum_allowed Minimum mean illumination allowed
   * @param max_illum_allowed Maximum mean illumination allowed */
  mean_illumination_normalization( unsigned window_length = 20,
                                     unsigned sampling_rate = 4,
                                     double min_illum_allowed = 0.0,
                                     double max_illum_allowed = 1.0 )
  : window_length_( window_length ),
    sampling_rate_( sampling_rate ),
    min_illum_allowed_( min_illum_allowed ),
    max_illum_allowed_( max_illum_allowed )
  {
    this->reset();
  }

  vil_image_view<PixType> operator()( vil_image_view<PixType> const& img, bool deep_copy = false );

  void reset();

  double calculate_mean( vil_image_view<PixType> const& img ) const;

protected:

  unsigned window_length_;
  unsigned sampling_rate_;
  double min_illum_allowed_;
  double max_illum_allowed_;

  ring_buffer< double > illum_history_;

};

}
