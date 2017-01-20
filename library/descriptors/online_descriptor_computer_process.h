/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_online_descriptor_computer_process_h_
#define vidtk_online_descriptor_computer_process_h_


// VXL includes
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

// VIDTK includes
#include <tracking_data/shot_break_flags.h>

#include <utilities/video_metadata.h>
#include <utilities/video_modality.h>
#include <utilities/homography.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

// Descriptor includes
#include <descriptors/online_descriptor_generator.h>
#include <descriptors/multi_descriptor_generator.h>
#include <descriptors/descriptor_filter.h>
#include <descriptors/net_descriptor_computer.h>

// Standard library includes
#include <vector>

// Boost includes
#include <boost/scoped_ptr.hpp>

namespace vidtk
{


/// A process for computing all raw descriptors. Currently this creates a descriptor
/// generator for each type of descriptor that's enabled, and jointly runs them
/// in a threaded sub-system.
template <typename PixType>
class online_descriptor_computer_process
  : public process
{

public:

  typedef online_descriptor_computer_process self_type;

  online_descriptor_computer_process( std::string const& name );
  virtual ~online_descriptor_computer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  virtual bool dump_descriptor_configs( const std::string& directory );
  virtual bool flush();

  void set_source_image( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_source_tracks( std::vector<track_sptr> const& trks );
  VIDTK_INPUT_PORT( set_source_tracks, std::vector<track_sptr> const& );

  void set_source_metadata( video_metadata const& meta );
  VIDTK_INPUT_PORT( set_source_metadata, video_metadata const& );

  void set_source_metadata_vector( std::vector< video_metadata > const& meta_vec );
  VIDTK_INPUT_PORT( set_source_metadata_vector, std::vector< video_metadata > const& );

  void set_source_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_source_image_to_world_homography( image_to_utm_homography const& homog );
  VIDTK_INPUT_PORT( set_source_image_to_world_homography, image_to_utm_homography const& );

  void set_source_modality( video_modality modality );
  VIDTK_INPUT_PORT( set_source_modality, video_modality );

  void set_source_shot_break_flags( shot_break_flags sb_flags );
  VIDTK_INPUT_PORT( set_source_shot_break_flags, shot_break_flags );

  void set_source_gsd( double gsd );
  VIDTK_INPUT_PORT( set_source_gsd, double );

  std::vector<track_sptr> active_tracks() const;
  VIDTK_OUTPUT_PORT( std::vector<track_sptr>, active_tracks );

  raw_descriptor::vector_t descriptors() const;
  VIDTK_OUTPUT_PORT( raw_descriptor::vector_t, descriptors );

private:

  typedef boost::shared_ptr< online_descriptor_generator > generator_sptr;
  typedef track::vector_t track_sptr_vector;

  // Is this process disabled?
  bool disabled_;

  // The multi-descriptor generating process
  multi_descriptor_generator joint_generator_;
  multi_descriptor_settings joint_generator_settings_;

  // A vector of descriptors to output
  raw_descriptor::vector_t descriptors_;

  // A buffer to hold the process inputs
  frame_data_sptr inputs_;
  track_sptr_vector input_tracks_;

  // In IR mode, should we use IR specific settings?
  bool use_ir_settings_;

  // Was the last known frame EO or IR?
  bool was_last_eo_;

  // If we receive an invalid timestamp should we not process the frame?
  bool require_valid_timestamp_;

  // Last valid timestamp
  vidtk::timestamp last_timestamp_;

  // If we receive an invalid image should we not process the frame?
  bool require_valid_image_;

  // Minimum shot size required
  unsigned minimum_shot_size_;

  // Shot size counter
  unsigned shot_size_counter_;

  // Internal parameters/settings
  config_block config_;

  // Internal descriptor filters
  //descriptor_filter filter_;
  boost::scoped_ptr< net_descriptor_computer > net_computer_;

  // Step all internal generators, returns true on success
  bool step_joint_generator();

  // Push all descriptors from the internal generator to process outputs
  void collect_descriptors();
};


} // end namespace vidtk


#endif // vidtk_online_descriptor_computer_process_h_
