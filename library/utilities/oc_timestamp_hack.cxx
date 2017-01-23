/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "utilities/oc_timestamp_hack.h"
#include "utilities/timestamp.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include "vul/vul_sprintf.h"
#include "vul/vul_reg_exp.h"

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>
VIDTK_LOGGER("oc_timestamp_hack_cxx");

namespace vidtk
{

bool
oc_timestamp_hack
::load_file( const std::string& fn )
{
  std::ifstream is( fn.c_str() );
  if (!is)
  {
    LOG_ERROR("OC-TS-HACK: Couldn't open timestamp log file: " << fn);
    return false;
  }

  unsigned line_number = 0;
  std::string line;
  while (std::getline(is, line))
  {
    std::istringstream ss(line);
    line_number++;
    std::string frame_file;
    double time_of_day;
    if (!(ss >> frame_file))
    {
      LOG_WARN("OC-TS-HACK: Couldn't parse frame_file line " << line_number << ", skipping");
      continue;
    }

    if (!(ss >> time_of_day))
    {
      LOG_WARN("OC-TS-HACK: Couldn't parse time_of_day line " << line_number << ", skipping");
      continue;
    }

    vul_reg_exp re( "downtown_([0-9]+)\\.jpg" );
    if (!re.find( frame_file ))
    {
      LOG_WARN("OC-TS-HACK: Couldn't find frame in " << frame_file <<", skipping");
        continue;
    }

    unsigned int frame = boost::lexical_cast<unsigned int>( re.match(1) );

    if (this->frame_to_time_map_.find( frame ) != this->frame_to_time_map_.end())
    {
      LOG_WARN("OC-TS-HACK: Duplicate frame " << frame << ", skipping 2nd appearance");
      continue;
    }
    LOG_DEBUG(frame << " " << time_of_day * 1e6);
    this->frame_to_time_map_[ frame ] = time_of_day * 1e6;
  }

  return true;
}

bool
oc_timestamp_hack
::get_time_of_day( unsigned int frame, double& time_of_day ) const
{
  std::map< unsigned, double >::const_iterator i = this->frame_to_time_map_.find( frame );
  if (i == this->frame_to_time_map_.end())
  {
    return false;
  }
  else
  {
    time_of_day = i->second;
    return true;
  }
}

bool
oc_timestamp_hack
::set_timestamp( timestamp& ts ) const
{
  double t;
  if (this->get_time_of_day( ts.frame_number(), t ))
  {
    ts.set_time( t );
    return true;
  }
  else
  {
    return false;
  }
}

} //namespace vidtk
