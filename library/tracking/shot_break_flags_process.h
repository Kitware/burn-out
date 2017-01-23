/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_shot_break_flags_process_h_
#define vidtk_shot_break_flags_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking_data/shot_break_flags.h>


namespace vidtk
{

class shot_break_flags_process
  : public process
{
public:
  typedef shot_break_flags_process self_type;

  shot_break_flags_process( std::string const& name );

  virtual ~shot_break_flags_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  virtual bool reset();

  void set_is_tracker_shot_break( bool img );
  VIDTK_INPUT_PORT( set_is_tracker_shot_break, bool );

  void set_is_homography_shot_break( bool ts );
  VIDTK_INPUT_PORT( set_is_homography_shot_break, bool );

  /// Set of new tracks that were created at the last step.
  shot_break_flags flags() const;
  VIDTK_OUTPUT_PORT( shot_break_flags, flags );

protected:
  bool is_tracker_sb;
  bool is_homog_sb;
  bool got_data;

  shot_break_flags flags_;
};

} // end namespace vidtk


#endif // vidtk_shot_break_flags_process_h_
