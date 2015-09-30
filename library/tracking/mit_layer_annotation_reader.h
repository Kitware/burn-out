/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef mit_layer_annotation_reader_h_
#define mit_layer_annotation_reader_h_

#include <tracking/track_reader.h>

class TiXmlNode; 

struct occluder;


namespace vidtk
{
  class mit_layer_annotation_reader
    : public track_reader
  {
  private:
    void read_xml(TiXmlNode* pParent, 
      vcl_vector< vidtk::track_sptr >& trks,
      vcl_vector< occluder >* occluders);
    void read_occulder_xml( TiXmlNode* pParent, 
      vcl_vector<occluder>& occluders);
    vcl_string occluder_filename_;
    double frequency_of_frames;
    unsigned frame_indexing;

  public: 
    mit_layer_annotation_reader():
    track_reader()
    {
       frame_indexing = 0;
       frequency_of_frames = -1;
    }
    mit_layer_annotation_reader(const char* name):
    track_reader(name)
    {
       frame_indexing = 0;
       frequency_of_frames = -1;
    }

    void set_occluder_filename(const char* name)
    {
       occluder_filename_ = name;
    };
    void set_frame_indexing(int zero_or_one)
    {
       frame_indexing = zero_or_one;
    }
    void set_frequency_of_frames(double frq)
    {
      frequency_of_frames = frq;
    }

    virtual bool read(vcl_vector<track_sptr>&trks);
  };
}
#endif // mit_layer_annotation_reader_h_

