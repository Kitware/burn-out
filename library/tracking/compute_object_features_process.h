/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef compute_object_features_process_h_
#define compute_object_features_process_h_

#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/image_object.h>
#include <utilities/buffer.h>


namespace vidtk
{
/// Compute various features for the set of MODs in the current frame. 
///
/// Currently computing: 
///  - Mean intensity 
///  - TODO: color_histogram (move into this process)

template < class PixType >
class compute_object_features_process
  : public process
{
public:
  typedef compute_object_features_process self_type;

  compute_object_features_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_source_objects( vcl_vector< image_object_sptr > const& objs );

  VIDTK_INPUT_PORT( set_source_objects, vcl_vector< image_object_sptr > const& );

  void set_source_image( vil_image_view<PixType> const& image );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );
  
  vcl_vector< image_object_sptr > const& objects() const;

  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

protected:
  config_block config_;

  void compute_intensity_mean_var( image_object_sptr const & obj,
                                   float &, float & );

  // Input data
  vcl_vector< image_object_sptr > const* src_objs_;
  vil_image_view<PixType> const* image_;

  // Output state
  vcl_vector< image_object_sptr > objs_;

  // Parameters
  bool compute_mean_intensity_;
  bool compute_histogram_;
};


} // end namespace vidtk

#endif // compute_object_features_process_h_

