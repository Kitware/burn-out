/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// Given a sequence of images/frames and a related set of
/// image-to-world homographies, this executable will generate a
/// "stabilized" set of images.

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>
#include <vul/vul_reg_exp.h> // crop string parsing

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_opening.h>
#include <vil/algo/vil_binary_closing.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/vgl_box_2d.h>

#ifndef NOGUI
#include <vgui/vgui.h>
#include <vgui/vgui_utils.h>
#include <vgui/vgui_image_tableau.h>
#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_viewer2D_tableau.h>
#include <vgui/vgui_text_tableau.h>
#include <vgui/vgui_composite_tableau.h>
#include <vgui/vgui_grid_tableau.h>
#include <vgui/vgui_shell_tableau.h>
#include <vgui/vgui_menu.h>
#include <vgui/vgui_utils.h>
#endif // NOGUI

#include <pipeline/sync_pipeline.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp_reader_process.h>

#include <video/generic_frame_process.h>
#include <video/warp_image.h>
#include <video/image_list_writer_process.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_inverse.h>

// Pixel type

#ifdef PIXELTYPE
typedef PIXELTYPE PixelType;
#else
typedef vxl_byte PixelType;
#endif


//---------------------------------------------------------------------------
//
// Global variables for gui and command line.
//
//---------------------------------------------------------------------------

bool g_pause = false;
bool g_step = false;

void step_once()
{
  g_step = true;
}

void pause_cb()
{
  g_pause = ! g_pause;
}


vcl_string g_command_line;

//---------------------------------------------------------------------------
//
// Forward declare some helper functions
//
//---------------------------------------------------------------------------
void
compute_output_size( unsigned& out_ni,
                     unsigned& out_nj,
                     int& i0, int& j0,
                     vcl_vector< vgl_h_matrix_2d<double> > const& homogs );

bool get_crop_region_from_string( vcl_string& crop_string,
                                  int& crop_i0,
                                  int& crop_j0,
                                  unsigned int& crop_ni,
                                  unsigned int& crop_nj );

