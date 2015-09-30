/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "vgui_process.h"
#include <tracking/gui_process_impl.h>

#include <tracking_gui/image_object.h>
#include <tracking_gui/track.h>
#include <tracking_gui/synced_viewer2D_tableau.h>

#include <video/warp_image.h> 

#include <vul/vul_sprintf.h>

#include <vcl_algorithm.h>

#include <vgui/vgui.h>
#include <vgui/vgui_image_tableau.h>
#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_viewer2D_tableau.h>
#include <vgui/vgui_text_tableau.h>
#include <vgui/vgui_grid_tableau.h>
#include <vgui/vgui_shell_tableau.h>
#include <vgui/vgui_menu.h>
#include <vgui/vgui_utils.h>
#include <vgui/vgui_command.h>
#include <vgui/vgui_selector_tableau.h>

#include <iomanip>

namespace vidtk
{

class vgui_process_impl
{
public:
  bool initialized;

  // Tabs and such //

  vcl_vector < vgui_image_tableau_sptr > img_tabs;
  vcl_vector < vgui_easy2D_tableau_sptr > easy_tabs;
  vcl_vector < vgui_text_tableau_sptr > text_tabs;
  vcl_vector <vgui_selector_tableau_sptr> select_tabs;
  vcl_vector < synced_viewer2D_tableau_sptr > viewer_tabs;

  vgui_grid_tableau_sptr grid_tab;
  int num_imgs;

  vgui_shell_tableau_sptr shell_tab;

  vgui_menu main_menu;
  vgui_menu pause_menu;

