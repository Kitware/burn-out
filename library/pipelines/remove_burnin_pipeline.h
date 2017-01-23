/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_remove_burnin_pipeline_h_
#define vidtk_remove_burnin_pipeline_h_

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
class remove_burnin_pipeline_impl;

/*! \brief Pipeline which detects and removes metadata burn-in.
 *
 * This pipeline will detect different types of on-screen displays
 * (a.k.a. metadata burnin, heads-up displays) that can appear on
 * FMV video, and then inpaint the source imagery using the detected
 * burnin masks.
 */
template< class PixType >
class remove_burnin_pipeline
: public super_process
{
public:

  typedef remove_burnin_pipeline< PixType > self_type;

  remove_burnin_pipeline( std::string const& name );
  virtual ~remove_burnin_pipeline();

  virtual config_block params() const;
  virtual bool set_params( config_block const& blk );
  virtual bool initialize();
  virtual process::step_status step2();
  virtual bool step();

  void set_image( vil_image_view< PixType > const& );
  VIDTK_INPUT_PORT( set_image, vil_image_view< PixType > const& );

  void set_timestamp( vidtk::timestamp const& );
  VIDTK_INPUT_PORT( set_timestamp, vidtk::timestamp const& );

  void set_metadata( vidtk::video_metadata const& );
  VIDTK_INPUT_PORT( set_metadata, vidtk::video_metadata const& );

  vil_image_view< PixType > original_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, original_image );

  vil_image_view< PixType > inpainted_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, inpainted_image );

  vil_image_view< bool > detected_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, detected_mask );

  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  vidtk::video_metadata metadata() const;
  VIDTK_OUTPUT_PORT( vidtk::video_metadata, metadata );

private:

  remove_burnin_pipeline_impl< PixType >* impl_;
};

}

#endif
