/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "utilities/oc_timestamp_hack.h"
#include "utilities/timestamp.h"
#include "utilities/log.h"

#include "vcl_iostream.h"
#include "vcl_fstream.h"
#include "vcl_sstream.h"
#include "vul/vul_sprintf.h"
#include "vul/vul_reg_exp.h"

namespace
{

bool get_linestream( vcl_ifstream& is, vcl_istringstream& ss ) {
  vcl_string tmp;
  vcl_getline( is, tmp );
  if (is.eof()) return false;
  ss.str( tmp );
  return true;
}

}

namespace vidtk
{

bool
oc_timestamp_hack
::load_file( const vcl_string& fn )
{
  vcl_ifstream is( fn.c_str() );
  if (!is) {
    log_error( vul_sprintf("OC-TS-HACK: Couldn't open timestamp log file '%s'\n", fn.c_str()); );
    return false;
  }

  unsigned line_number = 0;
  while (!is.eof()) {
    vcl_istringstream ss;
    if (get_linestream( is, ss )) {
      line_number++;
      vcl_string frame_file;
      double time_of_day;
      if (!(ss >> frame_file)) {
	log_warning( vul_sprintf("OC-TS-HACK: Couldn't parse frame_file line %d; skipping\n", line_number));
	continue;
      }

      if (!(ss >> time_of_day)) {
	log_warning( vul_sprintf("OC-TS-HACK: Couldn't parse time_of_day line %d; skipping\n", line_number));
	continue;
      }

      vul_reg_exp re( "downtown_([0-9]+)\\.jpg" );
      if (!re.find( frame_file )) {
	log_warning( vul_sprintf("OC-TS-HACK: Couldn't find frame in %s; skipping\n", frame_file.c_str()));
	continue;
      }

      ss.str( re.match(1) );
      unsigned frame;
      ss >> frame;

      if (this->frame_to_time_map_.find( frame ) != this->frame_to_time_map_.end()) {
	log_warning( vul_sprintf("OC-TS-HACK: Duplicate frame %d; skipping 2nd appearance", frame));
	continue;
      }

      this->frame_to_time_map_[ frame ] = time_of_day * 1e6;
    }
  }

  return true;
}

bool
oc_timestamp_hack
::get_time_of_day( unsigned int frame, double& time_of_day ) const
{
  vcl_map< unsigned, double >::const_iterator i = this->frame_to_time_map_.find( frame );
  if (i == this->frame_to_time_map_.end()) {
    return false;
  } else {
    time_of_day = i->second;
    return true;
  }
}

bool
oc_timestamp_hack
::set_timestamp( timestamp& ts ) const
{
  double t;
  if (this->get_time_of_day( ts.frame_number(), t )) {
    ts.set_time( t );
    return true;
  } else {
    return false;
  }
}

} //namespace vidtk

