/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// Given a sequence of images and a "zoom specification", generate a
/// new sequence of images with the original image on the left,
/// highlighted by a red (RGB) or white (graysalce) box (default), and
/// that zoomed area blown up to match the original image side and
/// pasted in on the right.  So a 100x100 image with a zoom of
/// 15x15+50+50 will produce a sequence of 200x100 images, 100x100 on
/// the left and a zoom of the 15x15 on the right.
///
/// Note that geometry strings HxWcX+Y (note the 'c') will be CENTERED on x,y

#include <vcl_cstdlib.h>

#include <vcl_sstream.h>
#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>
#include <vul/vul_reg_exp.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/vil_fwd.h>
#include <vil/vil_crop.h>
#include <vil/vil_resample_bilin.h>
#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_opening.h>
#include <vil/algo/vil_binary_closing.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/vgl_box_2d.h>

#include <pipeline/sync_pipeline.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp_reader_process.h>

#include <video/generic_frame_process.h>
#include <video/warp_image.h>
#include <video/image_list_writer_process.h>

struct geom_spec
{
  unsigned h,w,x0,y0,x1,y1;  // height, width, (x,y) upper-left corner, (x,y) lower-right corner
  bool valid;  // did we get set correctly?
  void set( const vcl_string& s ) 
  {
    vul_reg_exp re( "([0-9]+).([0-9]+)(.)([0-9]+).([0-9]+)" );
    if (re.find( s )) 
    {
      vcl_istringstream iss( re.match(1)+" "+re.match(2)+" "+re.match(4)+" "+re.match(5) );
      valid = (iss >> h >> w >> x0 >> y0).good();
      if (valid) 
      {
        if ((re.match(3) == "c") || (re.match(3) == "C")) {
          x0 -= (w/2);
          y0 -= (h/2);
        }
        x1 = x0+w;
        y1 = y0+h;
      }

    } 
  }
  void set( unsigned ni, unsigned nj )
  {
    x0 = y0 = 0;
    x1 = ni-1;
    y1 = nj-1;
    w = ni;
    h = nj;
    valid = true;
  }
  geom_spec(): h(0), w(0), x0(0), y0(0), x1(0), y1(0), valid(false) {}
  geom_spec( const vcl_string& s ): h(0), w(0), x0(0), y0(0), x1(0), y1(0), valid(false) { set(s); }

};      

vcl_string g_command_line;

