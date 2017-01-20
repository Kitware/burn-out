/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_writer_vsl_h_
#define track_writer_vsl_h_

#include <tracking_data/track.h>
#include <tracking_data/io/track_writer_interface.h>

#include <string>
#include <fstream>

class vsl_b_ofstream;

namespace vidtk
{

class track_writer_vsl: public track_writer_interface
{
public:
  track_writer_vsl();
  virtual ~track_writer_vsl();

  using track_writer_interface::write;

  virtual bool write( std::vector<vidtk::track_sptr> const& tracks,
                      std::vector<vidtk::image_object_sptr> const* ios,
                      timestamp const& ts );
  virtual bool open( std::string const& fname );
  virtual bool is_open() const;
  virtual void close();
  virtual bool is_good() const;
  virtual void set_options(track_writer_options const& /*options*/)
  {}

protected:
  virtual bool write( vidtk::track const& /*track*/ )
  {
    return false;
  }

  virtual bool write( image_object const& /*io*/, timestamp const& /*ts*/)
  {
    return false;
  }

  vsl_b_ofstream* bfstr_;
};

}; //namespace vidtk

#endif
