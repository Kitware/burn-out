/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_writer_h_
#define vidtk_tot_writer_h_

#include <tracking_data/track.h>

#include <fstream>
#include <vector>
#include <string>

namespace vidtk
{

/// A class for writing target-object type probabilities to some file.
class tot_writer
{
public:

  tot_writer();
  ~tot_writer();

  /// Write the Target Object Type (TOT) probabilities associated with each track
  bool write( std::vector<vidtk::track_sptr> const& tracks );

  /// Write the Target Object Type (TOT) probabilities associated with a track
  bool write( vidtk::track const& track );

  bool set_format( std::string const& format );

  /// Open up a file for writing
  bool open( std::string const& fname );

  /// Is the file open for writing?
  bool is_open() const;

  /// Close a file for writing
  void close();

  /// Is the filestream open?
  bool is_good() const;

protected:

  std::ofstream fstr_;
};

}; //namespace vidtk

#endif
