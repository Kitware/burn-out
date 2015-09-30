#include <vcl_iostream.h>
#include <vul/vul_arg.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_iostream.h>
#include <tracking/track_reader_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/transform_tracks_process.h>
#include <pipeline/sync_pipeline.h>
#include <vxl_config.h>

using namespace vidtk;

int main(int argc, char** argv)
{
  vul_arg<char const*> config_file( "-c", "Config file (optional). Input arguments will over-ride the config file values." );
  vul_arg<vcl_string> input_tracks_arg("--input-tracks", "Path to the tracks to be ingested","");
  vul_arg<vcl_string> input_format_arg("--input-format", "Format of the input tracks","kw18");
  vul_arg<vcl_string> output_tracks_arg("--output-tracks", "Path for the tracks to be outputed","");
  vul_arg<vcl_string> output_format_arg("--output-format", "Format of the output tracks","kw18");
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );

  vul_arg_parse( argc, argv );

  sync_pipeline p;

  track_reader_process trp("track_reader");
  p.add( &trp );

  transform_tracks_process< vxl_byte > ttp( "transformer" );
  p.add( &ttp );
  p.connect( trp.tracks_port(), ttp.in_tracks_port() );

  track_writer_process twp("track_writer");
  p.add( &twp );
  p.connect( ttp.out_tracks_port(), twp.set_tracks_port() );

  config_block blk = p.params();
  // Parse the remaining arguments as additions to the config file
  blk.parse_arguments( argc, argv );

  if( output_config.set() )
  {
    vcl_ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      vcl_cerr << "Couldn't open " << output_config() << " for writing\n";
      return EXIT_FAILURE;
    }
    blk.print( fout );
    vcl_cout<< "Wrote config to " << output_config() << ". Exiting.\n";
    return EXIT_SUCCESS;
  }

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

  if(!input_tracks_arg.set())
  {
    vcl_cerr << "ERROR: Input tracks must be specified\n";
    return EXIT_FAILURE;
  }

  // If there is a config file specified as a parameter, then parse it.
  if( config_file.set() )
  {
    if ( ! blk.parse( config_file() ) )
    {
      vcl_cerr << "Could not parse the config file: " << config_file() << vcl_endl;
      return EXIT_FAILURE;
    }
  }

  blk.set("track_reader:filename", input_tracks_arg());
  if(input_format_arg() != "")
  {
    blk.set("track_reader:format", input_format_arg());
  }
  trp.set_batch_mode(true);

  blk.set("track_writer:filename", output_filename);
  blk.set("track_writer:overwrite_existing","true");
  blk.set("track_writer:format", output_format_arg());
  blk.set("track_writer:disabled","false");
  
  if(!p.set_params( blk ))
  {
    vcl_cerr << "pipeline::set_params() failed" << vcl_endl;
    return EXIT_FAILURE;
  }

  if(!p.initialize())
  {
    vcl_cerr << "pipeline failed to initialize" << vcl_endl;
    return EXIT_FAILURE;
  }

  if( !p.execute() )
  {
    vcl_cerr << "pipeline execution failed" << vcl_endl;
    return EXIT_FAILURE;
  }
  vcl_cout << "Wrote : " << output_filename << vcl_endl;

  return EXIT_SUCCESS;
}
