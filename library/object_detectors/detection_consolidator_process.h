/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_detection_consolidator_process_h_
#define vidtk_detection_consolidator_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <object_detectors/detector_implementation_base.h>

#include <tracking_data/image_object.h>

#include <boost/scoped_ptr.hpp>

#include <vector>

namespace vidtk
{

/// \brief Consolidate image objects from multiple detectors.
///
/// The multiple streams of image objects can be combined in a
/// variety of ways. Additionally, source ID tags can be added
/// to each image object based on which ports the image objects
/// are coming from.
class detection_consolidator_process
  : public process
{
public:
  typedef detection_consolidator_process self_type;
  typedef image_object_sptr detection_sptr;
  typedef std::vector< image_object_sptr > detection_list_t;

  detection_consolidator_process( std::string const& name );
  virtual ~detection_consolidator_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_motion_detections( detection_list_t const& dets );
  VIDTK_INPUT_PORT( set_motion_detections, detection_list_t const& );

  void set_projected_motion_detections( detection_list_t const& dets );
  VIDTK_INPUT_PORT( set_projected_motion_detections, detection_list_t const& );

  void set_appearance_detections_1( detection_list_t const& dets );
  VIDTK_INPUT_PORT( set_appearance_detections_1, detection_list_t const& );

  void set_appearance_detections_2( detection_list_t const& dets );
  VIDTK_INPUT_PORT( set_appearance_detections_2, detection_list_t const& );

  void set_appearance_detections_3( detection_list_t const& dets );
  VIDTK_INPUT_PORT( set_appearance_detections_3, detection_list_t const& );

  void set_appearance_detections_4( detection_list_t const& dets );
  VIDTK_INPUT_PORT( set_appearance_detections_4, detection_list_t const& );

  detection_list_t merged_detections() const;
  VIDTK_OUTPUT_PORT( detection_list_t, merged_detections );

  detection_list_t merged_projected_detections() const;
  VIDTK_OUTPUT_PORT( detection_list_t, merged_projected_detections );

private:

  // Parameters
  config_block config_;
  bool disabled_;
  enum{ MERGE, PREFER_APPEARANCE } mode_;
  double suppr_overlap_per_;

  // Possible Inputs
  detection_list_t motion_dets_;
  detection_list_t motion_proj_dets_;

  detection_list_t appearance_dets1_;
  detection_list_t appearance_dets2_;
  detection_list_t appearance_dets3_;
  detection_list_t appearance_dets4_;

  // Outputs
  detection_list_t merged_dets_;
  detection_list_t merged_proj_dets_;

  // Other variables
  boost::scoped_ptr< set_type_functor > motion_labeler_;

  boost::scoped_ptr< set_type_functor > appearance_labeler1_;
  boost::scoped_ptr< set_type_functor > appearance_labeler2_;
  boost::scoped_ptr< set_type_functor > appearance_labeler3_;
  boost::scoped_ptr< set_type_functor > appearance_labeler4_;
};


} // end namespace vidtk


#endif // vidtk_image_object_writer_process_h_
