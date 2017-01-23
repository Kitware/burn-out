/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_writer_process_h_
#define vidtk_image_object_writer_process_h_

#include <fstream>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking_data/image_object.h>
#include <utilities/timestamp.h>

class vsl_b_ofstream;

namespace vidtk
{

  class image_object_writer;

/// Writes image_object entries to file.
///
/// The output can be of different formats.  The format is chosen by
/// the parameter \c format.  See the documentation of the various
/// write_formatX methods (e.g. write_format0) for descriptions of the
/// format.
///
/// \note This process is disabled by default, and must be enabled by
/// setting the parameter \c disabled to \c false.
///
/// Note that as far as streaming is concerned, the objects at each
/// step() are independent from objects in the previous step.  That
/// is, if you pass the same pointer to in two different calls to
/// step, the object will be written twice into the stream (as opposed
/// to the stream "remembering" that they are the same object).
class image_object_writer_process
  : public process
{
public:
  typedef image_object_writer_process self_type;

  image_object_writer_process( std::string const& name );
  virtual ~image_object_writer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_image_objects( std::vector< image_object_sptr > const& objs );
  VIDTK_INPUT_PORT( set_image_objects, std::vector< image_object_sptr > const& );

  void set_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  bool is_disabled() const;

private:
  // Parameters
  config_block config_;

  bool disabled_;

  // Cached input
  std::vector< image_object_sptr > src_objs_;
  timestamp ts_;

  // Internal data
  std::vector < image_object_writer * > the_writer_;
  std::string format_;
};


} // end namespace vidtk


#endif // vidtk_image_object_writer_process_h_
