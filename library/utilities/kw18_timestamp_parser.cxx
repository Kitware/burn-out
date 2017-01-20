/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "kw18_timestamp_parser.h"

#include <fstream>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <utilities/shell_comments_filter.h>
#include <utilities/blank_line_filter.h>

namespace vidtk
{

/// Given a kw18/20 track file, parse all of the timestamps in it
/// and put them into a standard set.
bool load_timestamps_from_kw18( const std::string& filename,
                                std::set< vidtk::timestamp >& output )
{
  output.clear();

  // Parse input kw18
  std::ifstream input_file( filename.c_str() );

  if( !input_file.is_open() )
  {
    return false;
  }

  boost::iostreams::filtering_istream input;
  input.push (vidtk::blank_line_filter());
  input.push (vidtk::shell_comments_filter());
  input.push (input_file);

  std::string line;

  double time;
  unsigned frame_num;

  while( std::getline( input, line ) )
  {

    std::vector< std::string > parsed_line;
    std::stringstream ss( line );

    while( ss )
    {
      parsed_line.push_back( "" );
      ss >> parsed_line.back();
    }

    if( parsed_line.size() < 18 )
    {
      continue;
    }

    frame_num = boost::lexical_cast<unsigned>( parsed_line[2] );
    time = boost::lexical_cast<double>( parsed_line[17] );

    output.insert( timestamp( time, frame_num ) );
  }

  return true;
}

/// Given a kw18/20 track file, parse all of the timestamps in it
/// and put them into a standard queue.
bool load_timestamps_from_kw18( const std::string& filename,
                                std::queue< timestamp >& output )
{
  std::set< timestamp > timestamp_set;

  if( !load_timestamps_from_kw18( filename, timestamp_set ) )
  {
    return false;
  }

  // Populate internal queue
  for( std::set< timestamp >::iterator p = timestamp_set.begin(); p != timestamp_set.end(); ++p )
  {
    output.push( *p );
  }
  return true;
}

/// Given a kw18/20 track file, parse all of the timestamps in it
/// and put them into a standard vector.
bool load_timestamps_from_kw18( const std::string& filename,
                                std::vector< vidtk::timestamp >& output )
{
  output.clear();

  std::set< timestamp > timestamp_set;

  if( !load_timestamps_from_kw18( filename, timestamp_set ) )
  {
    return false;
  }

  // Populate internal queue
  for( std::set< timestamp >::iterator p = timestamp_set.begin(); p != timestamp_set.end(); ++p )
  {
    output.push_back( *p );
  }
  return true;
}

} // end namespace vidtk
