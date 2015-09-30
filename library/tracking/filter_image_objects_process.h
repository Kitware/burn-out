/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_filter_image_objects_process_h_
#define vidtk_filter_image_objects_process_h_

#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/ghost_detector.h>
#include <tracking/image_object.h>
#include <vil/vil_image_view.h>

namespace vidtk
{


/// Filter out objects that don't meet some criteria.
///
/// This class will filter out image objects (MODs) that don't match
/// certain criteria.  For example, it can filter out objects whose
/// area is below a threshold hold, or above a threshold.
///
/// The filter can apply multiple criteria in a single step, but it is
/// implementation defined as to the order in which the criteria are
/// applied.

template <class PixType>
class filter_image_objects_process
  : public process
{
public:
  typedef filter_image_objects_process self_type;

  filter_image_objects_process( vcl_string const& name );

  ~filter_image_objects_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  /// \brief Set the objects to be filtered.
  ///
  /// Note that the objects themselves will *not* be copied.
  void set_source_objects( vcl_vector< image_object_sptr > const& objs );
  VIDTK_INPUT_PORT( set_source_objects, vcl_vector< image_object_sptr > const& );

  /// \brief Source image corresponding to the frame at which MODs are detected.
  ///
  /// Currently being used to identify ghost/shadow MODs.
  void set_source_image( vil_image_view< PixType > const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  vcl_vector< image_object_sptr > const& objects() const;
  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

protected:
  bool filter_out_min_area( image_object_sptr const& obj ) const;
  bool filter_out_max_area( image_object_sptr const& obj ) const;
  bool filter_out_min_occupied_bbox( image_object_sptr const& obj ) const;
  bool filter_out_max_aspect_ratio( image_object_sptr const& obj ) const;
  bool filter_out_used_in_track( image_object_sptr const& obj ) const;
  bool filter_out_ghost_gradients( image_object_sptr const& obj ) const;

  ghost_detector< PixType > * grad_ghost_detector_;

  // Input data
  vcl_vector< image_object_sptr > const* src_objs_;
  vil_image_view< PixType > const* src_img_;

  // Parameters

  config_block config_;

  // See TXX file for parameter comments.
  double min_area_;
  double max_area_;
  double max_aspect_ratio_;
  double min_occupied_bbox_;
  bool filter_used_in_track_;
  bool used_in_track_;
  bool disabled_;
  bool ghost_gradients_;
  float min_ghost_var_;
  unsigned pixel_padding_;

  // Output state

  vcl_vector< image_object_sptr > objs_;
};


} // end namespace vidtk


#endif // vidtk_filter_image_objects_process_h_
