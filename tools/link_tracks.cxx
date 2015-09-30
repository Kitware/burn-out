/*ckwg +5
* Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
* KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
* Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
*/

#include <vul/vul_arg.h>
#include <vul/vul_timer.h>
#include <tracking/track_reader_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/kinematic_track_linking_functor.h>
#include <tracking/track_linking.h>
#include <tracking/track_linking_process.h>
#include <pipeline/sync_pipeline.h>

// Global for command line processing
vcl_string g_command_line;

void usage() 
{
  vcl_cout << "link_tracks --computed-tracks input_file [--output-file output_file] [--computed-format kw18 | mit | viper ] " << vcl_endl;
}

int main( int argc, char* argv[])
{
  for( int i = 0; i < argc; ++i )
  {
    g_command_line += " \"";
    g_command_line += argv[i];
    g_command_line += "\"";
  }

  vul_arg<char const*> config_file( "-c", "Config file" );
  vul_arg<vcl_string> output_config( "--output-config",
                                     "Dump the configuration to this file", "" );
  //Command line args can be used in place of a config file or in addition. 
  //Command line args take precidence over config file
  vul_arg<vcl_string> computed_tracks_arg( "--computed-tracks", "Path to the computed track file", "");
  vul_arg<vcl_string> computed_track_format_arg("--computed-format", "Format computed track file is in", "");
  vul_arg<vcl_string> output_file( "--output-file", "Filename for output tracks. Will be column tracks format.");
  vul_arg<vcl_string> path_to_images_arg("--path-to-images", "Used for reading pixel data for visual linking.","");
  vul_arg<vcl_string> link_output_arg( "--link-output", "Filename for the information of which tracks were linked to which.","");

  vidtk::sync_pipeline p;
  vidtk::config_block config;

  //Create processes
  vidtk::track_reader_process trp("track_reader");
  p.add(&trp);
  vidtk::track_linking_process tlp( "track_linking");
  p.add(&tlp);
  vidtk::track_writer_process twp("track_writer");
  p.add(&twp);
  //Connect inputs and outputs
  p.connect(trp.tracks_port(),tlp.set_input_active_tracks_port());
  p.connect(trp.tracks_port(),tlp.set_input_terminated_tracks_port());
  p.connect(tlp.get_output_tracks_port(), twp.set_tracks_port());



  //Set up configuration structure
  config.add_subblock(p.params(), "link_tracks");
  config.add_parameter( "link_tracks:read_tracks_in_one_step", "true",
                        "  false: progressive mode, read tracks in steps.\n"
                        "  true : batch mode, read tracks in one step.\n");
  config.add_parameter( "link_tracks:link_output_filename", "", 
                        "  filename where the information about tracks that\n"
                        "  were linked will be dumped\n");

  vul_arg_parse( argc, argv );
  //Set up config default values
  vidtk::timestamp tlp_ts(0);
  tlp.set_timestamp(tlp_ts);
  config.set("link_tracks:track_writer:disabled","true");
  config.set("link_tracks:track_linking:time_window",0);
  config.set("link_tracks:track_linking:disabled","false");
  config.set("link_tracks:track_writer:overwrite_existing","true");
  //Read user set arguments from config file
  if( config_file.set() )
  {
    config.parse( config_file() );
  }
  //Read user set arguments from command line
  if(computed_tracks_arg.set())
  {
    config.set("link_tracks:track_reader:filename",computed_tracks_arg());
  }
  if(computed_track_format_arg.set())
  {
    config.set("link_tracks:track_reader:format", computed_track_format_arg());
  }
  if(path_to_images_arg.set())
  {
    //This is only needed if you are writing out in MIT format, or you want 
    //to use functors that make use of pixel information
    config.set("link_tracks:track_reader:path_to_images",path_to_images_arg());
    config.set("link_tracks:track_writer:path_to_images",path_to_images_arg());
  }
  if ( output_file.set() )
  {
    config.set("link_tracks:track_writer:filename",output_file());
    config.set("link_tracks:track_writer:disabled","false");
  }
  if ( link_output_arg.set() )
  {
    config.set("link_tracks:link_output_filename",link_output_arg());
  }

  //Write out config file if requested
  if( output_config.set() )
  {
    vcl_ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      vcl_cerr << "Couldn't open " << output_config() << " for writing\n";
      return false;
    }
    vcl_cerr << "output config" << vcl_endl;
    config.print( fout );
    return false;
  } 

  //Give configured options to the processes and pipeline
  if( !p.set_params(config.subblock("link_tracks")) )
  {
    vcl_cerr << "Error setting pipeline parameters\n";
    return false;
  }
  bool read_tracks_in_one_step;
  config.get("link_tracks:read_tracks_in_one_step", read_tracks_in_one_step);
  trp.set_batch_mode(read_tracks_in_one_step);

  // Process the pipeline nodes
  if(!config.has("link_tracks:track_reader:filename"))
  {
    //Make sure the computed tracks are set 
    vcl_cerr << "NEED TO SET link_tracks:track_reader:filename or run with --computed-tracks flag" << vcl_endl;
    return false;
  }
  
  if( !p.initialize() )
  {
    vcl_cerr << "Failed to initialize pipeline\n";
    return false;
  }

  //Execute the all the processes in the pipeline
  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( p.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }

  //If link output information is requested dump it to a file
  if(config.has("link_tracks:link_output_filename"))
  {
    vcl_string link_output_filename;
    config.get("link_tracks:link_output_filename",link_output_filename);
    if(link_output_filename != "")
    {
      vcl_ofstream fout( link_output_filename.c_str() );
      if( ! fout )
      {
        vcl_cerr << "Couldn't open " << link_output_filename << " for writing\n";
        return false;
      }
      vcl_map< unsigned, unsigned > linkPaths; //map that maps previds to currentids
      vcl_vector< unsigned > startLinks; //ids that start links

      //build a walkable map of which id's are linked to other ids
      vcl_vector< vidtk::track_linking_process::current_link > curr_links = tlp.get_output_link_info();
      vcl_vector< vidtk::track_linking_process::current_link >::iterator link_iter = curr_links.begin();
      for(; link_iter != curr_links.end(); link_iter++)
      {
        linkPaths[(*link_iter).self_->id()] = (*link_iter).next_id_;
        if(!link_iter->is_start())
        {
          linkPaths[(*link_iter).prev_id_] = (*link_iter).self_->id();
        }
        //only add to startLinks if something actually linked to this track
        else if(!(*link_iter).is_terminated())
        {
          startLinks.push_back((*link_iter).self_->id());
        }
      }

      //Loop over all ids that are the origin of a link and walk 
      //the map to get all the other tracks that are associated and 
      //output it to a file
      fout << "[\n";
      vcl_vector< unsigned >::const_iterator iter = startLinks.begin();
      while(iter != startLinks.end())
      {
        unsigned currId = (*iter);
        fout << "[ ";
        while (currId != -1)
        {
          fout << currId ;
          currId = linkPaths[currId];
          if (currId != -1)
          {
            fout << ", ";
          }
        }
        fout << " ]";
        ++iter;
        if (iter != startLinks.end())
        {
          fout << ",";
        }
        fout << vcl_endl;
      }
      fout << "]";
    }
  }
  return 0;
}


