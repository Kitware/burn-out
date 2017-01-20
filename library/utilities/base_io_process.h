/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_base_io_process_h_
#define vidtk_base_io_process_h_

#include <vector>
#include <fstream>

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
  virtual void write_all_objects(std::ostream& str);

  // Read inputs
  virtual int read_all_objects(std::istream& str);

  // Write headers
  virtual void write_all_headers(std::ostream& str);

  std::string const& get_file_name() const;



protected:
  virtual bool initialize_internal() = 0;
  group_data_reader_writer * get_group();

  base_io_process(std::string const & name, std::string const & type);
  virtual ~base_io_process();

  virtual int pre_step_hook();
  virtual void post_step_hook();

  virtual int do_file_step() = 0;
  virtual void open_file() = 0;
  std::fstream& file_stream();

  config_block m_configBlock;
  std::string m_filename;
  group_data_reader_writer m_dataGroup;
  bool m_appendFile;
  bool m_enabled;


private:
  std::fstream m_fileStream;

};


} // end namespace vidtk


#endif // vidtk_base_io_process_h_