  vgui::draw_image_object obj_drawer;
  vgui::draw_image_object filtered_obj_drawer;
  vgui::draw_image_object world_obj_drawer;
  vgui::draw_image_object world_filtered_obj_drawer;
  vgui::draw_track mod_drawer;
  vgui::draw_track world_mod_drawer;
  vgui::draw_track track_drawer;
  vgui::draw_track filtered_track_drawer;
  vgui::draw_track world_track_drawer;
  vgui::draw_track track_drawer_amhi;
};

vgui_process
::vgui_process( vcl_string const& name )
: gui_process( name, "vgui_process" ),
  vgui_impl_( NULL )
{
  vgui_impl_ = new vgui_process_impl;

  vgui_impl_->initialized = false;
}


vgui_process
::~vgui_process()
{
  delete vgui_impl_;
}


bool
vgui_process
::initialize()
{
  if( impl_->disabled ) 
    return true;
  if( vgui_impl_->initialized )
    return true;
  vgui_impl_->initialized = true;

  int pos;

  while(1)
  {
    pos = impl_->tab_names.find( "," );
    impl_->img_names.push_back( impl_->tab_names.substr( 0, pos ) );
    impl_->tab_names.replace( 0, ( pos+1 ), "" );
    if( pos == -1 ) break;
  }

  // extract C string array arguments from white space seperated values in a std::string
  vcl_vector<char *> vgui_args;
  // add an empty string first as a place holder for the executable name
  vgui_args.push_back(new char[1]);
  vgui_args.back()[0] = '\0';
  // extract the rest of the args
  vcl_istringstream iss( impl_->gui_args.c_str() );
  vcl_string token;
  while(iss >> token) {
    char *arg = new char[token.size() + 1];
    vcl_copy(token.begin(), token.end(), arg);
    arg[token.size()] = '\0';
    vgui_args.push_back(arg);
  }
  vgui_args.push_back(0);

  int argc = vgui_args.size()-1;
  // make a copy of the vector for vgui::init to modify
  char **argv = new char* [vgui_args.size()];
  vcl_copy(vgui_args.begin(), vgui_args.end(), argv);
  ::vgui::init( argc, argv );

  for(unsigned int i = 0; i < vgui_args.size(); i++)
    delete[] vgui_args[i];
  delete [] argv;

  if( impl_->show_fg_image_str == "none" )
  {
    impl_->show_fg_image = 0;
  }
  else if( impl_->show_fg_image_str == "raw" )
  {
    impl_->show_fg_image = 1;
  }
  else if( impl_->show_fg_image_str == "morph" )
  {
    impl_->show_fg_image = 2;
  }
  else
  {
    vcl_cerr << "\"" << impl_->show_fg_image_str 
             << "\" is an invalid argument for --impl_->show_fg_image\n";
    return EXIT_FAILURE;
  }

  impl_->show_source_image =
    impl_->show_conn_comp || impl_->show_filtered_conn_comp ||
    impl_->show_mod || impl_->show_filtered_tracks || impl_->show_tracks || 
    impl_->show_image;
  impl_->show_world =
    impl_->show_world_conn_comp || impl_->show_world_filtered_conn_comp ||
    impl_->show_world_mod || impl_->show_world_tracks || impl_->show_world_image;
  impl_->show_amhi_image = 
    impl_->show_amhi_image_pan || impl_->show_amhi_tracks;

  char e[255];
  char t[255];
  for(unsigned i=0;i<impl_->max_tabs;i++)
  {
    vgui_impl_->img_tabs.push_back( vgui_image_tableau_new( ) );
    vgui_impl_->easy_tabs.push_back( vgui_easy2D_tableau_new( ) );
    vgui_impl_->text_tabs.push_back( vgui_text_tableau_new( ) );
  }
  for(unsigned k=0;k<impl_->grid_size;k++)
  {
    vgui_impl_->select_tabs.push_back( vgui_selector_tableau_new( ) );
    for(unsigned j=0;j<impl_->max_tabs;j++)
    {
      sprintf(e, "Easy Tab %d", j);
      sprintf(t, "Text Tab %d", j);
      vgui_impl_->select_tabs[k]->add( vgui_impl_->img_tabs[j], impl_->img_names[j] );
      vgui_impl_->select_tabs[k]->add( vgui_impl_->easy_tabs[j], e );
      vgui_impl_->select_tabs[k]->add( vgui_impl_->text_tabs[j], t );
      vgui_impl_->select_tabs[k]->toggle( impl_->img_names[j] );
      vgui_impl_->select_tabs[k]->toggle( e );
      vgui_impl_->select_tabs[k]->toggle( t );
    }
  }
  for(unsigned i=0;i<vgui_impl_->select_tabs.size();i++)
  {
    synced_viewer2D_tableau_sptr v =
      synced_viewer2D_tableau_new( vgui_impl_->select_tabs[i] );
    
    if( impl_->synched_tabs ) 
    {
      v->sync();
    }
    
    vgui_impl_->viewer_tabs.push_back( v );
  }

  for(unsigned j=0;j<vgui_impl_->select_tabs.size();j++)
  {
    vgui_impl_->select_tabs[j]->toggle( impl_->img_names[j] );
    vgui_impl_->select_tabs[j]->set_active( impl_->img_names[j] );
    vgui_impl_->select_tabs[j]->active_to_top();
    sprintf(e, "Easy Tab %d", j);
    vgui_impl_->select_tabs[j]->toggle( e );
    vgui_impl_->select_tabs[j]->set_active( e );
    vgui_impl_->select_tabs[j]->active_to_top();
    sprintf(t, "Text Tab %d", j);
    vgui_impl_->select_tabs[j]->toggle( t );
    vgui_impl_->select_tabs[j]->set_active( t );
    vgui_impl_->select_tabs[j]->active_to_top();
  }

  impl_->num_imgs = 0;
  vgui_impl_->grid_tab = vgui_grid_tableau_new( );

  if( impl_->show_source_image )
  {
    //vgui_impl_->grid_tab.add_column();
    vgui_impl_->grid_tab->add_next( vgui_impl_->viewer_tabs[0] );
    //vgui_impl_->grid_tab->add_next( viewer_tab );
    ++impl_->num_imgs;
  }
  if( impl_->show_fg_image > 0 )
  {
    vgui_impl_->grid_tab->add_next( vgui_impl_->viewer_tabs[1] );
    //vgui_impl_->grid_tab->add_next( viewer_tab2 );
    ++impl_->num_imgs;
  }
  if( impl_->show_diff_image )
  {
    vgui_impl_->grid_tab->add_next( vgui_impl_->viewer_tabs[2] );
    //vgui_impl_->grid_tab->add_next( viewer_tab3 );
    ++impl_->num_imgs;
  }
  if( impl_->show_world )
  {
    vgui_impl_->grid_tab->add_next( vgui_impl_->viewer_tabs[3] );
    //vgui_impl_->grid_tab->add_next( viewer_tab4 );
    ++impl_->num_imgs;
  }
  if( impl_->show_amhi_image )
  {
    vgui_impl_->grid_tab->add_next( vgui_impl_->viewer_tabs[4] );
    //vgui_impl_->grid_tab->add_next( viewer_tab5 );
    ++impl_->num_imgs;
  }

  vgui_impl_->shell_tab = vgui_shell_tableau_new( vgui_impl_->grid_tab );
  
  vgui_command_sptr step_command = new vgui_command_simple<vgui_process>
    (this, &vgui_process::step_once);
  vgui_command_sptr pause_command = new vgui_command_simple<vgui_process>
    (this, &vgui_process::pause_cb);
  vgui_impl_->pause_menu.add( "Step once", step_command, vgui_key(' ') );
  vgui_impl_->pause_menu.add( "Pause/unpause", pause_command, vgui_key('p') );
  vgui_impl_->main_menu.add( "Pause", vgui_impl_->pause_menu );

  vgui_impl_->obj_drawer.init( vgui_impl_->easy_tabs[0] );
  vgui_impl_->obj_drawer.text_ = vgui_impl_->text_tabs[0];
  vgui_impl_->obj_drawer.show_area_ = !impl_->suppress_number;

  vgui_impl_->filtered_obj_drawer.init( vgui_impl_->easy_tabs[0] );
  vgui_impl_->filtered_obj_drawer.text_ = vgui_impl_->text_tabs[0];
  vgui_impl_->filtered_obj_drawer.show_area_ = !impl_->suppress_number;
  vgui_impl_->filtered_obj_drawer.loc_color_ = vil_rgba<float>( 1, 1, 0, 1 );
  vgui_impl_->filtered_obj_drawer.bbox_color_ = vil_rgba<float>( 1, 1, 0, 1 );
  vgui_impl_->filtered_obj_drawer.boundary_color_ = vil_rgba<float>( 1, 1, 0, 1 );

  vgui_impl_->world_obj_drawer.init( vgui_impl_->easy_tabs[3] );
  vgui_impl_->world_obj_drawer.text_ = vgui_impl_->text_tabs[3];
  vgui_impl_->world_obj_drawer.show_area_ = !impl_->suppress_number;

  vgui_impl_->world_filtered_obj_drawer.init( vgui_impl_->easy_tabs[3] );
  if(!impl_->suppress_number) 
  {
    vgui_impl_->world_filtered_obj_drawer.text_ = vgui_impl_->text_tabs[3];
  }
  vgui_impl_->world_filtered_obj_drawer.show_area_ = !impl_->suppress_number;
  vgui_impl_->world_filtered_obj_drawer.loc_color_ = vil_rgba<float>( 1, 1, 0, 1 );

  vgui_impl_->mod_drawer.init( vgui_impl_->easy_tabs[0] );
  vgui_impl_->world_mod_drawer.init( vgui_impl_->easy_tabs[3] );

  vgui_impl_->track_drawer.init( vgui_impl_->easy_tabs[0] );
  if(!impl_->suppress_number) 
  {
    vgui_impl_->track_drawer.text_ = vgui_impl_->text_tabs[0];
  }
  vgui_impl_->track_drawer.trail_color_ = vil_rgba<float>( 0, 0, 1 );
  vgui_impl_->track_drawer.current_color_ = vil_rgba<float>( 0, 0, 1 );
  vgui_impl_->track_drawer.past_color_ = vil_rgba<float>( 0.9f, 0.9f, 1 );

  vgui_impl_->filtered_track_drawer.init( vgui_impl_->easy_tabs[0] );
  if(!impl_->suppress_number) 
  {
    vgui_impl_->filtered_track_drawer.text_ = vgui_impl_->text_tabs[0];
  }
  vgui_impl_->filtered_track_drawer.trail_color_ = vil_rgba<float>( 1, 1, 1 );
  vgui_impl_->filtered_track_drawer.current_color_ = vil_rgba<float>( 1, 1, 1 );
  vgui_impl_->filtered_track_drawer.past_color_ = vil_rgba<float>( 1, 0.9f, 0.9f );

  vgui_impl_->world_track_drawer.init( vgui_impl_->easy_tabs[3] );
  vgui_impl_->world_track_drawer.trail_color_ = vil_rgba<float>( 0, 0, 1 );
  vgui_impl_->world_track_drawer.current_color_ = vil_rgba<float>( 0, 0, 1 );
  vgui_impl_->world_track_drawer.past_color_ = vil_rgba<float>( 0.9f, 0.9f, 1 );

  vgui_impl_->track_drawer_amhi.init( vgui_impl_->easy_tabs[4] );
  vgui_impl_->track_drawer_amhi.draw_trail_ = false;
  if(!impl_->suppress_number) 
  {
    vgui_impl_->track_drawer_amhi.text_ = vgui_impl_->text_tabs[4];
  }
  vgui_impl_->track_drawer_amhi.trail_color_ = vil_rgba<float>( 0, 0, 1 );
  vgui_impl_->track_drawer_amhi.current_color_ = vil_rgba<float>( 0, 0, 1 );
  vgui_impl_->track_drawer_amhi.past_color_ = vil_rgba<float>( 0.9f, 0.9f, 1 );

  return true;
}

bool
vgui_process
::step()
{
  if( impl_->disabled ) 
    return true;

  if( impl_->num_imgs == 0 || ::vgui::quit_was_called() ) 
    return false;

  if( impl_->show_source_image )
  {
    vgui_impl_->img_tabs[0]->set_image_view( impl_->img );
    vgui_impl_->img_tabs[0]->reread_image();
    vgui_impl_->img_tabs[0]->post_redraw();

    vgui_impl_->easy_tabs[0]->clear();
    vgui_impl_->text_tabs[0]->clear();

    vcl_stringstream buf;
    // Frame number
    buf << "Frame: " << impl_->ts.frame_number( );
    vgui_impl_->text_tabs[0]->add( 10, 20, buf.str() );

    // GSD
    buf.str( "" );
    buf << "GSD: " << std::setprecision( 2 ) << impl_->gsd;
    vgui_impl_->text_tabs[0]->add( 10, 40, buf.str() );

    vgui_impl_->text_tabs[0]->post_redraw();

    if( impl_->show_conn_comp )
    {
      vgui_impl_->easy_tabs[0]->set_point_radius(5);
      vgui_impl_->easy_tabs[0]->set_line_width( 1 );
      vgui_impl_->easy_tabs[0]->set_foreground(1,0,0);
      vcl_vector< vidtk::image_object_sptr > const& objs = impl_->objs;
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        vgui_impl_->obj_drawer.draw( *objs[i] );
      }
    }

    if( impl_->show_filtered_conn_comp )
    {
      vgui_impl_->easy_tabs[0]->set_point_radius(5);
      vgui_impl_->easy_tabs[0]->set_line_width( 1 );
      vgui_impl_->easy_tabs[0]->set_foreground(1,1,0);
      vcl_vector< vidtk::image_object_sptr > const& objs = impl_->filter1_objs;
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        vgui_impl_->filtered_obj_drawer.draw( *objs[i] );
      }
    }