int main( int argc, char* argv[] )
{
  for( int i = 0; i < argc; ++i )
  {
    g_command_line += " \"";
    g_command_line += argv[i];
    g_command_line += "\"";
  }

  vul_arg<char const*> config_file( "-c", "Config file" );
  //vul_arg<bool> show_tracks( "--show-tracks", "Show the tracks overlaid on the source frame.", true );
  vul_arg<vcl_string> output_pipeline( "--output-pipeline", "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );
  vul_arg<vcl_string> zoom_crop( "--zoom", "WxH[+c]X+Y: Zoom area of image (before input_crop, if any)", "" );
  vul_arg<vcl_string> input_crop( "--input-crop", "(optional)  WxH[+c]X+Y: also crop input (left) image" , "" );
  vul_arg<bool> enable_pad_16( "--enable-pad-16", "Pad final image x,y to multiples of 16 for video", true);

  vul_arg_parse( argc, argv );


  // standard pipeline setup
    
  vidtk::sync_pipeline p;
  
  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::timestamp_reader_process<vxl_byte> timestamper( "timestamper" );
  p.add( &timestamper );
  p.connect( frame_src.image_port(), timestamper.set_source_image_port() );
  p.connect( frame_src.timestamp_port(), timestamper.set_source_timestamp_port() );

  vidtk::image_list_writer_process<vxl_byte> writer( "writer" );

  vidtk::config_block config = p.params();
  config.add_subblock( writer.params(), "writer" );

  if( config_file.set() )
  {
    config.parse( config_file() );
  }

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( output_config.set() )
  {
    vcl_ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      vcl_cerr << "Couldn't open " << output_config() << " for writing\n";
      return EXIT_FAILURE;
    }
    config.print( fout );
  }

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
  }


  if( ! p.set_params( config ) )
  {
    vcl_cerr << "Failed to set pipeline parameters\n";
    return EXIT_FAILURE;
  }
  if( ! writer.set_params( config.subblock( "writer" ) ) )
  {
    vcl_cerr << "Failed to set writer parameters\n";
    return EXIT_FAILURE;
  }


  if( ! p.initialize() )
  {
    vcl_cout << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  // verify we've got out required zoom and optional input crop specifications

  geom_spec input_spec, zoom_spec;
  if ( input_crop.set() )
  {
    input_spec.set( input_crop() );
    if ( ! input_spec.valid ) 
    {
      vcl_cerr << "Couldn't parse input crop '" << input_crop() << "'; exiting\n";
      return EXIT_FAILURE;
    }
  }

  if ( zoom_crop.set() ) 
  {
    zoom_spec.set( zoom_crop() );
    if ( ! zoom_spec.valid ) 
    {
      vcl_cerr << "Couldn't parse zoom crop '" << zoom_crop() << "'; exiting\n";
      return EXIT_FAILURE;
    }
  } else {
    vcl_cerr << "Must specify zoom crop with --zoom-crop option; exiting\n";
    return EXIT_FAILURE;
  }

  // Run the pipeline once, so we have the first image, etc.  We can
  // then figure out the appropriate size for the windows and so on.
  if( ! p.execute() )
  {
    vcl_cerr << "Initial step failed.\n";
    return EXIT_FAILURE;
  }

  // if the input spec wasn't set, set to the whole input image
  
  if ( ! input_spec.valid ) 
  {
    input_spec.set( timestamper.image().ni(), 
                    timestamper.image().nj() );
  }

  // adjust the coordinates of the zoom spec
  zoom_spec.x0 -= input_spec.x0;
  zoom_spec.x1 -= input_spec.x0;
  zoom_spec.y0 -= input_spec.y0;
  zoom_spec.y1 -= input_spec.y0;

  // for now, clamp so that the aspect ratio of the zoom doesn't flip portait/landscape 
  // from source

  double scaleW = input_spec.w * 1.0 / zoom_spec.w;
  double scaleH = input_spec.h * 1.0 / zoom_spec.h;
  double scale = vcl_min( scaleW, scaleH );

  unsigned zoom_ni = static_cast<unsigned>( zoom_spec.w * scale );
  unsigned zoom_nj = static_cast<unsigned>( zoom_spec.h * scale );

  unsigned out_ni = input_spec.w + zoom_ni;
  unsigned out_nj = input_spec.h;

  // typically, ensure image size is a multiple of 16 for video encoding
  if (enable_pad_16()) 
  {
    out_ni += (out_ni % 16);
    out_nj += (out_nj % 16);
  }
  vcl_cout << "Final image size: " << out_ni << "x" << out_nj << "\n";

  vil_image_view<vxl_byte> dest( out_ni,
                                 out_nj,
                                 1,
                                 timestamper.image().nplanes() );

  vil_image_view<vxl_byte> zoom_dest( zoom_ni,
                                      zoom_nj,
                                      1,
                                      timestamper.image().nplanes() );

  do 
  {
    // announce progress
    vcl_cout << "Done processing time = ";
    if( timestamper.timestamp().has_time() )
    {
      vcl_cout << vul_sprintf("%.3f",timestamper.timestamp().time_in_secs());
    }
    if( timestamper.timestamp().has_frame_number() )
    {
      vcl_cout << " (frame="<< timestamper.timestamp().frame_number() << ")";
    }
    vcl_cout << "\n";

    // crop the input image
    vil_image_view<vxl_byte> src_image = vil_crop( timestamper.image(), 
                                                   input_spec.x0, 
                                                   input_spec.w,
                                                   input_spec.y0,
                                                   input_spec.h );

    vil_copy_to_window( src_image, dest, 0, 0 );

    // draw the box

    for (unsigned pl=0; pl<timestamper.image().nplanes(); ++pl) 
      for (unsigned i=zoom_spec.x0; i<zoom_spec.x1; ++i) 
        for (unsigned j=zoom_spec.y0; j<zoom_spec.y1; ++j) 
        {
          int dist_from_v_edge = vcl_min<int>( vcl_abs<int>( i-zoom_spec.x0 ), vcl_abs<int>( i-zoom_spec.x1 ) );
          int dist_from_h_edge = vcl_min<int>( vcl_abs<int>( j-zoom_spec.y0 ), vcl_abs<int>( j-zoom_spec.y1 ) );
          if ((dist_from_v_edge < 3) || (dist_from_h_edge < 3 ))
          {
            dest( i, j, pl ) = (pl == 0) ? 255 : 0;
          }
        }

    // get our zoom
    vil_image_view<vxl_byte> zoom_image = vil_crop( src_image, 
                                                    zoom_spec.x0,
                                                    zoom_spec.w,
                                                    zoom_spec.y0,
                                                    zoom_spec.h );

    // scale up
    vil_resample_bilin( zoom_image, zoom_dest, zoom_ni, zoom_nj );

    // copy 
    vil_copy_to_window( zoom_dest, dest, input_spec.w-1, 0 );

    writer.set_image( dest );
    writer.set_timestamp( timestamper.timestamp() );
    writer.step();
  } while (p.execute() );

  vcl_cout << "Done\n";

}
