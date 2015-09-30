/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_base_io_process_h_
#define vidtk_base_io_process_h_

#include <vcl_vector.h>
#include <vcl_fstream.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/group_data_reader_writer.h>


namespace vidtk
{


// ----------------------------------------------------------------
/** Reader process abstract base class.
 *
 * This class represents the base class for processes that write
 * data to a file.
 *
 * @todo convert pre/post step hook and do_file_step to bool
 * return. Push changes to derived class.
 */
class base_io_process
  : public process
{
public:
  // typedef base_io_process self_type;


  // Process interface
  virtual config_block params() const;
  virtual bool set_params(config_block const&);
  virtual bool initialize();
  virtual bool step();

  vidtk::config_block& get_config();

  // add type to reader/writer list
  base_reader_writer * add_data_io(base_reader_writer const& obj);

  // Write outputs
  virtual void write_all_objects(vcl_ostream& str);

  // Read inputs
  virtual int read_all_objects(vcl_istream& str);

  // Write headers
  virtual void write_all_headers(vcl_ostream& str);

  vcl_string const& get_file_name() const;



protected:
  enum internal_t { INTERNAL };

  virtual bool initialize(base_io_process::internal_t) = 0;
  group_data_reader_writer * get_group();

  base_io_process(vcl_string const & name, vcl_string const & type);
  virtual ~base_io_process();

  virtual int pre_step_hook();
  virtual void post_step_hook();

  virtual int do_file_step() = 0;
  virtual void open_file() = 0;
  vcl_fstream& file_stream();

  config_block m_configBlock;
  vcl_string m_filename;
  group_data_reader_writer m_dataGroup;
  bool m_appendFile;
  bool m_enabled;


private:
  vcl_fstream m_fileStream;

};


} // end namespace vidtk


#endif // vidtk_base_io_process_h_
