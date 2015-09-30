/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef shape_file_writer_h_
#define shape_file_writer_h_

#include <tracking/track.h>

#include <vcl_string.h>

// This relies on shapelib (http://shapelib.maptools.org/)

namespace vidtk
{

class shape_file_writer
{
  public:
    shape_file_writer();
    ~shape_file_writer();
    bool write( vidtk::track_sptr track );
    bool write( vidtk::track const & track );
    void open( vcl_string & fname );
    void close();
    void set_shape_type( int shape_type );
    void set_is_utm(bool v);
    void set_utm_zone(vcl_string const & zone);
  protected:
    struct internal;
    internal * internal_;
    int shape_type_;
    bool is_open_;
    bool is_utm_;
    vcl_string zone_;
    unsigned int count_;
};

}; //namespace vidtk

vidtk::shape_file_writer & operator<<( vidtk::shape_file_writer & writer, vidtk::track_sptr );

#endif
