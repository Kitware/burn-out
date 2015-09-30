/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_writer_process_h_
#define vidtk_track_writer_process_h_

#include <vcl_fstream.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>

#include <tracking/vsl/image_object_io.h>
#include <tracking/image_object.h>
#include <tracking/track.h>
#include <tracking/tracking_keys.h>

class vsl_b_ofstream;

namespace vidtk
{


/// Writes track entries to file.
///
/// The output can be of different formats.  The format is chosen by
/// the parameter \c format.  See the documentation of the various
/// write_formatX methods (e.g. write_format0) for descriptions of the
/// format.
///
/// Note that this process writes entire tracks, not partial.  This is
/// a suitable process for writing terminated tracks but innapropriate for
/// active tracks
///

class track_writer_process
  : public process
{
public:
  typedef track_writer_process self_type;

  track_writer_process( vcl_string const& name );

  ~track_writer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_image_objects( vcl_vector< image_object_sptr > const& objs );

  VIDTK_INPUT_PORT( set_image_objects, vcl_vector< image_object_sptr > const& );

  void set_tracks( vcl_vector< track_sptr > const& trks );

  VIDTK_INPUT_PORT( set_tracks, vcl_vector< track_sptr > const& );

  void set_timestamp( timestamp const& ts );

  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  bool is_disabled() const;

private:
  bool fail_if_exists( vcl_string const& filename );

  // documentation in .cxx
  void write_format_0();

  // documentation in .cxx
  void write_format_vsl();

  // documentation in .cxx
  void write_format_2();

  void write_format_mit();

  // Parameters

  config_block config_;

  bool disabled_;
  unsigned format_;
  vcl_string filename_;
  bool allow_overwrite_;
  bool suppress_header_;
  vcl_string path_to_images_;//used only in mit
  int x_offset_;
  int y_offset_;

  // Cached input

  vcl_vector< image_object_sptr > const* src_objs_;
  vcl_vector< track_sptr > const* src_trks_;
  timestamp ts_;

  // Internal data

  vcl_ofstream fstr_;
  vsl_b_ofstream* bfstr_;

  //timestamp derivation parameter

  double frequency_;

  // Class flag to indicate whether to write lat/lon
  // in the world locations in KW18
  bool write_lat_lon_for_world_;
};


} // end namespace vidtk


#endif // vidtk_track_writer_process_h_
