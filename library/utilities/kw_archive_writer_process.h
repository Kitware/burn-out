/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _KW_ARCHIVE_WRITER_PROCESS_H_
#define _KW_ARCHIVE_WRITER_PROCESS_H_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include "kw_archive_index_writer.h"



namespace vidtk
{

template< class PixType >
class kw_archive_writer;


// ----------------------------------------------------------------
/** WK Archive format writer process.
 *
 *
 */
template<class PixType>
class kw_archive_writer_process
  : public process
{
public:
  typedef kw_archive_writer_process self_type;
  kw_archive_writer_process(std::string const& name);
  virtual ~kw_archive_writer_process();

  // -- process interface --
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // -- process inputs --
#define IN_PORT(N,T) void set_input_ ## N (T val); VIDTK_INPUT_PORT ( set_input_ ## N, T )
#define IN_PORT_OPT(N,T) void set_input_ ## N (T val); VIDTK_OPTIONAL_INPUT_PORT ( set_input_ ## N, T )

  IN_PORT     (timestamp,             vidtk::timestamp const&);
  IN_PORT     (image,                 vil_image_view < PixType > const&);
  IN_PORT_OPT (src_to_ref_homography, vidtk::image_to_image_homography const&);
  IN_PORT     (corner_points, vidtk::video_metadata const&);
  IN_PORT     (world_units_per_pixel, double);

#undef IN_PORT
#undef IN_PORT_OPT


private:
  config_block config_;

  // Config data
  bool disable_;
  std::string output_directory_;
  std::string base_filename_;
  bool separate_meta_;
  std::string mission_id_;
  std::string stream_id_;
  bool compress_image_;

  // input data areas
  vidtk::timestamp timestamp_;
  vidtk::video_metadata corner_points_;
  vil_image_view< PixType > image_;
  vidtk::image_to_image_homography frame_to_ref_;
  double gsd_;

  // Archive writer object
  vidtk::kw_archive_index_writer< PixType > * archive_writer_;
}; // end class kw_archive_writer_process

} // end namespace


#endif /* _KW_ARCHIVE_WRITER_PROCESS_H_ */
