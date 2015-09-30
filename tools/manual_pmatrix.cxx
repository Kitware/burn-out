/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This application can be used to quickly derive manually-specified
/// ground plane homographies and projective matrices.
///
/// It allows you to specify a series of image to world
/// correspondences, and then can use them to estimate various
/// transformations.  It can also project world models (e.g. of
/// football fields) to serve as a sanity check of the estimated
/// transformation.

#include <vcl_cstdlib.h>
#include <vcl_fstream.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <vul/vul_arg.h>
#include <vul/vul_sprintf.h>

#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_double_3x4.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_inverse.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_4.h>

#include <vgl/vgl_homg_point_2d.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/algo/vgl_h_matrix_2d_compute_linear.h>
#include <vgl/algo/vgl_h_matrix_2d_optimize_lmq.h>

#include <vgui/vgui.h>
#include <vgui/vgui_soview2D.h>
#include <vgui/vgui_dialog.h>
#include <vgui/vgui_deck_tableau.h>
#include <vgui/vgui_image_tableau.h>
#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_viewer2D_tableau.h>
#include <vgui/vgui_composite_tableau.h>
#include <vgui/vgui_shell_tableau.h>
#include <vgui/vgui_rubberband_tableau.h>
#include <vgui/vgui_menu.h>
#include <vgui/vgui_active_tableau.h>

#include <video/warp_image.h>

// An image<->world point correspondence
struct point_corresp
{
  vnl_vector_fixed<double,2> img_loc_;
  vnl_vector_fixed<double,3> world_loc_;

  // Id of visible soview, if not -1.
  unsigned soview_id_;

  point_corresp()
    : soview_id_( -1 )
  {
  }
};

// A 3x4 pmatrix projector

class pmatrix_projector
{
public:  
  explicit pmatrix_projector( const vcl_string& filename );
  vnl_vector_fixed<double, 2> world2img( const vnl_vector_fixed<double, 3>& world_loc ) const;
  vnl_vector_fixed<double, 2> img2worldZ0( double ix, double iy ) const;
private:
  vnl_double_3x4 pm;
  vnl_double_3x3 m, back_m;
};

pmatrix_projector
::pmatrix_projector( const vcl_string& filename )
{
  this->m.set_identity();
  vcl_ifstream is( filename.c_str() );
  if (!is) {
    vcl_cerr << "Couldn't open '" << filename << "'\n";
    return;
  }
  vcl_string line;
  vcl_getline( is, line );
  vcl_istringstream iss( line );
  if ( ! (iss >> this->pm )) {
    vcl_cerr << "Couldn't parse pmatrix from '" << line << "'\n";
    return;
  }
  this->m.set_column( 0, this->pm.get_column(0) );
  this->m.set_column( 1, this->pm.get_column(1) );
  this->m.set_column( 2, this->pm.get_column(3) );
  this->back_m = vnl_inverse( this->m );
}

vnl_vector_fixed<double, 2> 
pmatrix_projector
::world2img( const vnl_vector_fixed<double, 3>& world_loc) const
{
  vnl_double_4 ip( world_loc[0], world_loc[1], world_loc[2], 1.0 ); 
  vnl_double_3 op = this->pm * ip;
  return vnl_vector_fixed<double, 2>( op[0]/op[2], op[1]/op[2] );
}

vnl_vector_fixed<double, 2> 
pmatrix_projector
::img2worldZ0( double ix, double iy ) const
{
  vnl_double_3 ip( ix, iy, 1.0 ); 
  vnl_double_3 op = this->back_m * ip;
  return vnl_vector_fixed<double, 2>( op[0]/op[2], op[1]/op[2] );
}

    

// Collection of all current correspondences
vcl_vector< point_corresp > g_point_corresp;
bool g_homography_valid = false;
vgl_h_matrix_2d<double> g_homography;

bool g_draw_college_field = false;
bool g_draw_pro_field = false;

vcl_string g_save_prefix = "";

vil_image_view<vxl_byte> g_src_img;
vgui_rubberband_tableau_sptr g_rubber_tab;
vgui_easy2D_tableau_sptr g_easy_tab;
vgui_easy2D_tableau_sptr g_overlay_tab;
vgui_image_tableau_sptr g_img_tab;
vgui_image_tableau_sptr g_img2_tab;
pmatrix_projector* g_pmatrix_projector;

