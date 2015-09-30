/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_difference_process_h_
#define vidtk_image_difference_process_h_

#include <vcl_vector.h>

#include <vil/vil_image_view.h>
#include <tracking/fg_image_process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// \brief The parameters controlling the image-difference process
///
/// \sa image_difference_process
struct image_difference_process_params
{
  /// how many images must accumulate-- set to one greater than the number 
  /// of images required to separate two movers-- should probably be stated
  /// in terms of time instead of number of images
  unsigned image_spacing;

  /// grayscale difference such that >= is a true difference
  double diff_threshold;

  /// use a second image difference to try to remove object shadows.
  bool shadow_removal;
};


/// Find moving object by pixelwise comparison of registered images.
///
/// This class will accept a sequence of registered images and internally
/// buffer them; the output is the difference image.  The difference image
/// is only meaningful after sufficient time has passed that moving objects
/// between two images will no longer overlap; before that, the difference
/// image is blank.


template < class PixType >
class image_difference_process
  : public fg_image_process<PixType>
{
public:
  typedef image_difference_process self_type;

  image_difference_process( vcl_string const& name );
  
  ~image_difference_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// The next image.
  ///
  /// This should be called prior to each call to step(); the image object
  /// is cloned and the clone is managed by the internal buffer.

  void set_source_image( vil_image_view<PixType> const& src );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );


  void set_latest_image( vil_image_view<PixType> const& img );

  VIDTK_INPUT_PORT( set_latest_image, vil_image_view<PixType> const& );

  void set_second_image( vil_image_view<PixType> const& img );

  VIDTK_INPUT_PORT( set_second_image, vil_image_view<PixType> const& );

  void set_third_image( vil_image_view<PixType> const& img );

  VIDTK_INPUT_PORT( set_third_image, vil_image_view<PixType> const& );

  /// the difference image between the current source image and the lagged
  /// reference image

  virtual vil_image_view<bool> const& fg_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );


  /// \brief A representative image of the background.
  virtual vil_image_view<PixType> bg_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, bg_image );

protected:

  void clear_buffer();

  config_block config_;

  /// internal buffer of past images.
  vcl_vector< vil_image_view<PixType>* > buffer_;


  vil_image_view<PixType> const* src_img1_;
  vil_image_view<PixType> const* src_img2_;
  vil_image_view<PixType> const* src_img3_;

  /// current difference image.
  vil_image_view<bool> diff_image_;

  // cached to avoid mallocing on every run
  vil_image_view<bool> diff_short_;

  image_difference_process_params* param_;

};

} // namespace vidtk

#endif
