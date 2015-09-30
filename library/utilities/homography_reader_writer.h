/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _HOMOGRAPHY_READER_WRITER_H_
#define _HOMOGRAPHY_READER_WRITER_H_

#include <utilities/base_reader_writer.h>
#include <utilities/homography.h>
#include <utilities/timestamp.h>
#include <vul/vul_sprintf.h>
#include <vcl_cstdio.h>
#include <vcl_ostream.h>
#include <vcl_iomanip.h>
#include <vcl_sstream.h>


namespace vidtk
{

// --------------------------------------------------------------- -
/** Homography reader/writer base class.
 *
 *
 */
class homography_reader_writer_base
{
public:
  virtual ~homography_reader_writer_base() { }


protected:
  homography_reader_writer_base() { }


  void write_reference (vcl_ostream& str, vidtk::timestamp ts)
  {
    str << ts.frame_number() << " "
        << vcl_setprecision(20) << ts.time() << " ";
  }


  vidtk::timestamp read_reference (vcl_istream& str, vidtk::timestamp)
  {
    double time;
    unsigned int frame;

    str >> frame >> time;

    vidtk::timestamp ts;
    ts.set_time (time);
    ts.set_frame_number (frame);
    return ts;
  }


  void write_reference (vcl_ostream& str, vidtk::utm_zone_t utm)
  {
    str << utm.zone() << " "
        << utm.is_north() << " ";
  }


  vidtk::utm_zone_t read_reference (vcl_istream& str, vidtk::utm_zone_t)
  {
    int zone;
    bool north;

    str >> zone >> north;

    vidtk::utm_zone_t utm;
    utm.set_zone (zone);
    utm.set_is_north (north);

    return utm;
  }


  void write_reference (vcl_ostream& str, vidtk::plane_ref_t /*ref*/)
  {
    str << "[plane] ";
  }


  vidtk::plane_ref_t read_reference (vcl_istream& str, vidtk::plane_ref_t)
  {
    vcl_string junk;
    str >> junk;

    return vidtk::plane_ref_t();
  }


  // write transformation matrix
  void write_base(vcl_ostream& str, homography * homog)
  {
    vcl_stringstream working_stream;
    vcl_string data_str;
    size_t idx;

    str << homog->is_valid() << " " << homog->is_new_reference() << " ";

    working_stream  << vcl_setprecision(20)
                    << homog->get_transform().get_matrix();
    data_str = working_stream.str();

    // replace NL with space so it is all on one line.
    for (int i = 0; i < 4; i++)
    {
      idx = data_str.find("\n");
      if (idx == vcl_string::npos)
      {
        break;
      }

      data_str.replace(idx, 1, " ");
    }

    str << data_str;
  }


  void read_base(vcl_istream& str, homography * homog)
  {
    vidtk::homography::transform_t trans;
    bool valid;
    bool new_ref;

    str >> valid >> new_ref;

    // Read values
    vnl_matrix_fixed < double, 3, 3 > matrix;
    str >> matrix;

    trans.set(matrix); // install the matrix into the transform
    homog->set_transform(trans); // install transform into homography

    homog->set_valid(valid);
    homog->set_new_reference(new_ref);

  }

};   // end class homography_reader_writer_base



// ================================================================
/** reader/writer for homography.
 *
 *
 */
template < class T >
class homography_reader_writer_T
  : public vidtk::base_reader_writer_T < T >,
    private homography_reader_writer_base
{
public:
  homography_reader_writer_T (const char * TAG, T * obj)
    :  base_reader_writer_T < T > (TAG, obj)
  { }

  virtual base_reader_writer* clone() const { return new homography_reader_writer_T < T >(*this); }

  virtual ~homography_reader_writer_T() { }


// ----------------------------------------------------------------
/** Write out homography.
 *
 *
 */
  virtual void write_object(vcl_ostream& str)
  {
    str << this->entry_tag_string() << " " ;

    // format transform
    write_base (str, base_reader_writer_T<T>::datum_addr());

    // format src and dest ref's (timestamps)
    write_reference (str, base_reader_writer_T<T>::datum_addr()->get_source_reference() );
    write_reference (str, base_reader_writer_T<T>::datum_addr()->get_dest_reference() );
    str << vcl_endl;
  }


 // ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(vcl_ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  [valid] [new-ref] [3x3 transform] [source-ref] [dest-ref]"
        << vcl_endl;
  }


// ----------------------------------------------------------------
/** Read in homography.
 *
 *
 */
  virtual int read_object(vcl_istream& str)
  {
    vcl_string input_tag;

    str >> input_tag;

    read_base (str, base_reader_writer_T<T>::datum_addr());

    base_reader_writer_T<T>::datum_addr()->set_source_reference(
      read_reference (str, base_reader_writer_T<T>::datum_addr()->get_source_reference())
      );

    base_reader_writer_T<T>::datum_addr()->set_dest_reference(
      read_reference (str, base_reader_writer_T<T>::datum_addr()->get_dest_reference())
      );

    // Test for errors
    if (str.fail() )
    {
      return 1;
    }

    this->set_valid_state (true);
    return 0;
  }

};  // end template homography_reader_writer


//
// Define the specialized reader/writer classes
//
#define DEFCLASS(B,TAG)                                                 \
class B ## _reader_writer : public homography_reader_writer_T < B >     \
{                                                                       \
public:                                                                 \
  B ## _reader_writer(B * obj)                                          \
    : homography_reader_writer_T < B >(TAG, obj) { }                    \
};

// Define reader/writer classes
DEFCLASS (image_to_image_homography, "I2IH")
DEFCLASS (image_to_plane_homography, "I2PH")
DEFCLASS (plane_to_image_homography, "P2IH")
DEFCLASS (image_to_utm_homography, "I2UTMH")
DEFCLASS (utm_to_image_homography, "UTM2IH")
DEFCLASS (plane_to_utm_homography, "P2UTMH")
DEFCLASS (utm_to_plane_homography, "UTM2PH")

#undef DEFCLASS

} // end namespace


#endif /* _HOMOGRAPHY_READER_WRITER_H_ */
