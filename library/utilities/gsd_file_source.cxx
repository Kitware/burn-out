/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gsd_file_source.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <vul/vul_reg_exp.h>

#include <logger/logger.h>
VIDTK_LOGGER( "gsd_file_source_cxx" );

namespace // anon
{

//
// This holds the data from a range line, and implements
// the lookup routine, as well as an overlaps check for
// consistency verification
//
// start and end values are always integral.  Frames are frames,
// timestamps are timestamp_usecs, and if the client is
// using timestamp_secs, it's up to the client to convert
// before calling contains().

struct range_value_t
{
  unsigned long long start;
  unsigned long long end;
  double value;
  range_value_t( unsigned long long s, unsigned long long e, double v )
    : start(s), end(e), value(v) {}

  bool overlaps( const range_value_t& other ) const;
  bool contains( unsigned long long probe ) const;
};

bool
range_value_t
::overlaps( const range_value_t& other ) const
{
  if ( this->end < other.start ) return false;
  if ( this->start > other.end ) return false;
  return true;
}

bool
range_value_t
::contains( unsigned long long probe ) const
{
  return (this->start <= probe ) && ( probe <= this->end );
}


} // anon

namespace vidtk
{

struct gsd_file_source_impl_t
{
  gsd_file_source_impl_t(): valid(false) {}
  bool initialize( const std::string& fn );
  bool initialize( std::istream& is );

  double get_gsd( const timestamp& ts ) const;

  enum key_t { KEY_NONE, KEY_FRAME, KEY_TS_USECS, KEY_TS_SECS };

