/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//test_classification.cxx

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>

#include <vul/vul_arg.h>
#include <vul/vul_timer.h>
#include <vnl/vnl_random.h>
#include <vnl/vnl_math.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.hxx>

#include <utilities/log.h>

#include <pipeline/sync_pipeline.h>

#include <tracking/track_reader_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/tracking_keys.h>
#include <tracking/image_object.h>
#include <tracking/track_sptr.h>
#include <tracking/track_state.h>

#include <classifier/platt_score_adaboost_classifier_pipeline.h>
#include <classifier/track_to_vector_of_descriptors.h>

#include <utilities/apply_function_to_vector_process.h>
#include <utilities/apply_function_to_vector_process.txx>

double GLOBAL_gsd;

vidtk::track_sptr transform_location(const vidtk::track_sptr & p)
{
  vidtk::track_sptr r = p->clone();
  vcl_vector< vidtk::track_state_sptr > const& hist = r->history();
  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    vidtk::track_state_sptr at = hist[i];
    at->loc_[0] = at->loc_[0] * GLOBAL_gsd;
    at->loc_[1] = at->loc_[1] * GLOBAL_gsd;
    at->vel_[0] = at->vel_[0] * GLOBAL_gsd;
    at->vel_[1] = at->vel_[1] * GLOBAL_gsd;
  }
  return r;
}

vidtk::track_sptr transform_area(const vidtk::track_sptr & p)
{
  vidtk::track_sptr r = p->clone();
  vcl_vector< vidtk::track_state_sptr > const& hist = r->history();
  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    vcl_vector<vidtk::image_object_sptr> objs;
    if( hist[i]->data_.get( vidtk::tracking_keys::img_objs, objs ) )
    {
      for( unsigned x = 0; x < objs.size(); ++x )
      {
        if(objs[x]->area_ == -1)
        {
          continue;
        }
        objs[x]->area_ = objs[x]->area_ * GLOBAL_gsd * GLOBAL_gsd;
      }
    }
  }
  return r;
}

int no_track_transformation(vcl_string config_f, vcl_string output)
{
  vidtk::sync_pipeline p;

  vidtk::track_reader_process file_reader("track_reader");
  vidtk::platt_score_adaboost_classifier_pipeline ada("ada_classifier");
  vidtk::track_writer_process file_writer("result_writer");

  p.add( &file_reader );
  p.add( &ada );
  p.add( &file_writer );

  p.connect( file_reader.tracks_port(), ada.set_source_tracks_port() );
  p.connect( ada.get_classified_tracks_port(), file_writer.set_tracks_port() );

  vidtk::config_block config;
  config.add_subblock(p.params(), "adaboost_track_classifier");
  config.add_component_to_check_list("adaboost_track_classifier");

  config.parse( config_f );

  config.set("adaboost_track_classifier:ada_classifier:location_in_gsd", "false");
  config.set("adaboost_track_classifier:ada_classifier:area_in_gsd", "false");
  config.set("adaboost_track_classifier:result_writer:format", "vsl");
  config.set("adaboost_track_classifier:result_writer:filename", output);

  if( !p.set_params(config.subblock("adaboost_track_classifier")) )
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

  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( p.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }
  return 0;
}

int transform_location(vcl_string config_f, vcl_string output)
{
  vidtk::sync_pipeline p;

  vidtk::track_reader_process file_reader("track_reader");
  vidtk::apply_function_to_vector_process<vidtk::track_sptr, vidtk::track_sptr, transform_location> track_location_transformer("transform_location");
  vidtk::platt_score_adaboost_classifier_pipeline ada("ada_classifier");
  vidtk::track_writer_process file_writer("result_writer");

  p.add( &file_reader );
  p.add( &track_location_transformer );
  p.add( &ada );
  p.add( &file_writer );

  //p.connect( file_reader.tracks_port(), ada.set_source_tracks_port() );
  p.connect( file_reader.tracks_port(), track_location_transformer.set_input_port() );
  p.connect( track_location_transformer.get_output_port(),
             ada.set_source_tracks_port() );
  p.connect( ada.get_classified_tracks_port(), file_writer.set_tracks_port() );

  vidtk::config_block config;
  config.add_subblock(p.params(), "adaboost_track_classifier");
  config.add_component_to_check_list("adaboost_track_classifier");

  config.parse( config_f );

  GLOBAL_gsd = config.get<double>("adaboost_track_classifier:ada_classifier:gsd");
  config.set("adaboost_track_classifier:ada_classifier:location_in_gsd", "true");
  config.set("adaboost_track_classifier:ada_classifier:area_in_gsd", "false");
  config.set("adaboost_track_classifier:result_writer:format", "vsl");
  config.set("adaboost_track_classifier:result_writer:filename", output);

  if( !p.set_params(config.subblock("adaboost_track_classifier")) )
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

  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( p.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }
  return 0;
}

