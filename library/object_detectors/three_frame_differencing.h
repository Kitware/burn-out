/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_three_frame_differencing_h_
#define vidtk_three_frame_differencing_h_

#include <vil/vil_image_view.h>

#include <utilities/external_settings.h>

namespace vidtk
{

#define settings_macro( add_param, add_array, add_enumr ) \
  add_enumr( \
    diff_type, \
    (ABSOLUTE) (ZSCORE) (ADAPTIVE_ZSCORE), \
    ABSOLUTE, \
    "Choose between absolute, zscore, or adaptive_zscore differencing. " \
    "This is the method used to threshold the computed, final motion " \
    "difference image." ); \
  add_enumr( \
    operation_type, \
    (UNSIGNED_SUM) (SIGNED_SUM) (UNSIGNED_MIN) (UNSIGNED_ZSCORE_SUM) \
    (UNSIGNED_ADAPTIVE_ZSCORE_SUM) (UNSIGNED_ADAPTIVE_ZSCORE_MIN), \
    UNSIGNED_SUM, \
    "The frame combination operation method for different types of three " \
    "frame differencing. Can be unsigned_sum, signed_sum, unsigned_min, " \
    "unsigned_zscore_sum, or unsigned_adaptive_zscore_sum. This is the " \
    "method by which the raw difference image is generated before " \
    "thresholding. See class documentation." ); \
  add_param( \
    z_threshold, \
    float, \
    2.5f, \
    "The difference threshold at which a *z_score* difference is " \
    "considered indicative of a object difference" ); \
  add_param( \
    threshold, \
    float, \
    20.0f, \
    "The difference threshold at which a difference is considered " \
    "indicative of a object difference" ); \
  add_param( \
    nan_value, \
    int, \
    -1, \
    "The special value used to indicate that a pixel is invalid " \
    "(outside the valid region when warping, etc). If all the " \
    "components of a pixel have this value, then the pixel is " \
    "considered to be invalid." ); \
  add_array( \
    jitter_delta, \
    unsigned, \
    2, \
    "0 0", \
    "The image differencing will occur using these delta_i and " \
    "delta_j jitter parameters.  The jitter essentially compensates " \
    "for registration errors.  A jitter of \"1 2\" allows +/-1 pixel " \
    "error in the i-axis and +/-2 pixel error in the j-axis." ); \
  add_param( \
    z_radius, \
    float, \
    200.0f, \
    "Radius of the box in which to compute locally adaptive z-scores. " \
    "For each pixel the zscore statistics are computed seperately " \
    "within a box that is 2*radius+1 on a side and centered on the " \
    "pixel." ); \
  add_param( \
    compute_unprocessed_mask, \
    bool, \
    false, \
    "Whether to produce a valid unprocessed pixels mask or not. The " \
    "mask records pixels (w/ true value) that are not supposed to " \
    "produce valid motion segmentation." ); \
  add_param( \
    use_threads, \
    bool, \
    false, \
    "Use up to an extra 2 threads to speed up differencing" ); \
  add_param( \
    min_pixel_difference_for_z_score, \
    float, \
    0.0f, \
    "If the image difference is below this value, it is considered " \
    "background" ); \
  add_param( \
    min_zscore_stdev, \
    float, \
    0.0f, \
    "When computing the zscore image, enforce this minimum standard " \
    "deviation of the difference values. This should help suppress " \
    "false alarms in absence of any true movers (with high difference " \
    "values)." ); \
  add_param( \
    subsample_step, \
    unsigned, \
    2, \
    "The pixel sample step used for estimating image properties (means, " \
    "sigmas) when computing them for zscore computations." ); \

init_external_settings3( frame_differencing_settings, settings_macro );

#undef settings_macro

/// \brief Find moving objects by pixelwise comparison of registered images.
///
/// Images should be presented to functions within this class in their
/// temporal order, ie, image1 (A) should usually be the frame furthest in the
/// past and image3 (C) should be the current (most recent) frame.
///
/// This class can compute the 3 frame differencing according to a variety
/// of equations, given images A, B, and C, the motion estimate for frame
/// C is given by the following:
///
/// unsigned_sum (default):
///  = | | A - C | + | C - B | - | A - B | |
///
/// signed_sum:
///  = ( Y + X - |Y| + |X| )
///
///  where X = ( C - A ) + | C - B | - | A - B |
///    and Y = ( C - A ) - | C - B | + | A - B |
///
/// unsigned min:
///  = min( | C - A |, | C - B | )
///
/// unsigned adaptive_zscore_min:
///  = min( azscore( C - A ), azscore( C - B ) )
///
///  where azscore( X ) = | ( X - E[X] ) / sigma( local_subregion( X ) ) |
///
/// unsigned_zscore_sum:
///  = | zscore( A - C ) + zscore( C - B ) - zscore( A - B ) |
///
///  where zscore( X ) = | ( X - E[X] ) / sigma( X ) |
///
/// unsigned adaptive_zscore_sum:
///  = | azscore( A - C ) + azscore( C - B ) - azscore( A - B ) |
///
///  where azscore( X ) = | ( X - E[X] ) / sigma( local_subregion( X ) ) |
///
/// Note: these images will be scaled slightly differently. For both
/// zscore varients, the values will be in zscore space. For unsigned
/// sum mode, raw values will be produced but will be about double
/// the actual motion difference (ie, if you have a white car with a
/// pixel magnitude of 255 driving on a completely black background,
/// the diff image for the car for frame C will be about 510). For
/// signed_sum, they will be about 4 times the difference, so 1020
/// in the above example (or -1020 for a completely black object on
/// a white background). Scaling is not yet normalized in order to
/// save computational resources and prevent an extra operand.
///
/// This difference image can then be thresholded to create a fg mask
/// via absolute, zscore, or adaptive_zscore methods.
template< typename InputType, typename OutputType >
class three_frame_differencing
{
public:

  typedef vil_image_view< InputType > input_image_t;
  typedef vil_image_view< OutputType > diff_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef frame_differencing_settings settings_t;

  three_frame_differencing() {}
  ~three_frame_differencing() {}

  bool configure( const settings_t& settings );

  bool process_frames(
    const input_image_t& image1,
    const input_image_t& image2,
    const input_image_t& image3,
    mask_image_t& fg_image );

  bool process_frames(
    const input_image_t& image1,
    const input_image_t& image2,
    const input_image_t& image3,
    diff_image_t& diff_image );

  bool process_frames(
    const input_image_t& image1,
    const input_image_t& image2,
    const input_image_t& image3,
    mask_image_t& fg_image,
    diff_image_t& diff_image );

private:

  void unsigned_sum_diff(
    const input_image_t& A,
    const input_image_t& B,
    const input_image_t& C,
    diff_image_t& diff_image );

  void signed_sum_diff(
    const input_image_t& A,
    const input_image_t& B,
    const input_image_t& C,
    diff_image_t& diff_image );

  void unsigned_min_diff(
    const input_image_t& A,
    const input_image_t& B,
    const input_image_t& C,
    diff_image_t& diff_image );

  void suppress_nan_regions_from_diff(
    const input_image_t& image,
    diff_image_t& diff_image );

  void compute_zscore_fg_mask(
    diff_image_t& diff_image,
    mask_image_t& mask_image );

  void compute_absolute_fg_mask(
    const diff_image_t& diff_image,
    mask_image_t& mask_image );

  settings_t settings_;

  diff_image_t tmp_image1_;
  diff_image_t tmp_image2_;

};

} // namespace vidtk

#endif