  bool valid;
  key_t key_type;
  double default_gsd;
  std::vector< range_value_t > range_lines;
};

bool
gsd_file_source_impl_t
::initialize( const std::string& fn )
{
  std::ifstream is( fn.c_str() );
  if ( ! is )
  {
    LOG_ERROR( "Couldn't open GSD file source '" << fn << "'" );
    return false;
  }
  return this->initialize( is );
}

bool
gsd_file_source_impl_t
::initialize( std::istream& is )
{
  this->range_lines.clear();

  vul_reg_exp comment_re( "^ *#" ), blank_re( "^ *$" );
  std::string line;
  size_t content_line_count = 0;

  while ( getline( is, line ))
  {
    if ( blank_re.find( line ))
    {
      continue;
    }

    if ( comment_re.find( line ))
    {
      continue;
    }

    ++content_line_count;

    if ( content_line_count == 1 )
    {
      //
      // parse the header
      //

      std::string gsd_s, using_s, units_s, default_s, default_value_s;
      std::istringstream liness(line);
      if ( ! ( liness >> gsd_s >> using_s >> units_s >> default_s >> default_value_s ))
      {
        LOG_ERROR( "GSD file source: couldn't parse header '" << line << "'" );
        return false;
      }

      // small sanity check
      if ( gsd_s != "gsd" )
      {
        LOG_ERROR( "GSD file source: couldn't find 'gsd' keyword in header '" << line << "'" );
        return false;
      }

      // get the units
      if ( units_s == "framenumber" )
      {
        this->key_type = gsd_file_source_impl_t::KEY_FRAME;
      }
      else if ( units_s == "ts_usecs" )
      {
        this->key_type = gsd_file_source_impl_t::KEY_TS_USECS;
      }
      else if ( units_s == "ts_secs" )
      {
        this->key_type = gsd_file_source_impl_t::KEY_TS_SECS;
      }
      else
      {
        LOG_ERROR( "GSD file source: couldn't parse units '" << units_s << "; valid options are "
                   << " framenumber, ts_usecs, or ts_secs" );
        return false;
      }

      // get the default value
      {
        std::istringstream iss( default_value_s );
        if ( ! ( iss >> this->default_gsd ))
        {
          LOG_ERROR( "GSD file source: couldn't parse default value '" << default_value_s << "'" );
          return false;
        }
      }
    }
    else
    {
      // parse range lines
      std::string start_s, end_s, gsd_s;
      unsigned long long start_v, end_v;
      double gsd_v;

      {
        std::istringstream iss( line );
        if ( ! ( iss >> start_s >> end_s >> gsd_s ))
        {
          LOG_ERROR( "Couldn't parse range line '" << line << "'" );
          return false;
        }
      }
      std::istringstream iss_se( start_s+ " "+end_s ), iss_g( gsd_s );

      // parse the start / end values (need to convert if seconds)
      switch (this->key_type)
      {

      case gsd_file_source_impl_t::KEY_FRAME:
      {
        if ( ! ( iss_se >> start_v >> end_v ))
        {
          LOG_ERROR( "Couldn't parse starting or ending frame from '" << line << "'" );
          return false;
        }
      }
      break;

      case gsd_file_source_impl_t::KEY_TS_USECS:
      {
        if ( ! ( iss_se >> start_v >> end_v ))
        {
          LOG_ERROR( "Couldn't parse starting or ending timestamp(usecs) from '" << line << "'; "
                     << "(only integral values allowed) ");
          return false;
        }
      }
      break;

      case gsd_file_source_impl_t::KEY_TS_SECS:
      {
        double tmp_s, tmp_e;
        if ( ! ( iss_se >> tmp_s >> tmp_e ))
        {
          LOG_ERROR( "Couldn't parse starting or ending timestamp(secs) from '" << line << "'" );
          return false;
        }
        start_v = static_cast<unsigned long long>( tmp_s * 1.0e6 );
        end_v = static_cast<unsigned long long>( tmp_e * 1.0e6 );
      }
      break;

      default:
      {
        LOG_ERROR( "Logic error: unknown key value " << this->key_type );
        return false;
      }

      } // ...case

      // parse the default value
      if ( ! ( iss_g >> gsd_v ))
      {
        LOG_ERROR( "Couldn't parse GSD value from '" << line << "'" );
        return false;
      }

      if ( end_v < start_v )
      {
        LOG_ERROR( "GSD file line '" << line << "' has end time before start time; bailing" );
        return false;
      }

      // create the record and add it
      this->range_lines.push_back( range_value_t( start_v, end_v, gsd_v ));

    } // ...for all non-header lines

  } // ...for all lines

  //
  // Some more sanity checking...
  //
  if ( this->range_lines.size() > 1 )
  {
    for (size_t i=0; i<this->range_lines.size(); ++i)
    {
      const range_value_t& r_this = this->range_lines[i];
      for (size_t j=i+1; j<this->range_lines.size(); ++j)
      {
        const range_value_t& r_that = this->range_lines[j];
        if (r_this.overlaps( r_that ))
        {
          LOG_WARN( "GSD range line " << i << " overlaps range line " << j << "; continuing anyway" );
        }
      }
    }
  }

  // we are now valid if we've seen at least one non-blank line
  this->valid = (content_line_count > 0);
  if ( ! this->valid )
  {
    LOG_ERROR( "Couldn't initialize a gsd file source." );
    gsd_file_source_t::log_format_as_info();
  }
  return this->valid;
}

double
gsd_file_source_impl_t
::get_gsd( const timestamp& ts ) const
{
  if ( ! this->valid )
  {
    throw std::runtime_error( "Attempted to get GSD from an uninitialized GSD file source" );
  }

  unsigned long long probe;
  switch (this->key_type)
  {

  case gsd_file_source_impl_t::KEY_FRAME:
  {
    if ( ! ts.has_frame_number() )
    {
      LOG_WARN( "GSD timestamp probe " << ts  << " has no frame number; using default GSD of " << this->default_gsd );
      return this->default_gsd;
    }
    probe = ts.frame_number();
  }
  break;

  case gsd_file_source_impl_t::KEY_TS_USECS:
  case gsd_file_source_impl_t::KEY_TS_SECS:
  {
    if ( ! ts.has_time() )
    {
      LOG_WARN( "GSD timestamp probe " << ts << " has no timestamp; using default GSD of " << this->default_gsd );
      return this->default_gsd;
    }
    probe = static_cast<unsigned long long>( ts.time() );
  }
  break;

  default:
  {
    std::ostringstream oss;
    oss << "Logic error in get_gsd: unknown gsd source file key type " << this->key_type;
    throw std::runtime_error( oss.str().c_str() );
  }
  } // switch

  for (size_t i=0; i<this->range_lines.size(); ++i)
  {
    if ( this->range_lines[i].contains( probe ))
    {
      return this->range_lines[i].value;
    }
  }
  return this->default_gsd;
}

gsd_file_source_t
::gsd_file_source_t()
  : p( new gsd_file_source_impl_t() )
{}

gsd_file_source_t
::~gsd_file_source_t()
{
  delete p;
}

void
gsd_file_source_t
::log_format_as_info()
{
  LOG_INFO( "The format for a gsd file source is:" );
  LOG_INFO( "--" );
  LOG_INFO( "gsd using $UNITS default $DEFAULT_GSD" );
  LOG_INFO( "$min_value $max_value  $gsd_value" );
  LOG_INFO( "--" );
  LOG_INFO( "The first line is mandatory; the second line is optional" );
  LOG_INFO( "and may be repeated as often as desired." );
  LOG_INFO( "$UNITS may be 'framenumber', 'ts_usecs', or 'ts_secs'." );
  LOG_INFO( "$min_value and $max_value are in units of $UNITS." );
  LOG_INFO( "Blank lines and '#' comment lines are allowed." );
}

bool
gsd_file_source_t
::is_valid() const
{
  return p->valid;
}

bool
gsd_file_source_t
::initialize( const std::string& fn )
{
  return p->initialize(fn);
}

bool
gsd_file_source_t
::initialize( std::istream& is )
{
  return p->initialize( is );
}

double
gsd_file_source_t
::get_gsd( const timestamp& ts ) const
{
  return p->get_gsd( ts );
}

} // vidtk