void
add_to_display( point_corresp& pc )
{
  g_easy_tab->set_point_radius( 5 );
  g_easy_tab->set_foreground( 1, 0, 1 );
  vgui_soview* so = g_easy_tab->add_point( float(pc.img_loc_[0]), float(pc.img_loc_[1]) );
  pc.soview_id_ = so->get_id();
}


struct feature_adder
  : public vgui_rubberband_client
{
  void set_activating_char( unsigned char c ) 
  { 
    this->ac = c;
  }
  void add_point( float ix, float iy )
  {
    switch (this->ac) {
    case 'a': add_corresp( ix, iy ); break;
    case 't': test_pmatrix( ix, iy ); break;
    default : vcl_cerr << "Whoa!  unknown activating char '" << this->ac << "'?\n"; break;
    }
  }

  void add_corresp( float ix, float iy )
  {
    static double wx, wy, wz = 0.0;

    // clear status line
    vgui::out << " \n";

    vgui_dialog dlg( "Point correspondence" );
    dlg.field( "World x", wx );
    dlg.field( "World y", wy );
    dlg.field( "World z", wz );
    dlg.field( "Image x", ix );
    dlg.field( "Image y", iy );
    if( dlg.ask() )
    {
      point_corresp pc;
      pc.img_loc_ = vnl_double_2( ix, iy );
      pc.world_loc_ = vnl_double_3( wx, wy, wz );
      add_to_display( pc );

      g_point_corresp.push_back( pc );

      g_easy_tab->post_redraw();
    }
  }

  void test_pmatrix( float ix, float iy )
  {
    vgui::out << " \n";
    if ( ! g_pmatrix_projector ) return;

    // convert click point to world-(x,y,0)
    vnl_vector_fixed<double, 2> wxy = g_pmatrix_projector->img2worldZ0( ix, iy );
    // crosshair w(0,0,0) - w(0, 5,0)
    vnl_vector_fixed<double, 3> wp1( wxy[0], wxy[1]+5, 0 );
    vnl_vector_fixed<double, 2> p1 = g_pmatrix_projector->world2img( wp1 );
    // crosshair w(0,0,0) - w(5, 0,0)
    vnl_vector_fixed<double, 3> wp2( wxy[0]+5, wxy[1], 0 );
    vnl_vector_fixed<double, 2> p2 = g_pmatrix_projector->world2img( wp2 );
    // crosshair w(0,0,0) - w(5, 0,0)
    vnl_vector_fixed<double, 3> wp3( wxy[0], wxy[1], 5 );
    vnl_vector_fixed<double, 2> p3 = g_pmatrix_projector->world2img( wp3 );

    g_easy_tab->set_foreground( 0, 1, 0 );
    g_easy_tab->add_line( ix, iy, p1[0], p1[1] );
    g_easy_tab->add_line( ix, iy, p2[0], p2[1] );
    g_easy_tab->set_foreground( 1, 1, 0 );
    g_easy_tab->add_line( ix, iy, p3[0], p3[1] );
  }

private:
  unsigned char ac;

};




void
error_box( vcl_string const& msg, vcl_string const& title = "Error" )
{
  // There is something wrong with the vgui-MFC dialog implementation
  // that causes the following to abort after the dialog is dismissed

  // vgui_dialog dlg( title.c_str() );
  // dlg.message( msg.c_str() );
  // dlg.set_cancel_button( 0 );
  // dlg.ask();

  vgui::out << "Error: " << msg << "\n";
  vcl_cerr << "Error: " << msg << "\n";
}


void
add_image_world_corresp()
{
  vgui::out << "Click on image point\n";
  feature_adder* f = dynamic_cast< feature_adder *>( g_rubber_tab->get_client() );
  f->set_activating_char('a');
  g_rubber_tab->rubberband_point();
}

void
test_pmatrix()
{
  vgui::out << "Click on crosshair location\n";
  feature_adder* f = dynamic_cast< feature_adder *>( g_rubber_tab->get_client() );
  f->set_activating_char('t');
  g_rubber_tab->rubberband_point();
}

