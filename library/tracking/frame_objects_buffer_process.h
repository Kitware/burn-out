/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_objects_buffer_process_h_
#define vidtk_frame_objects_buffer_process_h_

#include "frame_objects_buffer_process.h"

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/image_object.h>
#include <utilities/timestamp.h>

#include <vcl_vector.h>
#include <vcl_map.h>

/// \file Contains frame level data items with random read/write access.

namespace vidtk
{

class frame_objects_buffer_process_impl;
typedef vcl_vector< image_object_sptr > objs_type;
typedef vcl_map< timestamp, objs_type > frame_objs_type;

class frame_objects_buffer_process
  : public process
{
public:
  typedef frame_objects_buffer_process self_type;

  frame_objects_buffer_process( vcl_string const& name );

  ~frame_objects_buffer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize(){ return true; };

  virtual bool reset(){ return true; };

  virtual process::step_status step2();

  virtual bool step(){ return (step2() == SUCCESS) ? true : false; };

  void set_current_objects( objs_type const& objs );
  VIDTK_INPUT_PORT( set_current_objects, objs_type const& );

  void set_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  void set_updated_frame_objects( frame_objs_type const& );
  VIDTK_INPUT_PORT( set_updated_frame_objects, frame_objs_type );

  frame_objs_type const& frame_objects() const;
  VIDTK_OUTPUT_PORT( frame_objs_type const&, frame_objects );

private:
  frame_objects_buffer_process_impl * impl_;
};

} // end namespace vidtk

#endif // vidtk_frame_item_process_h_
