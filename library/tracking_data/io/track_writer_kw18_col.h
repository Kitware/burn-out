/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_writer_kw18_col_h_
#define track_writer_kw18_col_h_

#include <tracking_data/track.h>
#include <tracking_data/io/track_writer_interface.h>

#include <string>
#include <fstream>

namespace vidtk
{

class track_writer_kw18_col
  : public track_writer_interface
{
public:
  enum colfmt
  {
    KW20,
    COL
  };

  track_writer_kw18_col( colfmt in = KW20 );

  virtual ~track_writer_kw18_col();

  using track_writer_interface::write;

  virtual bool write( vidtk::track const& track );
  virtual bool write( image_object const& io, timestamp const& ts );
  virtual bool open( std::string const& fname );
  virtual bool is_open() const;
  virtual void close();
  virtual bool is_good() const;
  virtual void set_options( track_writer_options const& options );


protected:
  void write_io( image_object const& io );

  colfmt format_;
  std::ofstream* fstr_;
  bool suppress_header_;
  int x_offset_;
  int y_offset_;
  bool write_lat_lon_for_world_;
  bool write_utm_;
  double frequency_;
};


//
// Classes with simple CTORS needed for plugin loader
//

// ------------------------------------------------------------------
class track_writer_kw18
  : public track_writer_kw18_col
{
public:
  track_writer_kw18()
    : track_writer_kw18_col(KW20)
  { }

  virtual ~track_writer_kw18() { }
};


// ------------------------------------------------------------------
class track_writer_col
  : public track_writer_kw18_col
{
public:
  track_writer_col()
    : track_writer_kw18_col(COL)
  { }

  virtual ~track_writer_col() { }
};


}; //namespace vidtk

#endif
