/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_reader_process_h_
#define vidtk_image_object_reader_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking_data/image_object.h>
#include <utilities/timestamp.h>

#include <vector>
#include <string>

namespace vidtk
{

class image_object_reader;

/**
 * @brief Reads image_object entries from a file.
 *
 * This process supplies a vector of image objects (object detections)
 * from a file for the current timestamp.
 *
 * In conjunction with image_object_writer_process, this can be used
 * to stage pipeline executions.  For example, run MOD, write the
 * result to file, and then run tracking by reading the MODs from
 * file.  This allows you experiment with tracking algorithms and
 * parameters without re-computing the MODs.
 *
 * The input can be of different formats.  The format Automatically
 * determined by the reader class.
 *
 * This process can be used in two scenarios. The first is sourcing
 * image objects for a stand alone analysis tool. In this case, the
 * input timestamp should be unset, or set to a default constructed
 * (invalid) timestamp. In this case, the image object vectors are
 * returned in the sequence they are read from the file. If there are
 * no detections for a given frame in the data file, that frame will
 * be skipped in the output. Only frames that have detections will be
 * seen on the output. The step() call fails at EOF.
 *
 * The other scenario is injecting detections into a tracking
 * pipeline. In this case. the input timestamp is set to the current
 * frame being processed. This reader process will return the
 * detections for that frame. If there are no detections for a frame,
 * an empty vector is returned by this process. in this mode, the
 * process does not fail at EOF, but keeps returning success to allow
 * the tracker pipeline to continue to the end of the video.
 */
class image_object_reader_process
  : public process
{
public:
  typedef image_object_reader_process self_type;

  image_object_reader_process( std::string const& name );
  virtual ~image_object_reader_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_timestamp( vidtk::timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, vidtk::timestamp const& );

  /// The timestamp that was read from the serialized stream.
  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  std::vector< image_object_sptr > objects() const;
  VIDTK_OUTPUT_PORT( std::vector< image_object_sptr >, objects );

  bool is_disabled() const;

private:
  // Parameters
  config_block config_;

  bool disabled_;
  bool eof_seen_;
  std::string format_;
  std::string filename_;
  vidtk::timestamp input_ts_;

  image_object_reader * the_reader_;

  // Internal data and cached output
  vidtk::timestamp output_ts_;
  std::vector< image_object_sptr > output_objs_;

  std::vector< image_object_sptr > file_objs_;
  vidtk::timestamp file_timestamp_;
};


} // end namespace vidtk


#endif // vidtk_image_object_reader_process_h_
