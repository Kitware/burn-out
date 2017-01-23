/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor_writer_json.h"

#include <vul/vul_sprintf.h>

#include <logger/logger.h>

namespace vidtk {
namespace ns_raw_descriptor_writer {

VIDTK_LOGGER("raw_descriptor_writer_json");


// ----------------------------------------------------------------
raw_descriptor_writer_json::
raw_descriptor_writer_json()
  : descriptor_counter(0)
{
}

raw_descriptor_writer_json::
~raw_descriptor_writer_json()
{
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer_json::
write( descriptor_sptr_t desc )
{
  if ( ! is_good() )
  {
    LOG_ERROR( "Output stream not open or in error state" );
    return false;
  }

  ++this->descriptor_counter;

  // Write descriptor header
  output_stream() << (this->descriptor_counter != 1 ? ", " : "  " )
                  << "{ \"raw_descriptor_" << this->descriptor_counter << "\" : {\n";

  // Write ID type
  output_stream() << "    \"type\" : \"" << desc->get_type() << "\",\n";

  // Write timecodes/framenumbers
  descriptor_history_t const& hist = desc->get_history();

  if ( hist.size() > 0 )
  {
    timestamp ts = hist[0].get_timestamp();
    timestamp ets =  hist[hist.size() - 1].get_timestamp();

    output_stream() << "    \"start_frame\" : " << ts.frame_number() << ",\n";
    output_stream() << "    \"end_frame\" : " << ets.frame_number() << ",\n";

    output_stream() << "    \"start_time\" : ";
    if ( ts.has_time() )
    {
      output_stream() << vul_sprintf( "%.6f", ts.time_in_secs() ) ;
    }
    else
    {
      output_stream() << "-1";
    }
    output_stream() << ",\n"
                    << "    \"end_time\" : ";


    if ( ets.has_time() )
    {
      output_stream() << vul_sprintf( "%.6f", ets.time_in_secs() );
    }
    else
    {
      output_stream() << "-1";
    }
    output_stream() << ",\n";

  }
  else
  {
    output_stream() << "    \"start_frame\" : -1,\n"
                    << "    \"end_frame\" : -1,\n"
                    << "    \"start_time\" : -1,\n"
                    << "    \"end_time\" : -1,\n";
  }

  // Write reference track IDs
  output_stream() << "    \"tracks\" : [ ";

  std::vector< track::track_id_t > const& tracks = desc->get_track_ids();
  for ( unsigned j = 0; j < tracks.size(); ++j )
  {
    output_stream() << ( j != 0 ? ", " : "" ) << tracks[j];
  }
  output_stream() << " ],\n";

  // Write actual dsescriptor data
  output_stream() << "    \"descriptor_data\" : [ ";
  descriptor_data_t const& data = desc->get_features();
  for ( unsigned j = 0; j < data.size(); ++j )
  {
    output_stream() << ( j != 0 ? ", " : "" ) << data[j];
  }
  output_stream() << " ]\n"
                  << "  }}\n";

  return true;
} // raw_descriptor_writer_json::write


// ----------------------------------------------------------------
bool
raw_descriptor_writer_json::
initialize()
{
  // Write out header
  output_stream() << "{ \"descriptors\" : [\n";

  return true;
}


void
raw_descriptor_writer_json::
finalize()
{
  output_stream() << "] }\n";
}


} // end namespace
} // end namespace vidtk
