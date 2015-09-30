/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef viper_reader_h_
#define viper_reader_h_

#include <tracking/track_reader.h>

class TiXmlNode;


namespace vidtk
{

//enum that indicates the parent attribute of the element if applicable
enum XML_STATE
{
  NONE,
  LOCATION,
  OCCLUSION,
  DESCRIPTOR
};

class viper_reader : public track_reader
{
protected:
  void init();

private: 
  vcl_vector<vcl_string> descriptor_object_names_;

  void read_xml( TiXmlNode* pParent, 
                 vcl_vector< vidtk::track_sptr >& trks, 
                 XML_STATE xml_state);

public: 
  viper_reader() 
  : track_reader()
  { 
    init(); 
  }

  viper_reader(const char* name) 
  : track_reader(name)
  {
    init(); 
  }

  virtual bool read(vcl_vector<track_sptr>& trks);

  vcl_vector<vcl_string> descriptor_object_names() const
  { 
    return descriptor_object_names_; 
  }
};

} 

#endif  //viper_reader_h_
