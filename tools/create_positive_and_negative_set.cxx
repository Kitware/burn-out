/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <boost/lexical_cast.hpp>
#include <vcl_iostream.h>
#include <vul/vul_arg.h>
#include <vul/vul_awk.h>
#include <vul/vul_string.h>

#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_iostream.h>
#include <vcl_map.h>

#include <tracking/track_reader_process.h>
#include <tracking/track_writer_process.h>
#include <pipeline/sync_pipeline.h>

//using namespace vidtk;

#include <utilities/split_vector_process.h>
#include <utilities/split_vector_process.txx>

#include <descriptors/function_path_length_descriptor.h>
#include <descriptors/calculate_descriptors.h>

vcl_map< unsigned int, int > TRACK_ID_TO_LABEL;
double G_min_max_distance_from_endpoint;
double G_gsd;
double G_min_track_length;

int split_label(const vidtk::track_sptr &t) 
{
  vcl_map< unsigned int, int >::const_iterator i = TRACK_ID_TO_LABEL.find(t->id());
  vcl_cout << t->id() << " ";
  if(i != TRACK_ID_TO_LABEL.end())
  {
    {
      vidtk::function_distance_from_begin_and_end_descriptor
        ept_fun( t->first_state(), t->last_state(), t->history().size(), G_gsd );
      vidtk::function_min_max_mean_sd_of_vector_function fun(&ept_fun);
      vidtk::compute_descriptor( t->history(), fun );
      vidtk::function_calculate_descriptor & tf = fun;
      vnl_vector<double> d = tf.get_value();
      vcl_cout << " max_distance " << d[1] << " ";
      if(d[1] < G_min_max_distance_from_endpoint && i->second == 1 )
      {
        vcl_cout << "not enough travel\n";
        return 0;
      }
    }
    {
      vidtk::function_path_length_descriptor fun(G_gsd);
      vidtk::compute_descriptor( t->history(), fun );
      double track_length = fun.get_single_value();
      vcl_cout << " length " << track_length << " ";
      if(track_length < G_min_track_length && i->second == 1 )
      {
        vcl_cout << "too short\n";
        return 0;
      }
    }
    vcl_cout << i->second << "\n";
    return i->second;
  }
  vcl_cout << "default\n";
  return 0;
}

int main(int argc, char** argv)
{
  vul_arg<vcl_string> input_tracks_arg("--input-tracks", "Path to the tracks to be ingested","");
  vul_arg<vcl_string> input_format_arg("--input-format", "Format of the input tracks","");
  vul_arg<vcl_string> output_tracks_prefix_arg("--output-track-prefix", "Path for the tracks to be outputed","");
  vul_arg<vcl_string> output_format_arg("--output-format", "Format of the output tracks","");
  vul_arg<vcl_string> path_to_images_arg("--path-to-images", 
                                         "If outputing a mit file you need to specify where "
                                         "the images coresponding to this file are located","");
  vul_arg<vcl_string> example_set_arg( "--labeled-set", "The labeled set", "" );
  vul_arg<int> x_offset_arg("--x-offset", "Write files with an x offset",0);
  vul_arg<int> y_offset_arg("--y-offset", "Write files with a y offset",0);
  vul_arg<float> frequency_arg("--frequency", "Frames per second in original video",0);
  vul_arg<float> min_max_distance_from_endpoint("--mmdfe",
                                                "Minumum distance the track has to trave from"
                                                " the end point to be postive example (meters)", 0);
  vul_arg<float> gsd("--gsd", "Pixels per meter", 0.5);
  vul_arg<float> min_track_length("--mtl", "min track length (meters)",0);
  vul_arg_parse( argc, argv );

  G_min_max_distance_from_endpoint = min_max_distance_from_endpoint();
  G_gsd = gsd();
  G_min_track_length = min_track_length();

  if(example_set_arg.set())
  {
    vcl_ifstream probStream( example_set_arg().c_str() );
    if ( !probStream )
    {
      vcl_cerr << "Couldn't read probability file " << example_set_arg() << ";\n Exiting\n";
      return EXIT_FAILURE;
    }
    vul_awk awk2(probStream);
    for( ; awk2; ++awk2)
    {
      unsigned int id = static_cast<unsigned int>(vul_string_atoi(awk2[0]));
      //int classification = vul_string_atoi(awk2[1]);
      int l = vul_string_atoi(awk2[1]);
      TRACK_ID_TO_LABEL[id] = l;
      vcl_cout << id << " " << l << vcl_endl;
    }
  }
  else
  {
    vcl_cerr << "need a labeled file" << vcl_endl;
    return 0;
  }

  //create the pipeline
  vidtk::sync_pipeline p;

  //create processes
  vidtk::track_reader_process reader("reader");
  vidtk::split_vector_process<vidtk::track_sptr, split_label> set_spliter("set_spliter");
  vidtk::track_writer_process writer_pos("writer_positive");
  vidtk::track_writer_process writer_neg("writer_negative");

  //assemble pipeline
  p.add(&reader);
  p.add(&set_spliter);
  p.add(&writer_pos);
  p.add(&writer_neg);
  p.connect( reader.tracks_port(), set_spliter.set_input_port() );
  p.connect( set_spliter.negative_port(), writer_neg.set_tracks_port() );
  p.connect( set_spliter.positive_port(), writer_pos.set_tracks_port() );

  //figure out output filename from given parameters
  vcl_string output_filename_pos, output_filename_neg;
  if(output_tracks_prefix_arg.set())
  {
    output_filename_pos = output_tracks_prefix_arg() + "_pos.kw18";
    output_filename_neg = output_tracks_prefix_arg() + "_neg.kw18";
  }
  else if(output_format_arg.set())
  {
    output_filename_pos = input_tracks_arg();
    output_filename_pos.append("_pos.");
    output_filename_pos.append(output_format_arg());
    output_filename_neg = input_tracks_arg();
    output_filename_neg.append("_neg.");
    output_filename_neg.append(output_format_arg());
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
  if(frequency_arg.set())
  {
    blk.set("reader:frequency",frequency_arg());
    blk.set("writer_positive:frequency",frequency_arg());
    blk.set("writer_negative:frequency",frequency_arg());
  }
  if(input_format_arg() != "")
  {
    blk.set("reader:format", input_format_arg());
  }
  blk.set("reader:filename", input_tracks_arg());
  blk.set("writer_positive:disabled","false");
  blk.set("writer_negative:disabled","false");
  blk.set("writer_positive:filename", output_filename_pos);
  blk.set("writer_negative:filename", output_filename_neg);
  blk.set("writer_positive:overwrite_existing","true");
  blk.set("writer_negative:overwrite_existing","true");
  blk.set("writer_positive:format", output_format_arg());
  blk.set("writer_negative:format", output_format_arg());
  if(x_offset_arg.set())
  {
    blk.set("writer_positive:x_offset",x_offset_arg());
    blk.set("writer_negative:x_offset",x_offset_arg());
  }
  if(y_offset_arg.set())
  {
    blk.set("writer_positive:y_offset",y_offset_arg());
    blk.set("writer_negative:y_offset",y_offset_arg());
  }
  if(path_to_images_arg.set())
  {
    blk.set("writer_positive:path_to_images",path_to_images_arg());
    blk.set("writer_negative:path_to_images",path_to_images_arg());
  }
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
