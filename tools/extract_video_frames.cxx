/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This program will extract a specified set of frames from a given
/// video file.
///
/// If the requested frames start in the middle of the video, or are
/// not contiguous, the program expects that the video is well-formed
/// enough to allow seeking.

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_opening.h>
#include <vil/algo/vil_binary_closing.h>

#include <pipeline/sync_pipeline.h>

#include <video/generic_frame_process.h>
#include <video/image_list_writer_process.h>


int
main( int argc, char** argv )
{
  vul_arg<vcl_string> video_file( "-i", "Video file" );
  vul_arg<vcl_vector<unsigned> > req_frames( "-f", "Requested frames" );
  vul_arg<vcl_vector<unsigned> > req_frame_range( "-fr", "Requested frame range" );
  vul_arg<vcl_string> output_pattern( "-o", "Output pattern (image_list_writer style)", "frame%2$05d.png" );
  vul_arg<bool> number_from_zero_flag( "-z", "Start output frame IDs at zero" );

  vul_arg_parse( argc, argv );

  // Here we use the pipeline purely to aggregate the configuration
  // and initialization into a single call.  We don't actually use the
  // pipeline to pass data around.

  vidtk::sync_pipeline p;

  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::image_list_writer_process<vxl_byte> writer( "writer" );
  p.add( &writer );

  vidtk::config_block config = p.params();

  config.set( "src:type", "vidl_ffmpeg" );
  config.set( "src:vidl_ffmpeg:filename", video_file() );
  config.set( "writer:pattern", output_pattern() );

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( ! p.set_params( config ) )
  {
    vcl_cerr << "Failed to set pipeline parameters\n";
    return EXIT_FAILURE;
  }


  if( ! p.initialize() )
  {
    vcl_cerr << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  // Step the source to get to frame 0.
  if( ! frame_src.step() )
  {
    vcl_cerr << "Failed to step to first frame\n";
    return EXIT_FAILURE;
  }

  vcl_vector<unsigned> frames;

  if (req_frames.set())
  {
    // Order the requested frames to make life easier on us.
    frames = req_frames();
    vcl_sort( frames.begin(), frames.end() );
  }
  else
  {
    if (!req_frame_range.set())
    {
      vcl_cerr << "No frames specified; please use either -f or -fr option\n";
      return EXIT_FAILURE;
    }
    vcl_vector<unsigned> fr = req_frame_range();
    if (fr.size() != 2)
    {
      vcl_cerr << "-fr option takes exactly two frame indices, e.g. -fr 6168 6268\n";
      return EXIT_FAILURE;
    }
    for (unsigned i=fr[0]; i <= fr[1]; i++)
    {
      frames.push_back( i );
    }
  }

  // Iterate over them, saving each frame as we go.
  typedef vcl_vector<unsigned>::const_iterator iter_type;
  for( iter_type it = frames.begin(); it != frames.end(); ++it )
  {
    if( ! frame_src.timestamp().has_frame_number() )
    {
      vcl_cerr << "No frame number in video!\n";
      return EXIT_FAILURE;
    }

    if( frame_src.timestamp().frame_number() == *it )
    {
      // nothing to do; we are already where we need to be.
    }
    else
    {
      // Seek only if we have to
      if( frame_src.timestamp().frame_number() + 1 != *it )
      {
        // we need to seek
        if( ! frame_src.seek( *it ) )
        {
          vcl_cerr << "Couldn't seek to frame " << *it << "\n";
          return EXIT_FAILURE;
        }
      }

      // step() is what actually refreshes the image, and is necessary
      // even after a seek.
      if( ! frame_src.step() )
      {
        vcl_cerr << "Couldn't step to the next frame\n";
        return EXIT_FAILURE;
      }
    }

    if (number_from_zero_flag()) {
      vidtk::timestamp ts;
      unsigned relative_frame_number = frame_src.timestamp().frame_number() - frames[0];
      ts.set_frame_number( relative_frame_number );
      writer.set_timestamp( ts );
    }
    else
    {

      writer.set_timestamp( frame_src.timestamp() );
    }

    writer.set_image( frame_src.image() );
    if( ! writer.step() )
    {
      vcl_cerr << "Couldn't write image for frame "
               << frame_src.timestamp().frame_number() << "\n";
      return EXIT_FAILURE;
    }

    vcl_cout << "Wrote frame "
             << frame_src.timestamp().frame_number() << "\n";
  }

  return EXIT_SUCCESS;
}
