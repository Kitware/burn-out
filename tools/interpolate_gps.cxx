/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//transform_track_location.cxx

#include <boost/lexical_cast.hpp>
#include <vcl_iostream.h>
#include <vul/vul_arg.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_iostream.h>
#include <tracking/track_reader_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/track.h>
#include <pipeline/sync_pipeline.h>

#include <tracking/interpolate_gps_track_state_process.h>

//using namespace vidtk;

double GLOBAL_X, GLOBAL_Y;

vidtk::track_sptr translate(const vidtk::track_sptr & track)
{
  vcl_vector< vidtk::track_state_sptr > hist = track->history();
  for(unsigned int i = 0; i < hist.size(); ++i)
  {
    hist[i]->loc_[0] += GLOBAL_X;
    hist[i]->loc_[1] += GLOBAL_Y;
  }
  return track;
}

int main(int argc, char** argv)
{
  vul_arg<vcl_string> input_tracks_arg("--input-tracks", "Path to the tracks to be ingested","");
  vul_arg<vcl_string> input_format_arg("--input-format", "Format of the input tracks","");
  vul_arg<vcl_string> output_tracks_arg("--output-tracks", "Path for the tracks to be outputed","");
  vul_arg<vcl_string> output_format_arg("--output-format", "Format of the output tracks","");

  vul_arg_parse( argc, argv );

  //create the pipeline
  vidtk::sync_pipeline p;

  //create processes
  vidtk::track_reader_process reader("reader");
  vidtk::track_writer_process writer("writer");
  vidtk::interpolate_gps_track_state_process transformer("transformer");


  //assemble pipeline
  p.add(&reader);
  p.add(&transformer);
  p.add(&writer);
  p.connect( reader.tracks_port(), transformer.set_input_tracks_port());
  p.connect( transformer.output_track_list_port(), writer.set_tracks_port() );

  //figure out output filename from given parameters
  vcl_string output_filename;
  if(output_tracks_arg.set())
  {
    output_filename = output_tracks_arg();
  }
  else if(output_format_arg.set())
  {
    output_filename = input_tracks_arg();
    output_filename.append(".");
    output_filename.append(output_format_arg());
  }
  else
  {
    vcl_cerr << "ERROR: Output format must be specified\n";
    return EXIT_FAILURE;
  }

  //check that input filename is set
  if(!input_tracks_arg.set())
  {
    vcl_cerr << "ERROR: Input tracks must be specified\n";
    return EXIT_FAILURE;
  }

  //set configure pipeline
  vidtk::config_block blk = p.params();
  if(input_format_arg() != "")
  {
          blk.set("reader:format", input_format_arg());
  }
  blk.set("reader:filename", input_tracks_arg());
  blk.set("writer:disabled","false");
  blk.set("writer:filename", output_filename );
  blk.set("writer:overwrite_existing","true");
  blk.set("writer:format", output_format_arg());
  reader.set_batch_mode( false );
  if( !p.set_params(blk) )
  {
    vcl_cerr << "Error setting pipeline parameters\n";
    return EXIT_FAILURE;
  }

  // Process the pipeline nodes
  if( !p.initialize() )
  {
    vcl_cerr << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  //DO IT( one frame at a time... )
  while( p.execute() )
  {}
  vcl_cout << "done." << vcl_endl;

  return EXIT_SUCCESS;
}

