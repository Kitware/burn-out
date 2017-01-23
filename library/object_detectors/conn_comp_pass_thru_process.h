/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_conn_comp_pass_thru_process_h_
#define vidtk_conn_comp_pass_thru_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking_data/image_object.h>
#include <utilities/homography.h>

#include <object_detectors/conn_comp_super_process.h>

namespace vidtk
{

template <class PixType>
class conn_comp_pass_thru_process
  : public process
{
public:
  typedef conn_comp_pass_thru_process self_type;
  typedef typename pixel_feature_array< PixType >::sptr_t feature_array_sptr;

  conn_comp_pass_thru_process( std::string const& name );
  virtual ~conn_comp_pass_thru_process();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

#define pass_thru_outputs(name, type) \
  VIDTK_PASS_THRU_PORT(name, type);

  conn_comp_sp_outputs(pass_thru_outputs)
  conn_comp_sp_optional_outputs(pass_thru_outputs)

#undef pass_thru_outputs

protected:

  // Internal Parameters
  config_block config_;
};

} // end namespace vidtk


#endif // vidtk_conn_comp_pass_thru_process_h_
