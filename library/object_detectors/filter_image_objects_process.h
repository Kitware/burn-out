/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_filter_image_objects_process_h_
#define vidtk_filter_image_objects_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <object_detectors/ghost_detector.h>
#include <tracking_data/image_object.h>
#include <tracking_data/shot_break_flags.h>
#include <utilities/homography.h>
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

  filter_image_objects_process( std::string const& name );

  virtual ~filter_image_objects_process();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  /// \brief Set the objects to be filtered.
  ///
  /// Note that the objects themselves will *not* be copied.
  void set_source_objects( std::vector< image_object_sptr > const& objs );
  VIDTK_INPUT_PORT( set_source_objects, std::vector< image_object_sptr > const& );

  /// \brief Source image corresponding to the frame at which MODs are detected.
  ///
  /// Currently being used to identify ghost/shadow MODs.
  void set_source_image( vil_image_view< PixType > const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  /// Used to filter objects if they don't contain a pixel above
  /// threshold (used with standard thresholding to implement a
  /// pesudo-hysteresis thresholding).
  void set_binary_image( vil_image_view< bool > const& img );
  VIDTK_INPUT_PORT( set_binary_image, vil_image_view< bool > const& );

  /// Used to calculate the UTM and lat/lon points from image
  /// so we can geo-filter the image_objects.
  void set_src_to_utm_homography( image_to_utm_homography const& H );
  VIDTK_INPUT_PORT( set_src_to_utm_homography, image_to_utm_homography const& );

  // Used to handle unusable frames
  void set_shot_break_flags( shot_break_flags const& sbf );
  VIDTK_OPTIONAL_INPUT_PORT( set_shot_break_flags, shot_break_flags const& );

  /// Vector of filtered/updated image objects
  std::vector< image_object_sptr > objects() const;
  VIDTK_OUTPUT_PORT( std::vector< image_object_sptr >, objects );

protected:
  bool filter_out_min_image_area( image_object_sptr const& obj ) const;
  bool filter_out_max_image_area( image_object_sptr const& obj ) const;
  bool filter_out_min_area( image_object_sptr const& obj ) const;
  bool filter_out_max_area( image_object_sptr const& obj ) const;
  bool filter_out_min_occupied_bbox( image_object_sptr const& obj ) const;
  bool filter_out_max_aspect_ratio( image_object_sptr const& obj ) const;
  bool filter_out_used_in_track( image_object_sptr const& obj ) const;
  bool filter_out_ghost_gradients( image_object_sptr const& obj ) const;
  bool filter_out_binary_image( image_object_sptr const& obj ) const;
  bool filter_out_intersects_region( image_object_sptr const& obj ) const;
  bool filter_out_with_depth( image_object_sptr const& obj, const vil_image_view<vxl_byte> &depth ) const;
  bool filter_out_max_occlusion( image_object_sptr const& obj ) const;

  void setup_depth_filtering(std::string const&);

  ghost_detector< PixType > * grad_ghost_detector_;

  // Input data
  std::vector< image_object_sptr > const* src_objs_;
  vil_image_view< PixType > const* src_img_;
  vil_image_view< bool > const* binary_img_;
  image_to_utm_homography H_src2utm_;
  shot_break_flags shot_break_flags_;
  bool sbf_valid_;

  // Parameters
  config_block config_;

  // See TXX file for parameter comments.
  double min_image_area_;
  double max_image_area_;
  double min_area_;
  double max_area_;
  double max_aspect_ratio_;
  double min_occupied_bbox_;
  bool filter_used_in_track_;
  bool filter_binary_;
  bool used_in_track_;
  bool disabled_;
  bool ghost_gradients_;
  float min_ghost_var_;
  unsigned pixel_padding_;
  bool geofilter_enabled_;
  vgl_polygon<double> geofilter_polygon_;
  float max_occlusion_;
  bool filter_unusable_frames_;

  double depth_filter_thresh_;
  std::vector< std::string > depth_file_names_;
  unsigned int depth_file_index_;

  // Output state
  std::vector< image_object_sptr > objs_;

private:
  void set_aoi( std::string const& regions );
  void add_region (std::string const & points );
};

} // end namespace vidtk

#endif // vidtk_filter_image_objects_process_h_
