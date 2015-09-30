/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_compute_color_histogram_process_h_
#define vidtk_compute_color_histogram_process_h_

#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/image_object.h>
#include <utilities/buffer.h>


namespace vidtk
{
/// Compute color histograms for the set of MODs in the current frame. 
///
/// Color histogram will be used for now for fast correspondence b/w MODs 
/// and tracks. For the model update we'd need to use correlation around the
/// MODs, so just histogram won't suffice for that. 
///
/// IMPORTANT NOTE: 
///     This class is deprecated. Pleaes use compute_object_features_process
///     instead. In addition, image_color_histogram and image_color_histogram2
///     are also depcrecated. Please use image_histogram instead. 


template < class PixType >
class compute_color_histogram_process
  : public process
{
public:
  typedef compute_color_histogram_process self_type;

  compute_color_histogram_process( vcl_string const& name );

  ~compute_color_histogram_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief Set the objects to be filtered.
  ///
  /// Note that the objects themselves will *not* be copied.
  void set_source_objects( vcl_vector< image_object_sptr > const& objs );

  VIDTK_INPUT_PORT( set_source_objects, vcl_vector< image_object_sptr > const& );

  void set_source_image( vil_image_view<PixType> const& image );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );
  
  vcl_vector< image_object_sptr > const& objects() const;

  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

protected:
  // Input data
  vcl_vector< image_object_sptr > const* src_objs_;
  vil_image_view<PixType> const* image_;

  // Parameters

  config_block config_;

  // Output state

  vcl_vector< image_object_sptr > objs_;
};


} // end namespace vidtk


#endif // vidtk_compute_color_histogram_process_h_
