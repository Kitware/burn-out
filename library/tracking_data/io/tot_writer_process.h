/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_writer_process_h_
#define vidtk_tot_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/io/tot_writer.h>

namespace vidtk
{


/// Writes Target Object Type (TOT) probabilities on tracks to a file.
///
/// Currently the only format supported is a simple ASCII text format
/// that is compatible with the *.pvo.txt files used in VIRAT Phase 2.
///
/// Note that this process writes the TOT probabilites once per track.
/// This is a suitable process for writing terminated tracks but
/// innapropriate for active tracks
///
class tot_writer_process
  : public process
{
public:
  typedef tot_writer_process self_type;

  tot_writer_process( std::string const& name );
  virtual ~tot_writer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_tracks( std::vector< track_sptr > const& trks );
  VIDTK_INPUT_PORT( set_tracks, std::vector< track_sptr > const& );

  bool is_disabled() const;

private:
  bool fail_if_exists( std::string const& filename );

  void clear();

  // Parameters
  config_block config_;

  bool disabled_;
  std::string filename_;
  bool allow_overwrite_;

  tot_writer writer_;

  // Cached input
  std::vector<track_sptr> src_trks_;
  bool src_trks_set_;
};


} // end namespace vidtk


#endif // vidtk_tot_writer_process_h_
