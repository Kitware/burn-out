/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_writer_mit_h_
#define track_writer_mit_h_

#include <tracking_data/track.h>
#include <tracking_data/io/track_writer_interface.h>

#include <string>
#include <fstream>

namespace vidtk
{

class track_writer_mit: public track_writer_interface
{
public:
track_writer_mit()
  : fstr_( NULL ),
    x_offset_( 0 ),
    y_offset_( 0 ),
    num_imgs_( 0 )
    {}

  track_writer_mit( std::string path_to_images,
                    int x_offset = 0, int y_offset = 0 );

  virtual ~track_writer_mit();

  using track_writer_interface::write;

  virtual bool write( vidtk::track const& track );
  virtual bool open( std::string const& fname );
  virtual bool is_open() const;
  virtual void close();
  virtual bool is_good() const;
  virtual void set_options(track_writer_options const& options);

protected:
  std::ofstream * fstr_;
  std::string path_to_images_;
  int x_offset_;
  int y_offset_;
  int num_imgs_;
};

}; //namespace vidtk

#endif
