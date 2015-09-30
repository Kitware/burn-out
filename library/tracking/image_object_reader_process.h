/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_reader_process_h_
#define vidtk_image_object_reader_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/image_object.h>
#include <utilities/timestamp.h>
#include <vcl_fstream.h>

class vsl_b_ifstream;

namespace vidtk
{


/// Reads image_object entries from a file.
///
/// In conjunction with image_object_writer_process, this can be used
/// to stage pipeline executions.  For example, run MOD, write the
/// result to file, and then run tracking by reading the MODs from
/// file.  This allows you experiment with tracking algorithms and
/// parameters without re-computing the MODs.
///
/// The input can be of different formats.  The format is chosen by
/// the parameter \c format.  See the documentation of the various
/// read_formatX methods (e.g. read_format_vsl) for descriptions of
/// the formats.
///
class image_object_reader_process
  : public process
{
public:
  typedef image_object_reader_process self_type;

  image_object_reader_process( vcl_string const& name );

  ~image_object_reader_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_timestamp( vidtk::timestamp const& ts );

  VIDTK_INPUT_PORT( set_timestamp, vidtk::timestamp const& );

  /// The timestamp that was read from the serialized stream.
  vidtk::timestamp const& timestamp() const;

  VIDTK_OUTPUT_PORT( vidtk::timestamp const&, timestamp );

  vcl_vector< image_object_sptr > const& objects() const;

  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

  bool is_disabled() const;

private:
  // documentation in .cxx
  bool read_format_vsl();
  bool read_format_0();

  // Parameters
  config_block config_;

  bool disabled_;
  unsigned format_;
  vcl_string filename_;

  // Cached input

  vidtk::timestamp const* inp_ts_;

  // Internal data and cached output

  vidtk::timestamp read_ts_;
  vcl_vector< image_object_sptr > objs_;

  vsl_b_ifstream* bfstr_;
  vcl_ifstream txtfstr_;

  bool b_new_frame_started;
  unsigned last_frame_num_;
};


} // end namespace vidtk


#endif // vidtk_image_object_reader_process_h_
