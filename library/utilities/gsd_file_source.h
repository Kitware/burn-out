/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_GSD_FILE_SOURCE_H
#define INCL_GSD_FILE_SOURCE_H

#include <string>
#include <iostream>

#include <utilities/timestamp.h>

namespace vidtk
{

struct gsd_file_source_impl_t;

/**
 * This class allows the user to specify the GSD on a per-frame
 * basis.  The motivating use case is when the metadata is valid
 * but incorrect; the solution is to sketch out some plausible
 * GSD values and tell WCSP to use those values no matter what
 * the metadata says.
 *
 *
 * The format is a header line and any number of range-value lines.
 * Blank lines and '#' comment lines are allowed.
 * The header line is:
 *
 * gsd using [framenumber | ts_usecs | ts_secs ] default $default
 *
 * A range-value line is:
 *
 * $min_value $max_value $gsd_value
 *
 *
 * $min_value and $max_value are in the units specified on the header
 * line, and are inclusive.
 *
 *
 * Examples:
 *
 * --
 * gsd using framenumber default 0.5
 * --
 *
 * ... will return 0.5 as the GSD for all frame numbers.
 *
 * --
 * gsd using ts_secs default 0.5
 *
 * # shot #1 is really zoomed in
 * 35.5 65.2 0.1
 *
 * # shot #2 is very wide
 * 100 200  2.0
 * --
 *
 * ...returns a GSD of 0.1 for timestamps, in seconds, in the range
 * [35.5, 65.2] inclusive, 2.0 for timestamps in the range [100, 200]
 * inclusive, and 0.5 for all other values.
 *
 */
class gsd_file_source_t
/*
  \todo Arslan's code review comments to address:
  "This class should be updated to reflect the change from GSD to image width.
  If that's not desired, then the consumption of this file needs to be updated."
 */
{
public:

  gsd_file_source_t();
  virtual ~gsd_file_source_t();

  bool initialize( const std::string& fn );
  bool initialize( std::istream& is );

  bool is_valid() const;

  // returns a gsd based on the timestamp.  If the timestamp does not
  // contain the appropriate field, log a warning and return the default
  // value.  If the object is uninitialized, throw.
  double get_gsd( const vidtk::timestamp& ts ) const;

  static void log_format_as_info();

private:
  gsd_file_source_impl_t* p;

};

} // vidtk

#endif