int transform_area(vcl_string config_f, vcl_string output)
{
  vidtk::sync_pipeline p;

  vidtk::track_reader_process file_reader("track_reader");
  vidtk::apply_function_to_vector_process<vidtk::track_sptr, vidtk::track_sptr, transform_area> track_area_transformer("transform_area");
  vidtk::platt_score_adaboost_classifier_pipeline ada("ada_classifier");
  vidtk::track_writer_process file_writer("result_writer");

  p.add( &file_reader );
  p.add( &ada );
  p.add( &track_area_transformer );
  p.add( &file_writer );

  //p.connect( file_reader.tracks_port(), ada.set_source_tracks_port() );
  p.connect( file_reader.tracks_port(), track_area_transformer.set_input_port() );
  p.connect( track_area_transformer.get_output_port(),
             ada.set_source_tracks_port() );
  p.connect( ada.get_classified_tracks_port(), file_writer.set_tracks_port() );

  vidtk::config_block config;
  config.add_subblock(p.params(), "adaboost_track_classifier");
  config.add_component_to_check_list("adaboost_track_classifier");

  {
    config.parse( config_f );
  }

  GLOBAL_gsd = config.get<double>("adaboost_track_classifier:ada_classifier:gsd");
  config.set("adaboost_track_classifier:ada_classifier:location_in_gsd", "false");
  config.set("adaboost_track_classifier:ada_classifier:area_in_gsd", "true");
  config.set("adaboost_track_classifier:result_writer:format", "vsl");
  config.set("adaboost_track_classifier:result_writer:filename", output);

  if( !p.set_params(config.subblock("adaboost_track_classifier")) )
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

  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( p.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }
  return 0;
}

int transform_location_area(vcl_string config_f, vcl_string output)
{
  vidtk::sync_pipeline p;

  vidtk::track_reader_process file_reader("track_reader");
  vidtk::apply_function_to_vector_process<vidtk::track_sptr, vidtk::track_sptr, transform_area> track_area_transformer("transform_area");
  vidtk::apply_function_to_vector_process<vidtk::track_sptr, vidtk::track_sptr, transform_location> track_location_transformer("transform_location");
  vidtk::platt_score_adaboost_classifier_pipeline ada("ada_classifier");
  vidtk::track_writer_process file_writer("result_writer");

  p.add( &file_reader );
  p.add( &ada );
  p.add( &track_area_transformer );
  p.add( &track_location_transformer );
  p.add( &file_writer );

  //p.connect( file_reader.tracks_port(), ada.set_source_tracks_port() );
  p.connect( file_reader.tracks_port(), track_area_transformer.set_input_port() );
  p.connect( track_area_transformer.get_output_port(),
             track_location_transformer.set_input_port() );
  p.connect( track_location_transformer.get_output_port(),
             ada.set_source_tracks_port() );
  p.connect( ada.get_classified_tracks_port(), file_writer.set_tracks_port() );

  vidtk::config_block config;
  config.add_subblock(p.params(), "adaboost_track_classifier");
  config.add_component_to_check_list("adaboost_track_classifier");

  {
    config.parse( config_f );
  }

  GLOBAL_gsd = config.get<double>("adaboost_track_classifier:ada_classifier:gsd");
  config.set("adaboost_track_classifier:ada_classifier:location_in_gsd", "true");
  config.set("adaboost_track_classifier:ada_classifier:area_in_gsd", "true");
  config.set("adaboost_track_classifier:result_writer:format", "vsl");
  config.set("adaboost_track_classifier:result_writer:filename", output);

  if( !p.set_params(config.subblock("adaboost_track_classifier")) )
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

  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( p.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }
  return 0;
}

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> config_file( "-c",
    "configuration file");

  vul_arg_parse( argc, argv );

  no_track_transformation( config_file(), "orignal.vsl" );
  transform_location( config_file(), "transform_loc.vsl" );
  transform_area( config_file(), "transform_area.vsl");
  transform_location_area( config_file(), "transform_area_and_area.vsl" );

  return EXIT_SUCCESS;
}

