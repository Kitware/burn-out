/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _GSD_READER_WRITER_H_
#define _GSD_READER_WRITER_H_

#include <utilities/base_reader_writer.h>

#include <vcl_cstdio.h>
#include <vcl_iomanip.h>



namespace vidtk
{

// ----------------------------------------------------------------
/** GSD writer class
 *
 *
 */
class gsd_reader_writer
  : public base_reader_writer_T < double >
{
public:
  gsd_reader_writer(double * gsd)
    : base_reader_writer_T< double >("GSD", gsd)
  { }

  virtual base_reader_writer* clone() const { return new gsd_reader_writer(*this); }

  virtual ~gsd_reader_writer() { }

// ----------------------------------------------------------------
/** Write object to stream.
 *
 *
 */
  virtual void write_object(vcl_ostream& str)
  {
    str << this->entry_tag_string() << " "
        << vcl_setprecision(20)
        << *datum_addr()
        << vcl_endl;
  }


// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(vcl_ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  gsd-value" << vcl_endl;
  }


// ----------------------------------------------------------------
/** Read object from stream.
 *
 * Read an object from the stream expecting the format from the
 * writer method.
 *
 * @param[in] str - stream to read from
 *
 * @return The number of objects not read in. This would be either 0
 * or 1, since we are only a single object reader.
 *
 * @retval 0 - object read correctly
 * @retval 1 - object not recognized
 * @throws vcl_runtime_error - if format error.
 */
 virtual int read_object(vcl_istream& str)
  {
    vcl_string input_tag;
    double local_gsd;

    set_valid_state (false);

    str >> input_tag
        >> local_gsd;

    // Test for stream error
    if (str.fail())
    {
      return 1;
    }

    *datum_addr() = local_gsd;

    set_valid_state (true);
    return 0;
  }


}; // end class gsd_reader_writer

} // end namespace

#endif /* _GSD_READER_WRITER_H_ */
