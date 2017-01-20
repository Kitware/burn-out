/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <descriptors/descriptor_filter.h>

#include <iostream>
#include <fstream>

namespace
{

void run_test()
{
  vidtk::descriptor_filter filter;
  vidtk::descriptor_filter_settings filter_settings;

  vidtk::descriptor_sptr desc1 = vidtk::raw_descriptor::create( "descriptor1" );
  vidtk::descriptor_sptr desc2 = vidtk::raw_descriptor::create( "descriptorA" );

  desc1->resize_features( 10, 0.0 );
  desc2->resize_features( 20, 0.0 );

  desc1->add_track_id( 1 );
  desc2->add_track_id( 1 );

  vidtk::raw_descriptor::vector_t desc_list;
  desc_list.push_back( desc1 );
  desc_list.push_back( desc2 );

  filter_settings.to_duplicate.push_back( "descriptor1" );
  filter_settings.duplicate_ids.push_back( "descriptor2" );

  filter_settings.to_duplicate.push_back( "descriptorA" );
  filter_settings.duplicate_ids.push_back( "descriptorB" );

  filter_settings.to_merge.push_back( "descriptor1" );
  filter_settings.to_merge.push_back( "descriptorA" );
  filter_settings.merge_weights.push_back( 0.5 );
  filter_settings.merge_weights.push_back( 0.5 );
  filter_settings.merged_id = "joint";

  filter_settings.to_remove.push_back( "descriptor1" );

  TEST( "Filter Configure", filter.configure( filter_settings ), true );

  filter.filter( desc_list );

  TEST( "Filtered Size", desc_list.size(), 4 );

  std::map< vidtk::descriptor_id_t, unsigned > output_map;

  for( unsigned i = 0; i < desc_list.size(); ++i )
  {
    std::cout << desc_list[i]->get_type() << std::endl;
    output_map[ desc_list[i]->get_type() ]++;
  }

  TEST( "Contains Merged Descriptor", output_map[ "joint" ], 1 );
  TEST( "Contains Removed Descriptor", output_map[ "descriptor1" ], 0 );
}

} // end anonymous namespace

int test_descriptor_filter( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_descriptor_filter" );

  run_test();

  return testlib_test_summary();
}
