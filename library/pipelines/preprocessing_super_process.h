/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_preprocessing_super_process_h_
#define vidtk_preprocessing_super_process_h_

#include <pipeline_framework/super_process.h>

#include <process_framework/pipeline_aid.h>
#include <process_framework/process.h>

#include <utilities/config_block.h>
#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>

#include <vil/vil_image_view.h>

#include <string>

namespace vidtk
{

template< class PixType >
class preprocessing_super_process_impl;

/*! \brief Preprocessing Super Process
 *
 * This video preprocessor contains a temporal frame downsampler, a video
 * enhancer, and other processes such as a metadata burn-in detector. It
 * is useful for performing a few standard operations at the start of a
 * larger video processing pipeline.
 */
template< class PixType >
class preprocessing_super_process
: public super_process
{
public:

  typedef preprocessing_super_process< PixType > self_type;

  preprocessing_super_process( std::string const& name );
  virtual ~preprocessing_super_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& blk );
  virtual bool initialize();

  virtual process::step_status step2();
  virtual bool step();

  /// Set pipeline to monitor, used by internal downsampler.
  virtual bool set_pipeline_to_monitor( const vidtk::pipeline* p );

  void set_image( vil_image_view< PixType > const& );
  VIDTK_INPUT_PORT( set_image, vil_image_view< PixType > const& );

  void set_timestamp( vidtk::timestamp const& );
  VIDTK_INPUT_PORT( set_timestamp, vidtk::timestamp const& );

  void set_metadata( vidtk::video_metadata const& );
  VIDTK_INPUT_PORT( set_metadata, vidtk::video_metadata const& );

  vil_image_view< PixType > image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, image );

  vil_image_view< bool > mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, mask );

  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  vidtk::video_metadata metadata() const;
  VIDTK_OUTPUT_PORT( vidtk::video_metadata, metadata );

private:

  preprocessing_super_process_impl< PixType >* impl_;
};

}

#endif
