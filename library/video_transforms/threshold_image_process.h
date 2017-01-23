/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_threshold_image_process_h_
#define vidtk_threshold_image_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <vector>

namespace vidtk
{


template <typename PixType>
class threshold_image_process
  : public process
{
public:
  typedef threshold_image_process self_type;

  threshold_image_process( std::string const& name );

  virtual ~threshold_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_source_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  vil_image_view<bool> thresholded_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, thresholded_image );

private:
  enum threshold_type { ABSOLUTE, Z_SCORE, ADAPTIVE_Z_SCORE, PERCENTILES };
  enum threshold_condition { ABOVE, BELOW };

  config_block config_;

  vil_image_view<PixType> const* src_image_;

  PixType threshold_;
  int z_radius_;
  threshold_type type_;
  threshold_condition condition_;
  bool persist_output_;
  std::vector<double> percentiles_;

  vil_image_view<bool> output_image_;
};


} // end namespace vidtk


#endif // vidtk_threshold_image_process_h_
