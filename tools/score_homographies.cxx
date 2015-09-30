/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline/sync_pipeline.h>

#include <utilities/homography_reader_process.h>
#include <utilities/homography_scoring_process.h>

#include <vul/vul_arg.h>

#include <vcl_iostream.h>

using namespace vidtk;

int main(int argc, char** argv)
{
  vul_arg<vcl_string> expected_homog_file( "--expected", "Path to the expected homography file", "");
  vul_arg<int> expected_version( "--expected-version", "Expected file version number", 2);
  vul_arg<vcl_string> generated_homog_file( "--generated", "Path to the generated homography file", "");
  vul_arg<int> generated_version( "--generated-version", "Generated file version number", 2);
  vul_arg<double> dist( "--dist", "Maximum distance between points after transform", 5);
  vul_arg<double> area( "--area", "Maximum difference between area after transform", .2);
  vul_arg<int> quadrant( "--quadrant", "Quadrant to place the rectangle in", 1);
  vul_arg<int> height( "--height", "Height of the original rectanlge", 480);
  vul_arg<int> width( "--width", "Width of the original rectanlge", 720);
  vul_arg_parse( argc, argv );

  if(!expected_homog_file.set() || !generated_homog_file.set())
  {
    vcl_cerr << "ERROR: Homography files not all specified\n";
    return EXIT_FAILURE;
  }

  config_block blk;

  homography_reader_process expected( "expected" );

  blk = expected.params();
  blk.set( "textfile", expected_homog_file() );
  blk.set( "version", expected_version() );
  expected.set_params( blk );
  expected.initialize();

  homography_reader_process generated( "generated" );

  blk = generated.params();
  blk.set( "textfile", generated_homog_file() );
  blk.set( "version", generated_version() );
  generated.set_params( blk );
  generated.initialize();

  homography_scoring_process scoring( "scoring" );

  blk = scoring.params();
  blk.set( "disabled", false );
  blk.set( "max_dist_offset", dist() );
  blk.set( "area_percent_factor", area() );
  blk.set( "quadrant", quadrant() );
  blk.set( "height", height() );
  blk.set( "width", width() );
  scoring.set_params( blk );
  scoring.initialize();

  sync_pipeline p;

  p.add( &expected );
  p.add( &generated );
  p.add( &scoring );

  p.connect( expected.image_to_world_homography_port(),
             scoring.set_good_homography_port() );
  p.connect( generated.image_to_world_homography_port(),
             scoring.set_test_homography_port() );

  size_t frame = 0;

  while( p.execute() )
  {
    vcl_cout << "Frame " << frame << ": "
             << ( scoring.is_good_homography() ? "PASS" : "FAIL" ) << vcl_endl;
    ++frame;
  }

  return EXIT_SUCCESS;
}