void
delete_selected_features()
{
  vcl_vector<unsigned> sel = g_easy_tab->get_selected();
  for( unsigned i = 0; i < sel.size(); ++i )
  {
    typedef vcl_vector< point_corresp >::iterator iter_type;

    for( iter_type it = g_point_corresp.begin();
         it !=  g_point_corresp.end(); ++it )
    {
      if( it->soview_id_ == sel[i] )
      {
        g_easy_tab->remove( vgui_soview::id_to_object( sel[i] ) );
        vcl_cout << "Delete:\t"
                 << it->img_loc_ << "\t"
                 << it->world_loc_ << "\n";
        it = g_point_corresp.erase( it );
        break;
      }
    }
  }

  g_easy_tab->post_redraw();
}


void
show_highlighted_features()
{
  unsigned idx = g_easy_tab->get_highlighted();

  typedef vcl_vector< point_corresp >::iterator iter_type;

  for( iter_type it = g_point_corresp.begin();
       it !=  g_point_corresp.end(); ++it )
  {
    if( it->soview_id_ == idx )
    {
      vgui::out << "Img=("
                 << it->img_loc_ << "); World=("
                 << it->world_loc_ << ")\n";
      break;
    }
  }
}


void draw_football_field( vgui_easy2D_tableau_sptr easy,
                          double hashmark_distance );

void draw_overlays()
{
  g_overlay_tab->clear();

  if( g_homography_valid )
  {
    if( g_draw_college_field )
    {
      g_overlay_tab->set_foreground( 1, 0, 0 );
      draw_football_field( g_overlay_tab, 20 );
    }
    if( g_draw_pro_field )
    {
      g_overlay_tab->set_foreground( 1, 0, 0 );
      draw_football_field( g_overlay_tab, 23.583 );
    }
  }

  g_overlay_tab->post_redraw();
}


void
toggle_draw_college_field()
{
  g_draw_college_field = ! g_draw_college_field;
  draw_overlays();
}


void
toggle_draw_pro_field()
{
  g_draw_pro_field = ! g_draw_pro_field;
  draw_overlays();
}


void
estimate_homography()
{
  vcl_vector< vgl_homg_point_2d<double> > wld_pts;
  vcl_vector< vgl_homg_point_2d<double> > img_pts;

  if( g_point_corresp.size() < 4 )
  {
    error_box( "Not enough points (need at least 4)" );
    return;
  }

  for( unsigned i = 0; i < g_point_corresp.size(); ++i )
  {
    point_corresp const& p = g_point_corresp[i];
    img_pts.push_back( vgl_homg_point_2d<double>( p.img_loc_[0], p.img_loc_[1] ) );
    wld_pts.push_back( vgl_homg_point_2d<double>( p.world_loc_[0], p.world_loc_[1] ) );
    if( p.world_loc_[2] != 0 ) {
      vcl_cout << "Ignoring z=" << p.world_loc_[2] << " in " << p.world_loc_ << ", using z=0\n";
    }
  }

  // Obtain the initial linear estimate

  vgl_h_matrix_2d_compute_linear hcl;
  g_homography = hcl.compute( wld_pts, img_pts );
  g_homography_valid = true;

  // Refine with non-linear estimate

  vgl_h_matrix_2d_optimize_lmq hopt( g_homography );
  g_homography = hopt.optimize( wld_pts, img_pts );

  vcl_cout.precision( 30 );
  vcl_cout << "Homography=\n" << g_homography << "\n";

  // Refresh the overlays.
  draw_overlays();

  // Figure out the size of the remapped image, and compute it if it
  // is of a reasonable size.

  double minx = 0;
  double maxx = g_src_img.ni();
  double miny = 0;
  double maxy = g_src_img.nj();

  vcl_vector< vgl_homg_point_2d<double> > corners;
  corners.push_back( vgl_homg_point_2d<double>( minx, miny ) );
  corners.push_back( vgl_homg_point_2d<double>( maxx, miny ) );
  corners.push_back( vgl_homg_point_2d<double>( minx, maxy ) );
  corners.push_back( vgl_homg_point_2d<double>( maxx, maxy ) );

  vgl_h_matrix_2d<double> img_to_wld = g_homography.get_inverse();
  vgl_box_2d<double> box;
  for( unsigned j = 0; j < corners.size(); ++j )
  {
    vgl_point_2d<double> p = img_to_wld * corners[j];
    box.add( p );
  }

  int i0 = int( vcl_floor( box.min_x() ) );
  int j0 = int( vcl_floor( box.min_y() ) );
  unsigned ni = unsigned( vcl_ceil( box.width() ) );
  unsigned nj = unsigned( vcl_ceil( box.height() ) );

  vcl_cout << "Output image is " << ni << "x" << nj;
  if( i0 >= 0 ) vcl_cout << "+";
  vcl_cout << i0;
  if( j0 >= 0 ) vcl_cout << "+";
  vcl_cout << j0 << "\n";

  if( ni < 2000 && nj < 2000 )
  {
    vil_image_view<vxl_byte> warped( ni, nj, 1, g_src_img.nplanes() );
    vidtk::warp_image( g_src_img, warped, g_homography, i0, j0 );
    g_img2_tab->set_image_view( warped );
    g_img2_tab->reread_image();
    g_img2_tab->post_redraw();
  }

}



