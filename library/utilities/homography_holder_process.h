/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_HOMOGRAPHY_HOLDER_PROCESS_H_
#define _VIDTK_HOMOGRAPHY_HOLDER_PROCESS_H_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/homography.h>
#include <utilities/config_block.h>

// \file A process that feeds a fixed homography to the pipeline.
//
// The homography is provided by the user through a config option
// and typically remains unchanged through the life of the pipeline.

namespace vidtk
{

class homography_holder_process
  : public process
{
public:
  typedef homography_holder_process self_type;

  homography_holder_process( vcl_string const& pname );

  ~homography_holder_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  image_to_plane_homography const& homography_ref_to_wld();
  VIDTK_OUTPUT_PORT( image_to_plane_homography const&, homography_ref_to_wld );

  // More output ports (e.g. plane_to_image, utm_to_image etc.) can be
  // added when required. Another possibility is to template this class
  // over various types of homographies.

private:
  config_block config_;

  image_to_plane_homography H_ref2wld_;

}; // class

} // namespace

#endif // _VIDTK_HOMOGRAPHY_HOLDER_PROCESS_H_
