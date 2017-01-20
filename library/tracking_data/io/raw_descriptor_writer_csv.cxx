/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor_writer_csv.h"

#include <vul/vul_sprintf.h>

#include <logger/logger.h>

namespace vidtk {
namespace ns_raw_descriptor_writer {

VIDTK_LOGGER("raw_descriptor_writer_csv");


// ----------------------------------------------------------------
raw_descriptor_writer_csv::
raw_descriptor_writer_csv()
{
}

raw_descriptor_writer_csv::
~raw_descriptor_writer_csv()
{
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer_csv::
write( descriptor_sptr_t desc )
{
  if ( ! is_good() )
  {
    LOG_ERROR( "Output stream not open or in error state" );
    return false;
  }

  // Write ID type
  output_stream() << desc->get_type() << ", ";

  // Write timecodes/framenumbers
  descriptor_history_t const& hist = desc->get_history();

  if ( hist.size() > 0 )
  {
    timestamp ts = hist[0].get_timestamp();
    timestamp ets =  hist[hist.size() - 1].get_timestamp();

    output_stream() << ts.frame_number() << ", ";
    output_stream() << ets.frame_number() << ", ";

    if ( ts.has_time() )
    {
      output_stream() << vul_sprintf( "%.6f", ts.time_in_secs() ) << ", ";
    }
    else
    {
      output_stream() << "-1, ";
    }

    if ( ets.has_time() )
    {
      output_stream() << vul_sprintf( "%.6f", ets.time_in_secs() ) << ", ";
    }
    else
    {
      output_stream() << "-1,";
    }
  }
  else
  {
    output_stream() << "-1, -1, -1, -1, ";
  }

  // Write reference track IDs
  std::vector< track::track_id_t > const& tracks = desc->get_track_ids();
  for ( unsigned j = 0; j < tracks.size(); ++j )
  {
    output_stream() << ( j == 0 ? "" : " " ) << tracks[j];
  }
  output_stream() << ", ";

  // Write actual data
  descriptor_data_t const& data = desc->get_features();
  for ( unsigned j = 0; j < data.size(); ++j )
  {
    output_stream() << data[j] << " ";
  }

  // Write endline
  output_stream() << std::endl;

  return true;
} // raw_descriptor_writer_csv::write


// ----------------------------------------------------------------
bool
raw_descriptor_writer_csv::
initialize()
{
  // Write out header
  output_stream() << "# 1:descriptor_id, 2:start_frame, 3:end_frame, "
                    << "4:start_timestamp, 5:end_timestamp, "
                    << "6:track_references, 7:descriptor_data_vector"
                    << std::endl;
  return true;
}


void
raw_descriptor_writer_csv::
finalize()
{
}


} // end namespace
} // end namespace vidtk