void
save_correspondences()
{
  static vcl_string filename;
  static vcl_string regexp = "*";

  vgui_dialog dlg( "Save correspondences" );
  dlg.inline_file( "Save to", regexp, filename );
  if( dlg.ask() )
  {
    vcl_ofstream ofstr( filename.c_str() );
    if( ! ofstr )
    {
      error_box( vcl_string("Failed to open ")+filename );
      return;
    }

    for( unsigned i = 0; i < g_point_corresp.size(); ++i )
    {
      ofstr << g_point_corresp[i].img_loc_ << "\t"
            << g_point_corresp[i].world_loc_ << "\n";
    }

    vcl_cout << "Saved " << g_point_corresp.size() << " correspondences\n";
  }
}

void
write_data()
{
  if ( ! g_homography_valid )
  {
    error_box( vcl_string("No homography yet") );
    return;
  }
  {
    vcl_string cfile = vul_sprintf("%s.corresp.txt", g_save_prefix.c_str());
    vcl_ofstream cofstr( cfile.c_str() );
    if ( ! cofstr )
    {
      error_box( vcl_string("Couldn't write ")+cfile );
      return;
    }
    for (unsigned i=0; i<g_point_corresp.size(); i++) {
      cofstr << g_point_corresp[i].img_loc_ << "\t"
	     << g_point_corresp[i].world_loc_ << "\n";
    }
    vcl_cerr << "Wrote " << cfile << "\n";
  }
  {
    vcl_string hfile = vul_sprintf("%s.homog.txt", g_save_prefix.c_str());
    vcl_ofstream hofstr( hfile.c_str() );
    if ( ! hofstr )
    {
    error_box( vcl_string("Couldn't write ")+hfile );
    return;
    }
    hofstr.precision( 30 );
    hofstr << g_homography << "\n";
    vcl_cerr << "Wrote " << hfile << "\n";
  }
  vcl_exit(0);
}
  
  

void
load_correspondences()
{
  static vcl_string filename;
  static vcl_string regexp = "*";
  bool clear_previous = false;

  vgui_dialog dlg( "Load correspondences" );
  dlg.checkbox( "Clear previous correspondences", clear_previous );
  dlg.inline_file( "Load from", regexp, filename );
  if( dlg.ask() )
  {
    vcl_ifstream ifstr( filename.c_str() );
    if( ! ifstr )
    {
      error_box( vcl_string("Failed to open ")+filename );
      return;
    }

    vcl_vector< point_corresp > new_corresp;
    point_corresp pc;
    while( ifstr >> pc.img_loc_ >> pc.world_loc_ )
    {
      new_corresp.push_back( pc );
    }
    vcl_cout << "Loaded " << new_corresp.size() << " correspondences\n";

    if( clear_previous ) {
      g_point_corresp.clear();
      g_easy_tab->clear();
    }

    for( unsigned i = 0; i < new_corresp.size(); ++i )
    {
      g_point_corresp.push_back( new_corresp[i] );
      add_to_display( g_point_corresp.back() );
    }

    g_easy_tab->post_redraw();
  }
}