    if( impl_->show_mod )
    {
      vgui_impl_->easy_tabs[0]->set_point_radius(5);
      vgui_impl_->easy_tabs[0]->set_line_width( 1 );
      vgui_impl_->easy_tabs[0]->set_foreground(0,1,0);
      vcl_vector< vidtk::track_sptr > const& trks = impl_->init_tracks;
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        vgui_impl_->mod_drawer.draw( *trks[i] );
      }
    }

    if( impl_->show_filtered_tracks )
    {
      vgui_impl_->easy_tabs[0]->set_point_radius(5);
      vgui_impl_->easy_tabs[0]->set_line_width( 1 );
      vgui_impl_->easy_tabs[0]->set_foreground(1,0,0);
      vcl_vector< vidtk::track_sptr > const& trks = impl_->filter2_tracks;
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        vgui_impl_->filtered_track_drawer.draw( *trks[i] );
      }
    }

    if( impl_->show_tracks )
    {
      vgui_impl_->easy_tabs[0]->set_point_radius(5);
      vgui_impl_->easy_tabs[0]->set_line_width( 1 );
      vgui_impl_->easy_tabs[0]->set_foreground(0,0,1);
      vcl_vector< vidtk::track_sptr > const& trks = impl_->active_tracks;
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        vgui_impl_->track_drawer.draw( *trks[i] );
      }
    }

    vgui_impl_->easy_tabs[0]->post_redraw();
  }

  if( impl_->show_fg_image == 1 )
  {
    vgui_impl_->img_tabs[1]->set_image_view( impl_->fg_image );
    vgui_impl_->img_tabs[1]->reread_image();
    vgui_impl_->img_tabs[1]->post_redraw();
  }
  else if( impl_->show_fg_image == 2 )
  {
    vgui_impl_->img_tabs[1]->set_image_view( impl_->morph_image );
    vgui_impl_->img_tabs[1]->reread_image();
    vgui_impl_->img_tabs[1]->post_redraw();
  }

  if( impl_->show_diff_image )
  {
    vgui_impl_->img_tabs[2]->set_image_view( impl_->diff_image );
    vgui_impl_->img_tabs[2]->reread_image();
    vgui_impl_->img_tabs[2]->post_redraw();
  }

  if( impl_->show_world )
  {
    vgui_impl_->easy_tabs[3]->clear();
    vgui_impl_->text_tabs[3]->clear();

    if( impl_->show_world_conn_comp )
    {
      vgui_impl_->easy_tabs[3]->set_point_radius(5);
      vgui_impl_->easy_tabs[3]->set_line_width( 1 );
      vgui_impl_->easy_tabs[3]->set_foreground(1,0,0);
      vgui_impl_->text_tabs[3]->set_colour(1,0,0);
      vcl_vector< vidtk::image_object_sptr > const& objs = impl_->objs;
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        vgui_impl_->world_obj_drawer.draw_world2d( *objs[i] );
      }
    }

    if( impl_->show_world_filtered_conn_comp )
    {
      vgui_impl_->easy_tabs[3]->set_point_radius(5);
      vgui_impl_->easy_tabs[3]->set_line_width( 1 );
      vgui_impl_->easy_tabs[3]->set_foreground(1,0,0);
      vgui_impl_->text_tabs[3]->set_colour(1,1,0);
      vcl_vector< vidtk::image_object_sptr > const& objs = impl_->filter1_objs;
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        vgui_impl_->world_filtered_obj_drawer.draw_world2d( *objs[i] );
      }
    }

    if( impl_->show_world_mod )
    {
      vgui_impl_->easy_tabs[3]->set_point_radius(5);
      vgui_impl_->easy_tabs[3]->set_line_width( 1 );
      vgui_impl_->easy_tabs[3]->set_foreground(0,1,0);
      vcl_vector< vidtk::track_sptr > const& trks = impl_->init_tracks;
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        vgui_impl_->world_mod_drawer.draw_world2d( *trks[i] );
      }
    }

    if( impl_->show_world_image )
    {
      static vil_image_view<vxl_byte> world( impl_->img.ni(),
                                             impl_->img.nj(),
                                             1,
                                             impl_->img.nplanes() );

      // if we try to move the world (0,0), we have to draw the
      // picture somewhere other than (0,0), which vgui doesn't
      // easily support.
      warp_image( impl_->img, world, impl_->homog );
      vgui_impl_->img_tabs[3]->set_image_view( world );
      vgui_impl_->img_tabs[3]->reread_image();
      vgui_impl_->img_tabs[3]->post_redraw();
    }

    if( impl_->show_world_tracks )
    {
      vgui_impl_->easy_tabs[3]->set_point_radius(5);
      vgui_impl_->easy_tabs[3]->set_line_width( 1 );
      vgui_impl_->easy_tabs[3]->set_foreground(0,0,1);
      vcl_vector< track_sptr > const& trks = impl_->active_tracks;
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        vgui_impl_->world_track_drawer.draw_world2d( *trks[i] );
      }

      vcl_vector< da_so_tracker_sptr > const& trkers = impl_->active_trackers;
      for( unsigned i = 0; i < trkers.size(); ++i )
      {
        vnl_double_2 loc;
        vnl_double_2x2 cov;
        if( trkers[i]->last_update_time() == 
            impl_->ts.time_in_secs() )
        {
          loc = trkers[i]->get_current_location();
          cov = trkers[i]->get_current_location_covar();
          vgui_impl_->world_track_drawer.trail_color_ = vil_rgba<float>( 0, 0, 1 );
          vgui_impl_->text_tabs[3]->set_colour(0,0,1);
        }
        else
        {
          trkers[i]->predict( impl_->ts.time_in_secs(), 
                              loc, cov );
          vgui_impl_->world_track_drawer.trail_color_ = vil_rgba<float>( .7f, .7f, 1 );
          vgui_impl_->text_tabs[3]->set_colour(0.7f,0.7f,1);
        }
        vgui_impl_->world_track_drawer.draw_gate( loc, cov, impl_->tracker_gate_sigma );
        vgui_impl_->text_tabs[3]->add( loc[0], loc[1], 
                        vul_sprintf("%d(%d)",trks[i]->id(), i) );
      }
    }
  }

  if( impl_->show_amhi_image )
  {
    impl_->amhi_image_enabled = true;
    vgui_impl_->img_tabs[4]->set_image_view( impl_->amhi_image );
    vgui_impl_->img_tabs[4]->reread_image();
    vgui_impl_->img_tabs[4]->post_redraw();

    vgui_impl_->easy_tabs[4]->clear();
    vgui_impl_->text_tabs[4]->clear();

    if( impl_->show_amhi_tracks )
    {
      vgui_impl_->easy_tabs[4]->set_point_radius(5);
      vgui_impl_->easy_tabs[4]->set_line_width( 1 );
      vgui_impl_->easy_tabs[4]->set_foreground(0,0,1);
      vcl_vector< vidtk::track_sptr > const& trks = impl_->active_tracks;
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        if( (*trks[i]).last_state()->time_ == impl_->ts )
        {
          vgui_impl_->track_drawer_amhi.draw( *trks[i] );
        }
      }
    }

    vgui_impl_->easy_tabs[4]->post_redraw();
  }

  if ( ( (impl_->pause_at != 0) && 
       (impl_->ts.frame_number() == impl_->pause_at)))
  {
    impl_->b_pause = true;
  }


  if( impl_->num_imgs > 0 )
  {
    static bool first_frame = true;
    if( first_frame )
    {
      first_frame = false;
      ::vgui::adapt( vgui_impl_->shell_tab, 
                     impl_->img.ni()*impl_->num_imgs, 
                     impl_->img.nj(), 
                     vgui_impl_->main_menu );
    }
    ::vgui::run_till_idle();
    impl_->output_image = vgui_utils::colour_buffer_to_view();
    while( impl_->b_pause && ! impl_->b_step && ! ::vgui::quit_was_called() )
    {
      ::vgui::run_till_idle();
    }
    impl_->b_step = false;
  }

  return true;

}

bool
vgui_process
::persist()
{
  if( impl_->num_imgs > 0 )
  {
    if( ::vgui::quit_was_called() ||
        ! impl_->persist_gui_at_end )
    {
      return true;
    }
    else
    {
      return ::vgui::run();
    }
  }
  else
  {
    return true;
  }

}

} // namespace