//---------------------------------------------------------------------------
//
// Main
//
//---------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  for( int i = 0; i < argc; ++i )
  {
    g_command_line += " \"";
    g_command_line += argv[i];
    g_command_line += "\"";
  }

  vul_arg<char const*> config_file( "-c", "Config file" );
  vul_arg<vcl_string> output_pipeline( "--output-pipeline", "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );
  vul_arg<vcl_string> homog_file( "--homog-file", "The file containing the homographies.", "" );
  vul_arg<bool> auto_output_size( "--auto-output-size", "Automatically compute the output frame size (assuming frame size is constant).", false );
  vul_arg<vcl_string> out_homog_file( "--out-homog-file", "The homographies from the resulting frames to the source frames.", "" );
  vul_arg<bool> persist_gui_at_end( "--persist-gui-at-end", "Keep the GUI running after processing" );
  //double pixels_per_world_unit = 1;
  vul_arg<double> pixels_per_world_unit( "--pixels-per-world-unit", "Scale factor to convert from world coordinates to pixels", 1 );
  vul_arg<bool> build_mosaic( "--build-mosaic", "If set a mosaic of multiple images will be created.", false ); 
  vul_arg<unsigned> homog_file_ver( "--homog-file-version", 
    "1 for just homographies, 2 for frame_number, timestamp, and homographies", 1 );

  vul_arg_parse( argc, argv );

#ifndef NOGUI
  vgui::init( argc, argv );
#endif

  vidtk::sync_pipeline p;

  vidtk::generic_frame_process<PixelType> frame_src( "src" );
  p.add( &frame_src );

  vidtk::timestamp_reader_process<PixelType> timestamper( "timestamper" );
  p.add( &timestamper );
  p.connect( frame_src.image_port(), timestamper.set_source_image_port() );
  p.connect( frame_src.timestamp_port(), timestamper.set_source_timestamp_port() );

  vidtk::image_list_writer_process<PixelType> writer( "writer" );

  vidtk::config_block config = p.params();
  config.add_subblock( writer.params(), "writer" );
  config.add_optional( "crop_string", "Section of input frame used for homography computation (image 00 coordinates)" );

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


  vcl_vector< vgl_h_matrix_2d<double> > homogs;
  if( ! homog_file.set() )
  {
    vcl_cout << "Homography file not set" << vcl_endl;
    return EXIT_FAILURE;
  }

  vcl_ifstream homog_str( homog_file().c_str() );
  if( ! homog_str )
  {
    vcl_cout << "Couldn't open " << homog_file() << " for reading\n";
    return EXIT_FAILURE;
  }

  vcl_ofstream out_homog_str;
  if( out_homog_file.set() )
  {
    out_homog_str.open( out_homog_file().c_str() );
    if( ! out_homog_str )
    {
      vcl_cout << "Couldn't open " << out_homog_file() << " for writing\n";
      return EXIT_FAILURE;
    }
    out_homog_str.precision( 20 );
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
  if( ! writer.initialize() )
  {
    vcl_cout << "Failed to initialize writer\n";
    return EXIT_FAILURE;
  }

  vgl_h_matrix_2d<double> world_to_pixels;
  world_to_pixels.set_identity();
  if ( pixels_per_world_unit.set() )
  {
    world_to_pixels.set_scale( pixels_per_world_unit() );
  }
  vcl_cout << "WORLD TO PIXELS \n " << world_to_pixels << vcl_endl;

  while( homog_str )
  {
    vgl_h_matrix_2d<double> h;
    if( homog_file_ver() == 2 )
    {
      unsigned frame_number;
      double timestamp; 
      homog_str >> frame_number;
      homog_str >> timestamp;
    }
    if( homog_str >> h )
    {
      homogs.push_back( world_to_pixels * h );
    }
  }
  vcl_cout << "Read " << homogs.size() << " homographies\n";

  // Run the pipeline once, so we have the first image, etc.  We can
  // then figure out the appropriate size for the windows and so on.
  if( ! p.execute() )
  {
    vcl_cerr << "Initial step failed.\n";
    return EXIT_FAILURE;
  }

  unsigned out_ni = timestamper.image().ni();
  unsigned out_nj = timestamper.image().nj();
  //int out_i0 = 0;
  //int out_j0 = 0;

  //
  // Get the crop region or initialize to the entire image.
  //
  int crop_i0 = 0; // Initial positions can be negative, so this is an int.
  int crop_j0 = 0;
  unsigned int crop_numi = out_ni; // need to get default from input image.
  unsigned int crop_numj = out_nj; // need to get default from input image.

  if ( config.has( "crop_string" ) )
  {
    vcl_string crop_string_from_params;
    bool gotString = config.get( "crop_string", crop_string_from_params );
    if ( gotString ) 
    {
      bool gotCrop = get_crop_region_from_string( crop_string_from_params, crop_i0, crop_j0, crop_numi, crop_numj );
      if ( !gotCrop )
      {
        vcl_cerr << "Couldn't parse crop string : " << crop_string_from_params << vcl_endl;
        return EXIT_FAILURE;
      }
    }
  }

  vnl_double_3x3 translate_to_crop;
  translate_to_crop.set_identity();
  translate_to_crop( 0, 2 ) = crop_i0;
  translate_to_crop( 1, 2 ) = crop_j0;

  if( auto_output_size() )
  {
    compute_output_size( crop_numi, crop_numj, crop_i0, crop_j0, homogs );
  }

  // Now crop size has the transformed size.
  vil_image_view<PixelType> dest( crop_numi,
                                  crop_numj,
                                  1,
                                  timestamper.image().nplanes() );
  if( build_mosaic() )
  {
    dest.fill( 0 );
  }

  vnl_double_3x3 translate_to_world;
  translate_to_world.set_identity();
  translate_to_world( 0, 2 ) = crop_i0;
  translate_to_world( 1, 2 ) = crop_j0;


#ifndef NOGUI
  vgui_image_tableau_new img_tab;
  vgui_easy2D_tableau_new easy_tab( img_tab );
  vgui_text_tableau_new text_tab;
  vgui_composite_tableau_new comp_tab( easy_tab, text_tab );
  vgui_viewer2D_tableau_new viewer_tab( comp_tab );

  vgui_shell_tableau_new shell_tab( viewer_tab );

  vgui_menu main_menu;
  vgui_menu pause_menu;
  pause_menu.add( "Step once", &step_once, vgui_key(' ') );
  pause_menu.add( "Pause/unpause", &pause_cb, vgui_key('p') );
  main_menu.add( "Pause", pause_menu );

  vgui::adapt( shell_tab, timestamper.image().ni(), timestamper.image().nj(), main_menu );
  vgui::run_till_idle();
#endif // NOGUI

  unsigned homog_cnt = 0;
  do
  {
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

    if( homog_cnt >= homogs.size() )
    {
      vcl_cout << "Ran out of homographies!\n";
      return EXIT_FAILURE;
    }

    // Compose a few transforms to compute the transformation that will take
    // a pixel in the stabilized output frame into the crop region in the 
    // input image.
    vnl_double_3x3 m = translate_to_crop * vnl_inverse( homogs[homog_cnt].get_matrix() ) * translate_to_world;

    // Warp the image into the destination.
    vidtk::warp_image( timestamper.image(), dest, vgl_h_matrix_2d<double>(m),
                       vidtk::warp_image_parameters().set_fill_unmapped( !build_mosaic() ) );


    ++homog_cnt;

    if( out_homog_file.set() )
    {
      out_homog_str << m << "\n\n";
      out_homog_str.flush();
    }


    writer.set_image( dest );
    writer.set_timestamp( timestamper.timestamp() );
    writer.step();

#ifndef NOGUI
    img_tab->set_image_view( dest );
    img_tab->reread_image();
    img_tab->post_redraw();

    easy_tab->clear();
    text_tab->clear();

    vgui::run_till_idle();
    while( g_pause && ! g_step && ! vgui::quit_was_called() )
    {
      vgui::run_till_idle();
    }
    g_step = false;
#endif // NOGUI

  } while(
#ifndef NOGUI
    ! vgui::quit_was_called() &&
#endif
    p.execute() );



  vcl_cout << "Done" << vcl_endl;

#ifndef NOGUI

  if( vgui::quit_was_called() ||
      ! persist_gui_at_end() )
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return vgui::run();
  }

#else

  return EXIT_SUCCESS;

#endif
}