int
main( int argc, char** argv )
{
  vul_arg<vcl_string> img_filename( "-i", "Image file" );
  vul_arg<vcl_string> save_prefix( "-s", "Save prefix: for .corresp.txt and .homog.txt" );
  vul_arg<vcl_string> test_pmatrix_arg( "-t", "Load a 3x4 pmatrix for testing" );

  vul_arg_parse( argc, argv );

  vil_image_view<vxl_byte> img = vil_load( img_filename().c_str() );
  if( ! img )
  {
    vcl_cout << "Failed to load \"" << img_filename() << "\"\n";
    return EXIT_FAILURE;
  }
  g_src_img = img;

  g_pmatrix_projector = 0;
  if (test_pmatrix_arg.set()) {
    g_pmatrix_projector = new pmatrix_projector( test_pmatrix_arg() );
  }

  vgui::init( argc, argv );

  feature_adder feat_adder_;

  vgui_image_tableau_new img_tab( img );
  vgui_easy2D_tableau_new easy_tab( img_tab );
  vgui_easy2D_tableau_new overlay_easy_tab;
  vgui_active_tableau_new active_tab( overlay_easy_tab );
  // the overlay tab doesn't get any mouse, etc, events.
  active_tab->set_active( false );
  vgui_rubberband_tableau_new rubber_tab( &feat_adder_ );
  vgui_composite_tableau_new comp_tab( easy_tab, rubber_tab, active_tab );
  vgui_viewer2D_tableau_new view_tab( comp_tab );

  vgui_image_tableau_new img2_tab;
  vgui_viewer2D_tableau_new view2_tab( img2_tab );

  vgui_deck_tableau_new deck_tab;
  deck_tab->add( view2_tab );
  deck_tab->add( view_tab );
  vgui_shell_tableau_new shell_tab( deck_tab );

  g_rubber_tab = rubber_tab;
  g_easy_tab = easy_tab;
  g_overlay_tab = overlay_easy_tab;
  g_img_tab = img_tab;
  g_img2_tab = img2_tab;

  vgui_menu main_menu;

  {
    vgui_menu file_menu;
    file_menu.add( "Load correspondences", load_correspondences, vgui_key('o'), vgui_CTRL );
    file_menu.add( "Save correspondences", save_correspondences, vgui_key('s'), vgui_CTRL );
    if (save_prefix.set()) {
      file_menu.add( "Write homography & correspondences and exit", write_data, vgui_key('w'), vgui_CTRL );
      g_save_prefix = save_prefix();
      g_draw_college_field = true;
    }
    main_menu.add( "File", file_menu );
  }

  {
    vgui_menu point_menu;
    point_menu.add( "Add image-world corresp", add_image_world_corresp, vgui_key('a') );
    point_menu.add( "Remove selected features", delete_selected_features, vgui_key('r') );
    point_menu.add( "Show info for highlighted feature", show_highlighted_features, vgui_key('i') );
    point_menu.add( "Test pmatrix", test_pmatrix, vgui_key('t') );
    main_menu.add( "Point", point_menu );
  }

  {
    vgui_menu estimate_menu;
    estimate_menu.add( "Estimate homography", estimate_homography, vgui_key('e') );
    estimate_menu.add( "Toggle draw college field", toggle_draw_college_field );
    estimate_menu.add( "Toggle draw pro field", toggle_draw_pro_field );
    main_menu.add( "Estimate", estimate_menu );
  }

  vgui::run( shell_tab, img.ni(), img.nj(), main_menu, "manual pmatrix" );
}


// Add a line after transforming by the global homography g_homography
vgui_soview2D_lineseg*
add_H_line( vgui_easy2D_tableau_sptr easy,
            double x0, double y0,
            double x1, double y1 )
{
  typedef vgl_homg_point_2d<double> pt_type;
  pt_type p0 = g_homography * pt_type( x0, y0 );
  pt_type p1 = g_homography * pt_type( x1, y1 );
  return easy->add_line( p0.x()/p0.w(), p0.y()/p0.w(),
                         p1.x()/p1.w(), p1.y()/p1.w() );
}


void
draw_football_field( vgui_easy2D_tableau_sptr easy,
                     double hashmark_distance )
{
  // Units are in yards.

  // Sidelines
  add_H_line( easy, -10, 0, 110, 0 );
  add_H_line( easy, -10, 53.333, 110, 53.333 );

  // Ends of end zone
  add_H_line( easy, -10, 0, -10, 53.333 );
  add_H_line( easy, 110, 0, 110, 53.333 );

  // Full yard lines
  for( double x = 0; x <= 100; x += 5 )
  {
    add_H_line( easy, x, 0, x, 53.333 );
  }

  // Hash marks
  double ft = 0.333; // 1 ft = 0.333 yd
  for( double x = 5; x <= 95; x += 5 )
  {
    add_H_line( easy, x-ft, hashmark_distance, x+ft, hashmark_distance );
    add_H_line( easy, x-ft, 53.333-hashmark_distance, x+ft, 53.333-hashmark_distance );
  }
}
