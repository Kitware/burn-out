/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_writer_shape_h_
#define track_writer_shape_h_

#include <tracking_data/track.h>
#include <tracking_data/io/track_writer_interface.h>

#include <string>
#include <fstream>

class TiXmlElement;

namespace vidtk
{

class track_writer_shape: public track_writer_interface
{
public:
  track_writer_shape( bool apix_way = true, std::string dir = "", std::string pre = "track_" );
  virtual ~track_writer_shape();

  using track_writer_interface::write;

  virtual bool write( std::vector<vidtk::track_sptr> const& tracks );
  virtual bool open( std::string const& fname );
  virtual bool is_open() const; //make sures the directory exists
  virtual void close()
  {
    //does nothing
  }
  virtual bool is_good() const; //make sure the directory exists
  virtual void set_options(track_writer_options const& options);

protected:
  virtual bool write( vidtk::track const& /*track*/ )
  {
    return false;
  }

  virtual bool write( image_object const& /*io*/, timestamp const& /*ts*/)
  {
    return false;
  }

  std::string directory_;
  std::string prefix_;

  bool apix_way_;
};

}; //namespace vidtk

#endif
