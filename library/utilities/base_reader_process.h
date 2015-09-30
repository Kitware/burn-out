/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_base_reader_process_h_
#define vidtk_base_reader_process_h_


#include <utilities/base_io_process.h>


namespace vidtk
{


// ----------------------------------------------------------------
/** Reader process abstract base class.
 *
 * This class represents the base class for processes that read data
 * from a file.
 */
class base_reader_process
  : public base_io_process
{
public:

  base_reader_process( vcl_string const& name, vcl_string const& type );
  virtual ~base_reader_process();


protected:
  virtual bool initialize(base_io_process::internal_t);
  virtual int error_hook( int status);

private:
  void open_file();
  int do_file_step();

};

} // end namespace vidtk


#endif // vidtk_base_reader_process_h_
