/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_super_process_h_
#define vidtk_tot_super_process_h_

#include <pipeline_framework/super_process.h>

#include <tracking_data/track.h>
#include <tracking_data/shot_break_flags.h>

#include <utilities/config_block.h>
#include <utilities/video_metadata.h>
#include <utilities/video_modality.h>
#include <utilities/homography.h>
#include <utilities/ring_buffer.h>

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/tot_collector_process.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>

#include <vil/vil_image_resource.h>

namespace vidtk
{

template< typename PixType >
class tot_super_process_impl;

/// \brief This process produce target-object type classifications for the input
/// tracks, and appends them to each track when applicable.
///
/// Any raw descriptors and managed events produced as a by-product of
/// target-object type classifications will also be outputted for optional
/// use by downstream processes.
template< typename PixType >
class tot_super_process
  : public super_process
{
public:
  typedef tot_super_process self_type;

  tot_super_process( std::string const& name );
  ~tot_super_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual process::step_status step2();

  void set_source_image( vil_image_view<PixType> const& image );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  //compiler did not seem to like vil_image_resource_sptr const
  void set_source_image_res( vil_image_resource_sptr image );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_image_res, vil_image_resource_sptr );

  void set_source_tracks( std::vector<track_sptr> const& active_tracks );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_tracks, std::vector<track_sptr> const& );

  void set_source_metadata( video_metadata const& meta );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_metadata, video_metadata const& );

  void set_source_timestamp( timestamp const& ts );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_source_image_to_world_homography( image_to_utm_homography const& homog );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_image_to_world_homography, image_to_utm_homography const& );

  void set_source_modality( video_modality modality );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_modality, video_modality );

  void set_source_shot_break_flags( shot_break_flags sb_flags );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_shot_break_flags, shot_break_flags );

  void set_source_gsd( double gsd );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_gsd, double );

  std::vector<track_sptr> output_tracks() const;
  VIDTK_OUTPUT_PORT( std::vector<track_sptr>, output_tracks );

  raw_descriptor::vector_t descriptors() const;
  VIDTK_OUTPUT_PORT( raw_descriptor::vector_t, descriptors );

private:

  boost::scoped_ptr< tot_super_process_impl<PixType> > impl_;

};

} // namespace vidtk

#endif // vidtk_tot_super_process_h_