void
compute_output_size( unsigned& ni,
                     unsigned& nj,
                     int& i0,
                     int& j0,
                     vcl_vector< vgl_h_matrix_2d<double> > const& homogs )
{
  double minx = 0;
  double maxx = ni;
  double miny = 0;
  double maxy = nj;

  vcl_vector< vgl_homg_point_2d<double> > corners;
  corners.push_back( vgl_homg_point_2d<double>( minx, miny ) );
  corners.push_back( vgl_homg_point_2d<double>( maxx, miny ) );
  corners.push_back( vgl_homg_point_2d<double>( minx, maxy ) );
  corners.push_back( vgl_homg_point_2d<double>( maxx, maxy ) );

  vgl_box_2d<double> box;
  for( unsigned i = 0; i < homogs.size(); ++i )
  {
    for( unsigned j = 0; j < corners.size(); ++j )
    {
      vgl_point_2d<double> p = homogs[i] * corners[j];
      box.add( p );
    }
  }

  i0 = int( vcl_floor( box.min_x() ) );
  j0 = int( vcl_floor( box.min_y() ) );
  ni = unsigned( vcl_ceil( box.width() ) );
  nj = unsigned( vcl_ceil( box.height() ) );

  // Make ni and nj a multiple of 16, to make subsequent video computation easier
  ni = ((ni+15)/16)*16;
  nj = ((nj+15)/16)*16;

  vcl_cout << "Output image is " << ni << "x" << nj;
  if( i0 >= 0 ) vcl_cout << "+";
  vcl_cout << i0;
  if( j0 >= 0 ) vcl_cout << "+";
  vcl_cout << j0 << "\n";
}

bool get_crop_region_from_string( vcl_string& crop_string,
                                  int& crop_i0,
                                  int& crop_j0,
                                  unsigned int& crop_ni,
                                  unsigned int& crop_nj )
{
    // If the crop_string is set, initialize these. 
    crop_i0 = unsigned(-1);
    crop_j0 = 0;
    crop_ni = 0;
    crop_nj = 0;

    vul_reg_exp re( "([0-9]+)x([0-9]+)\\+([0-9]+)\\+([0-9]+)" );
    if( ! re.find( crop_string ) )
    {
      vcl_cerr << "Could not parse crop string " << crop_string << "\n";
      return false;
    }
    vcl_stringstream str;
    str << re.match(1) << " "
        << re.match(2) << " "
        << re.match(3) << " "
        << re.match(4);
    str >> crop_ni >> crop_nj >> crop_i0 >> crop_j0;
    vcl_cout << "crop section ("<<crop_i0<<","<<crop_j0<<") -> ("<<crop_i0+crop_ni<<","<<crop_j0+crop_nj<<")\n";

    return true;
}
