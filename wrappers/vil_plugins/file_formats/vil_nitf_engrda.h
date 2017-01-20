/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#ifndef VIL_NITF_ENGRDA_H_
#define VIL_NITF_ENGRDA_H_

#include <vxl_config.h>
#include <string>
#include <complex>
#include <map>
#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

class vil_nitf_engrda;

//: Different types for the ENGRDA data
enum vil_nitf_engrda_type
{
  VIL_NITF_ENGRDA_TYPE_ASCII,
  VIL_NITF_ENGRDA_TYPE_BINARY,
  VIL_NITF_ENGRDA_TYPE_UINT_8,
  VIL_NITF_ENGRDA_TYPE_INT_8,
  VIL_NITF_ENGRDA_TYPE_UINT_16,
  VIL_NITF_ENGRDA_TYPE_INT_16,
  VIL_NITF_ENGRDA_TYPE_UINT_32,
  VIL_NITF_ENGRDA_TYPE_INT_32,
  VIL_NITF_ENGRDA_TYPE_UINT_64,
  VIL_NITF_ENGRDA_TYPE_INT_64,
  VIL_NITF_ENGRDA_TYPE_FLOAT,
  VIL_NITF_ENGRDA_TYPE_DOUBLE,
  VIL_NITF_ENGRDA_TYPE_COMPLEX_FLOAT,
  VIL_NITF_ENGRDA_TYPE_COMPLEX_DOUBLE,
  VIL_NITF_ENGRDA_TYPE_UNKNOWN
};


//: Container for a single data element in a set of ENGRD records
class vil_nitf_engrda_element: public vbl_ref_count
{
public:
  vil_nitf_engrda_element(void);
  ~vil_nitf_engrda_element(void);

  const std::string& get_label(void) const;
  vil_nitf_engrda_type get_type(void) const;

  //: Get the raw data pointer
  const void* get_data(void) const;

  //: Get the data formatted as a string.
  // Return false if type is not ASCII
  bool get_data(std::string &value);

  //: Get the units of the stored data
  // Return false if type is not ASCII
  const std::string& get_units(void) const;
private:
  friend class vil_nitf_engrda;

  std::string lbl_;
  size_t mtxc_;
  size_t mtxr_;
  char typ_;
  size_t dts_;
  std::string datu_;
  size_t datc_;
  union
  {
    char                *data_c_;
    vxl_uint_8          *data_u8_;
    vxl_int_8           *data_i8_;
    vxl_uint_16         *data_u16_;
    vxl_int_16          *data_i16_;
    vxl_uint_32         *data_u32_;
    vxl_int_32          *data_i32_;
    vxl_uint_64         *data_u64_;
    vxl_int_64          *data_i64_;
    float               *data_f32_;
    double              *data_f64_;
    std::complex<float>  *data_c32_;
    std::complex<double> *data_c64_;
  };
};
typedef vbl_smart_ptr<vil_nitf_engrda_element> vil_nitf_engrda_element_sptr;


//: Parser for NITF ENGRDA metadata blocks.
// This is implemented based on the defined standards in:
// "Compendium of Controlled Extensions (CE) for the National Imagery
// Transmission Format (NITF) Volume 1, Tagged Record Extensions, Version 4.0"
// Specifically Appendix N:
// "Engineering Data (ENGRD) Support Data Extension (SDE)"
//
// At the time of writing, January 2012, this document is available at:
// https://nsgreg.nga.mil/NSGDOC/documents/STDI-0002-1_4.0%20CE%20NITF.zip
class vil_nitf_engrda
{
public:
  typedef std::map<std::string,vil_nitf_engrda_element_sptr>::iterator iterator;
  typedef std::map<std::string,vil_nitf_engrda_element_sptr>::const_iterator const_iterator;

  vil_nitf_engrda();

  //: Parse engrda metadata from a given data buffer
  bool parse(const char* data, size_t len);

  //: Parse engrda metadata from a given data buffer
  bool parse(const std::string &data)
  {
    return this->parse(data.c_str(), data.size());
  }

  //: Rietrieve the Unique Source System Name
  const std::string& get_source(void) const;

  //: Retrieve a data element given it's label
  const vil_nitf_engrda_element_sptr get(const std::string &label) const;

  //: Retrieve an iterator to the start of all data elements
  vil_nitf_engrda::const_iterator begin(void) const;

  //: Retrieve an iterator to the end of all data elements
  vil_nitf_engrda::const_iterator end(void) const;
private:
  //: Unique Source System Name
  std::string resrc_;

  //: All records
  std::map<std::string, vil_nitf_engrda_element_sptr> elements_;
};

#endif
