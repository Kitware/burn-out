/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <descriptors/dpm_tot_generator.h>

#include <tracking_data/track.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <string>
#include <iostream>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void run_classification_test( const std::string& data_dir )
{
  const std::string image_file = data_dir + "/ocean_city.png";
  const std::string person_model_file = data_dir + "/test_dpm_model_person.xml";
  const std::string vehicle_model_file = data_dir + "/test_dpm_model_car.xml";

  dpm_tot_generator test_generator;
  dpm_tot_settings test_settings;

  test_settings.person_model_filename = person_model_file;
  test_settings.vehicle_model_filename = vehicle_model_file;
  test_settings.thread_count = 1;
  test_settings.person_classifier_threshold = -1.0;
  test_settings.vehicle_classifier_threshold = -1.0;

  TEST( "Configure (Single-Thread)", test_generator.configure( test_settings ), true );

  // Format input data (tracks and image)
  vil_image_view<vxl_byte> image1;
  image1 = vil_load( image_file.c_str() );
  vidtk::timestamp ts1( 1.3, 1 );

  vidtk::track_sptr trk1( new track() );
  vidtk::track_state_sptr state1( new track_state() );
  std::vector< vidtk::track_sptr > trk_vec;

  trk1->set_id( 1 );
  vgl_box_2d< unsigned > bbox( 353, 390, 200, 230 );
  image_object_sptr obj( new image_object() );
  obj->set_bbox( bbox );
  state1->set_image_object( obj );
  state1->time_ = ts1;
  trk1->add_state( state1 );
  trk_vec.push_back( trk1 );

  frame_data_sptr test_frame( new frame_data() );
  test_frame->set_image( image1 );
  test_frame->set_timestamp( ts1 );
  test_frame->set_active_tracks( trk_vec );

  // Step descriptor generator, look at output.
  test_generator.set_input_frame( test_frame );

  TEST( "Step (Single-Thread)", test_generator.step(), true );

  raw_descriptor::vector_t outputs = test_generator.get_descriptors();

  TEST( "Output Count (Single-Thread)", outputs.size(), 1 );

  descriptor_data_t desc_values = outputs[0]->get_features();

  TEST( "Output Descriptor Size (Single-Thread)", desc_values.size(), 3 );
  TEST( "Output Descriptor Value (Single-Thread)", desc_values[1] > 0.50, true );

  // Run same tests again, just with multiple threads
  test_settings.thread_count = 8;
  trk1->set_id( 2 );

  TEST( "Configure (Multi-Thread)", test_generator.configure( test_settings ), true );

  test_generator.set_input_frame( test_frame );

  TEST( "Step (Multi-Thread)", test_generator.step(), true );

  outputs = test_generator.get_descriptors();

  TEST( "Output Count (Multi-Thread)", outputs.size(), 1 );

  desc_values = outputs[0]->get_features();

  TEST( "Output Descriptor Size (Multi-Thread)", desc_values.size(), 3 );
  TEST( "Output Descriptor Value (Multi-Thread)", desc_values[1] > 0.50, true );
}

} // end anonymous namespace

int test_dpm_tot_generator( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument" << std::endl;
    return EXIT_FAILURE;
  }

  testlib_test_start( "dpm_tot_generator" );

  std::string data_dir( argv[1] );
  run_classification_test( data_dir );

  return testlib_test_summary();
}
