/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_properties_extract_image_resolution_process_h_
#define vidtk_video_properties_extract_image_resolution_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <utility>


namespace vidtk
{


template <class PixType>
class extract_image_resolution_process
  : public process
{
public:
  typedef extract_image_resolution_process self_type;
  typedef std::pair<unsigned, unsigned> resolution_t;

  extract_image_resolution_process( std::string const& name );
  ~extract_image_resolution_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_image, vil_image_view<PixType> const& );

  resolution_t resolution() const;
  VIDTK_OUTPUT_PORT( resolution_t, resolution );

private:

  // Parameters
  config_block config_;

  // Input
  vil_image_view<PixType> input_image_;

  // Output
  resolution_t resolution_;
};


} // end namespace vidtk


#endif // vidtk_extract_image_resolution_process_h_
