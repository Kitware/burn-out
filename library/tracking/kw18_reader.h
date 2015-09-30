/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef kw_18_reader_h_
#define kw_18_reader_h_

#include <tracking/track_reader.h>

namespace vidtk
{
  class kw18_reader :
    public track_reader
  {
  private:
    bool read_pixel_data;
    vcl_string path_to_images_;
    vcl_vector< vcl_string > image_names_;
    vcl_istream* track_stream_;

    bool read( vcl_istream&, vcl_vector< track_sptr >& trks );

  public:
    kw18_reader() :
        track_reader(){
        read_pixel_data = false;
        path_to_images_ = "";
        track_stream_ = 0;
        }
    explicit kw18_reader(const char* name) :
        track_reader(name) {
        read_pixel_data = false;
        path_to_images_ = "";
        track_stream_ = 0;
        }
    explicit kw18_reader( vcl_istream& is ) :
      track_reader() {
      read_pixel_data = false;
      path_to_images_ = "";
      track_stream_ = &is;
    }
    //Allows for reading pixel data into tracks
    //The path must be a path to a directory containing
    //.png's, .jpg's, or .jpegs and the order of those
    //images must corespond to the frame number of the
    //tracks
    bool set_path_to_images(vcl_string& path);

    bool read(vcl_vector< track_sptr >& trks);
  };
}

#endif

