/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_filter_image_process_h_
#define vidtk_filter_image_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vil/vil_image_view.h>


namespace vidtk
{


/// Filter out images (binary for now) that don't meet some criteria.
///
/// This class will filter out images that don't match certain criteria.
/// For example, it can filter out foregound detections in a binary image
/// if they suddenly shoot to high value unexpectedly. We track the mean &
/// sigma of (%age of) pixels in the image and then threshold sigma on z-score.


class filter_image_process
  : public process
{
public:
  typedef filter_image_process self_type;

  filter_image_process( std::string const& name );

  ~filter_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset(){ return initialize(); };

  virtual bool step();

  void set_source_image( vil_image_view< bool > const& src );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view< bool > const& );

  bool isgood() const;

  VIDTK_OUTPUT_PORT( bool, isgood );

protected:
  // Input data
  vil_image_view< bool > const* src_img_;

  // Parameters

  config_block config_;

  double mean_pcent_fg_pixels_;
  double alpha_;
  double threshold_;
  bool disabled_;
  unsigned init_window_;
  unsigned init_counter_;

  // Output state

  bool isgood_;
};


} // end namespace vidtk


#endif // vidtk_filter_image_process_h_
