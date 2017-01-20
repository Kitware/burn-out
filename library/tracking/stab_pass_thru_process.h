/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_stab_pass_thru_process_h_
#define vidtk_stab_pass_thru_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <tracking_data/shot_break_flags.h>
#include <utilities/homography.h>
#include <utilities/video_metadata.h>
#include <kwklt/klt_track.h>

namespace vidtk
{

// Best. Process. Ever.
template <class PixType>
class stab_pass_thru_process
  : public process
{
public:
  typedef stab_pass_thru_process self_type;

  stab_pass_thru_process( std::string const& name );
  ~stab_pass_thru_process();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  VIDTK_PASS_THRU_PORT( image, vil_image_view< PixType > );
  VIDTK_PASS_THRU_PORT( timestamp, timestamp );
  VIDTK_PASS_THRU_PORT( metadata, video_metadata );
  VIDTK_PASS_THRU_PORT( metadata_vector, std::vector< video_metadata >  );
  VIDTK_PASS_THRU_PORT( homography, image_to_image_homography );
  VIDTK_PASS_THRU_PORT( klt_tracks, std::vector< klt_track_ptr > );
  VIDTK_PASS_THRU_PORT( shot_break_flags, vidtk::shot_break_flags );
  VIDTK_PASS_THRU_PORT( mask, vil_image_view< bool > );

protected:

  // Internal Parameters
  config_block config_;
};

} // end namespace vidtk


#endif // vidtk_stab_pass_thru_process_h_
