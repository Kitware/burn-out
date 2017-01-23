/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_OC_TIMESTAMP_HACK_H
#define INCL_OC_TIMESTAMP_HACK_H

#include <string>
#include <map>

namespace vidtk
{

class timestamp;

// a quick-n-dirty hack to set the time-of-day fields in the timestamp structure
// given the frame number, based on the external log files.

class oc_timestamp_hack
{
public:
  oc_timestamp_hack() {}
  bool load_file( const std::string& new_log_file );

  bool get_time_of_day( unsigned int frame, double& time_of_day ) const;
  bool set_timestamp( timestamp& ts ) const;

protected:

  std::map< unsigned, double > frame_to_time_map_;

private:

  oc_timestamp_hack( const oc_timestamp_hack& ); // no cpctor
  oc_timestamp_hack& operator=( oc_timestamp_hack& ); // no op=
};

} // namespace

#endif
