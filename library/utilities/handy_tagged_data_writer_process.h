/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _HANDY_TAGGED_DATA_WRITER_PROCESS_H_
#define _HANDY_TAGGED_DATA_WRITER_PROCESS_H_

#include "tagged_data_writer_process.h"

namespace vidtk
{

// ----------------------------------------------------------------
/** Handy tagged data writer.
 *
 * This class represents a tagged data writer that is easy to insert
 * and activate without requiring any configuration entries.
 *
 * Typical usage is as follows:
 *
\code

  process_smart_pointer< handy_tagged_data_writer_process > proc_writer;

  proc_writer = new handy_tagged_data_writer_process ("temp_writer");

  p->add (proc_writer);
  proc_writer->set_filename ("temp_writer_tagged_file.tdf"); // name of your choice
  proc_writer->set_enable();

  p->connect( proc_world_coord_sp->source_timestamp_port(),      proc_writer->set_input_timestamp_port() );
  proc_writer->set_timestamp_connected (true);

  p->connect( proc_world_coord_sp->source_image_port(),          proc_writer->set_input_image_port() );
  proc_writer->set_image_connected ();

  p->connect( proc_world_coord_sp->src_to_ref_homography_port(), proc_writer->set_input_src_to_ref_homography_port() );
  proc_writer->set_src_to_ref_homography_connected ();

\endcode
 */
class handy_tagged_data_writer_process
  : public tagged_data_writer_process
{
public:
  //typedef handy_tagged_data_writer_process self_type;

  handy_tagged_data_writer_process(vcl_string const& name)
    : tagged_data_writer_process (name),
      m_localEnable(false) { }

  virtual ~handy_tagged_data_writer_process() { }

  virtual bool initialize()
  {
    this->m_enabled = m_localEnable;  // needed?

    // Overwrite default file name if one has been specified.
    if ( ! this->m_localFilename.empty() )
    {
      m_filename = m_localFilename;  // needed?
      base_io_process::initialize(); // start initialization at the base class
    }

    return true;
  }


  void set_filename (vcl_string const& name) { m_localFilename = name; }
  void set_enable (bool v) { m_localEnable = v; }
  virtual bool set_params(config_block const& blk)
  { 
     this->m_enabled = m_localEnable;

    // Overwrite default file name if one has been specified.
    if ( ! this->m_localFilename.empty() )
    {
      m_filename = m_localFilename;
      //base_io_process::initialize(); // start initialization at the base class
    }
    return true; 
  }

#define MDRW_INPUT(N,T,W,I)                     \
public:                                         \
  void set_ ## N ## _connected ()               \
  {                                             \
    tagged_data_writer_process::set_ ## N ## _connected (true);             \
    set_ ## N ## _force_enabled ();             \
  }

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT


private:
  vcl_string m_localFilename;
  bool m_localEnable;

}; // end class handy_tagged_data_writer_process

} // end namespace

#endif /* _HANDY_TAGGED_DATA_WRITER_PROCESS_H_ */



