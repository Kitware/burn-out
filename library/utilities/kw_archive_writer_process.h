/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _KW_ARCHIVE_WRITER_PROCESS_H_
#define _KW_ARCHIVE_WRITER_PROCESS_H_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include "kw_archive_writer.h"



namespace vidtk
{

class kw_archive_writer;


// ----------------------------------------------------------------
/** WK Archive format writer process.
 *
 *
 */
class kw_archive_writer_process
  : public process
{
public:
  typedef kw_archive_writer_process self_type;
  kw_archive_writer_process(vcl_string const& name);
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
  IN_PORT     (image,                 vil_image_view < vxl_byte > const&);
  IN_PORT_OPT (src_to_ref_homography, vidtk::image_to_image_homography const&);
  IN_PORT_OPT (src_to_utm_homography, vidtk::image_to_utm_homography const&);
  IN_PORT_OPT (src_to_wld_homography, vidtk::image_to_plane_homography const&);
  IN_PORT     (wld_to_utm_homography, vidtk::plane_to_utm_homography const&);
  IN_PORT     (world_units_per_pixel, double);

#undef IN_PORT


private:
  config_block m_config;

  // Config data
  bool m_enable;
  vcl_string m_file_path;
  vcl_string m_file_name;
  bool m_allow_overwrite;

  // input data areas
  vidtk::image_to_plane_homography  m_src_to_wld_h;
  vidtk::plane_to_utm_homography   m_wld_to_utm_h;

  // Archive writer object
  vidtk::kw_archive_writer * m_archive_writer;

  // Archive writer data set
  vidtk::kw_archive_writer::data_set_homog m_data_set;

}; // end class kw_archive_writer_process

} // end namespace


#endif /* _KW_ARCHIVE_WRITER_PROCESS_H_ */
