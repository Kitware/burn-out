/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include "vil_nitf_engrda.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <limits>
#include <vbl/vbl_smart_ptr.txx>

template class vbl_smart_ptr<vil_nitf_engrda_element>;

vil_nitf_engrda_element
::vil_nitf_engrda_element(void)
: data_c_(NULL)
{
}


vil_nitf_engrda_element
::~vil_nitf_engrda_element(void)
{
  if( this->data_c_ )
  {
    delete[] this->data_c_;
    this->data_c_ = NULL;
  }
}


const std::string&
vil_nitf_engrda_element
::get_label(void) const
{
  return this->lbl_;
}


vil_nitf_engrda_type
vil_nitf_engrda_element
::get_type(void) const
{
  switch(this->typ_)
  {
    case 'A': return VIL_NITF_ENGRDA_TYPE_ASCII;
    case 'B': return VIL_NITF_ENGRDA_TYPE_BINARY;
    case 'I':
      switch(this->dts_)
      {
        case 1: return VIL_NITF_ENGRDA_TYPE_UINT_8;
        case 2: return VIL_NITF_ENGRDA_TYPE_UINT_16;
        case 4: return VIL_NITF_ENGRDA_TYPE_UINT_32;
        case 8: return VIL_NITF_ENGRDA_TYPE_UINT_64;
        default: return VIL_NITF_ENGRDA_TYPE_UNKNOWN;
      }
    case 'S':
      switch(this->dts_)
      {
        case 1: return VIL_NITF_ENGRDA_TYPE_INT_8;
        case 2: return VIL_NITF_ENGRDA_TYPE_INT_16;
        case 4: return VIL_NITF_ENGRDA_TYPE_INT_32;
        case 8: return VIL_NITF_ENGRDA_TYPE_INT_64;
        default: return VIL_NITF_ENGRDA_TYPE_UNKNOWN;
      }
    case 'R':
      switch(this->dts_)
      {
        case 4: return VIL_NITF_ENGRDA_TYPE_FLOAT;
        case 8: return VIL_NITF_ENGRDA_TYPE_DOUBLE;
        default: return VIL_NITF_ENGRDA_TYPE_UNKNOWN;
      }
    case 'C':
      switch(this->dts_)
      {
        case 4: return VIL_NITF_ENGRDA_TYPE_COMPLEX_FLOAT;
        case 8: return VIL_NITF_ENGRDA_TYPE_COMPLEX_DOUBLE;
        default: return VIL_NITF_ENGRDA_TYPE_UNKNOWN;
      }
    default: return VIL_NITF_ENGRDA_TYPE_UNKNOWN;
  }
}


const void*
vil_nitf_engrda_element
::get_data(void) const
{
  return this->data_c_;
}


bool
vil_nitf_engrda_element
::get_data(std::string &value)
{
  if( this->typ_ != 'A' )
  {
    return false;
  }
  std::stringstream ss;
  ss.write(this->data_c_, this->datc_);
  value = ss.str();
  return true;
}


const std::string&
vil_nitf_engrda_element
::get_units(void) const
{
  return this->datu_;
}

//***************************************************************************


vil_nitf_engrda
::vil_nitf_engrda()
{
}


//: Parse engrda metadata from a given data buffer
bool
vil_nitf_engrda
::parse(const char* data, size_t len)
{
  std::stringstream ss_block;
  ss_block.write(data, len);
  if( !ss_block ) return false;

  { // Read the resource name
    char buf[21];
    std::memset(buf, 0, 21);
    ss_block.read(buf, 20);
    this->resrc_ = buf;
  }

  size_t recnt;
  { // Read the record count
    char buf[4];
    std::memset(buf, 0, 4);
    ss_block.read(buf, 3);
    if( !ss_block ) return false;
    std::stringstream ss_conv;
    ss_conv << buf;
    ss_conv >> recnt;
  }

  // Read each record
  for( size_t i = 0; i < recnt; ++i )
  {
    vil_nitf_engrda_element_sptr element = new vil_nitf_engrda_element;
    {
      // 1: Parse the label length
      size_t ln;
      char buf_ln[3];
      std::memset(buf_ln, 0, 3);
      ss_block.read(buf_ln, 2);
      if( !ss_block ) return false;
      std::stringstream ss_conv(buf_ln);
      ss_conv >> ln;
      if( !ss_conv || ln == std::numeric_limits<size_t>::max() ) return false;

      // 2: Parse the label
      char *buf_lbl = new char[ln+1];
      std::memset(buf_lbl, 0, ln+1);
      ss_block.read(buf_lbl, ln);
      if( !ss_block )
      {
        delete[] buf_lbl;
        return false;
      }
      element->lbl_ = buf_lbl;
      delete[] buf_lbl;
    }
    { // 3: Parse the matrix column count
      char buf_mtxc[5];
      std::memset(buf_mtxc, 0, 5);
      ss_block.read(buf_mtxc, 4);
      if( !ss_block ) return false;
      std::stringstream ss_conv(buf_mtxc);
      ss_conv >> element->mtxc_;
      if( !ss_conv ) return false;
    }
    { // 4: Parse the matrix row count
      char buf_mtxr[5];
      std::memset(buf_mtxr, 0, 5);
      ss_block.read(buf_mtxr, 4);
      if( !ss_block ) return false;
      std::stringstream ss_conv(buf_mtxr);
      ss_conv >> element->mtxr_;
      if( !ss_conv ) return false;
    }
    { // 5: Parse the data type
      ss_block.read(&element->typ_, 1);
      if( !ss_block ) return false;
    }
    { // 6: Parse the data type size
      char buf_dts[2];
      std::memset(buf_dts, 0, 2);
      ss_block.read(buf_dts, 1);
      if( !ss_block ) return false;
      std::stringstream ss_conv(buf_dts);
      ss_conv >> element->dts_;
      if( !ss_conv ) return false;
    }
    { // 7: Parse the data units
      char buf_datu[3];
      std::memset(buf_datu, 0, 3);
      ss_block.read(buf_datu, 2);
      if( !ss_block ) return false;
      element->datu_ = buf_datu;
    }
    { // 8: Parse the data count
      char buf_datc[9];
      std::memset(buf_datc, 0, 9);
      ss_block.read(buf_datc, 8);
      if( !ss_block ) return false;
      std::stringstream ss_conv(buf_datc);
      ss_conv >> element->datc_;
      if( !ss_conv ) return false;
    }
    { // 9: Parse the data
      size_t len = element->dts_*element->datc_;
      element->data_c_ = new char[len];
      std::memset(element->data_c_, 0, len);
      ss_block.read(element->data_c_, len);
      if( !ss_block ) return false;
    }
    this->elements_[element->lbl_] = element;
  }
  ss_block.flush();
  return true;
}


//: Rietrieve the Unique Source System Name
const std::string&
vil_nitf_engrda
::get_source(void) const
{
  return this->resrc_;
}


//: Retrieve a data element given it's label
const vil_nitf_engrda_element_sptr
vil_nitf_engrda
::get(const std::string &label) const
{
  vil_nitf_engrda::const_iterator i;
  i = this->elements_.find(label);
  if( i == this->elements_.end() )
  {
    return NULL;
  }
  return i->second;
}


//: Retrieve an iterator to the start of all data elements
vil_nitf_engrda::const_iterator
vil_nitf_engrda
::begin(void) const
{
  return this->elements_.begin();
}


//: Retrieve an iterator to the end of all data elements
vil_nitf_engrda::const_iterator
vil_nitf_engrda
::end(void) const
{
  return this->elements_.end();
}

