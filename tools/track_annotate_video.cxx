/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_sstream.h>
#include <vcl_fstream.h>
#include <vcl_algorithm.h>

#include <vul/vul_sprintf.h>
#include <vul/vul_awk.h>

#include <vnl/vnl_matrix_fixed.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_3.h>

#include <vgui/vgui.h>
#include <vgui/vgui_image_tableau.h>
#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_viewer2D_tableau.h>
#include <vgui/vgui_grid_tableau.h>
#include <vgui/vgui_shell_tableau.h>
#include <vgui/vgui_menu.h>
#include <vgui/vgui_dialog.h>
#include <vgui/vgui_macro.h>
#include <vgui/vgui_adaptor.h>
#include <vgui/vgui_window.h>
#include <vgui/vgui_statusbar.h>
#include <vgui/vgui_projection_inspector.h>

#include <utilities/config_block.h>
#include <utilities/timestamp.h>

#include <video/generic_frame_process.h>
#include <video/vidl_ffmpeg_writer_process.h>

static const char *RGB_names[] = {"unsel/past","unsel/future","sel/past","sel/future","pts",0};
static const char *future_names[] = {"dotted", "solid", "none",0};
typedef vidtk::generic_frame_process< vxl_byte > video_src_type;

struct image_track_pos
{
  double ctr_x, ctr_y, bb_width, bb_height;
  vidtk::timestamp frame_num;
  image_track_pos() :
    ctr_x( -1.0 ), ctr_y( -1.0 ), bb_width( -1.0 ), bb_height( -1.0 ), frame_num( 0 )
  {}
  image_track_pos( double x, double y, double w, double h, unsigned f ) :
    ctr_x( x ), ctr_y( y ), bb_width( w ), bb_height( h )
  {
    frame_num.set_frame_number(f);
  }
  vcl_vector<double> get_bb_coords() const {
    vcl_vector<double> bb;
    double hw = bb_width / 2.0;
    double hh = bb_height / 2.0;
    bb.push_back( ctr_x - hw );  bb.push_back( ctr_y - hh );
    bb.push_back( ctr_x + hw );  bb.push_back( ctr_y - hh );
    bb.push_back( ctr_x + hw );  bb.push_back( ctr_y + hh );
    bb.push_back( ctr_x - hw );  bb.push_back( ctr_y + hh );
    return bb;
  }

};

struct image_track
{
  vcl_vector< image_track_pos > locs;
  vcl_string label;
  unsigned track_id;
  bool suppressed;
  image_track() : label(""), track_id(-1), suppressed(false) {}
  image_track( const vcl_string& s, unsigned id ) : label(s), track_id(id), suppressed(false) {}
  void reset() { locs.clear(); label=""; track_id = -1; suppressed =false; }
};


struct GUI_selection
{
  bool valid;
  vcl_string trackset_tag;
  unsigned track_id, frame_num, frame_index_in_track;
  bool bb_selected;
  GUI_selection() : valid(false), trackset_tag(""), track_id(-1), frame_num(-1), frame_index_in_track(-1), bb_selected(false) {}
  GUI_selection( const vcl_string& tag, unsigned t, unsigned f, unsigned fi ) : valid(true), trackset_tag(tag), track_id(t), frame_num(f), frame_index_in_track(fi), bb_selected(false) {}
  void print_self( vcl_ostream& os ) const { os << "GUI_sel v/" << valid << " set/" << trackset_tag << " track/" << track_id << " frame/" << frame_num << " index/" << frame_index_in_track << " bbsel/" << bb_selected << "\n"; }
  vcl_string to_str() const {
    if (!valid) return "invalid";
    vcl_ostringstream oss;
    oss << "'" << trackset_tag << "' T:" << track_id << " F: " << frame_num << " (" << frame_index_in_track << ")";
    return oss.str();
  }

};


struct GUI_RGB
{
  unsigned r,g,b;
  void to_opengl(double& R, double& G, double& B) const { R=r/255.0; G=g/255.0; B=b/255.0; }
  GUI_RGB( unsigned R, unsigned G, unsigned B ): r(R), g(G), b(B) {}
  vcl_string to_str() {
    vcl_ostringstream oss;
    oss << r << " " << g << " " << b;
    return oss.str();
  }
  GUI_RGB( const vcl_string& s ) {
    vcl_istringstream iss(s);
    iss >> r >> g >> b;  // yah, that's robust! :)
  }
};

struct track_display_state
{
  enum {RGB_unsel_past, RGB_unsel_future, RGB_sel_past, RGB_sel_future, RGB_pt_sel, RGB_top};
  vcl_vector< GUI_RGB > RGB_sets;

  enum {show_future_dotted, show_future_solid, show_future_none, future_top};
  unsigned show_future_flag;

  bool only_show_while_active;
  bool hide; // set at the class level
  bool always_show;
  unsigned time_window;
  bool show_bounding_box;

  track_display_state()
  {
    RGB_sets.push_back( GUI_RGB(  77, 204,  77 )); // unselected past tracks
    RGB_sets.push_back( GUI_RGB(  77, 204, 204 )); // unselected future tracks
    RGB_sets.push_back( GUI_RGB( 255, 102, 102 )); // selected past tracks
    RGB_sets.push_back( GUI_RGB( 102, 255, 102 )); // selected future tracks
    RGB_sets.push_back( GUI_RGB( 255, 255, 255 )); // selected points
    show_future_flag = show_future_dotted;
    only_show_while_active = false;
    hide = false;
    always_show = false;
    time_window = 10;
    show_bounding_box = true;
  }

  bool linestrip_opengl( unsigned frame_index, unsigned current_index, bool selected ) const
  {
    double R,G,B;
    if (frame_index > current_index) {
      if (show_future_flag == show_future_none) return false;
      unsigned rgb_index = (selected) ? RGB_sel_future: RGB_unsel_future;
      RGB_sets[rgb_index].to_opengl( R, G, B );
      glLineWidth( 2.0 );
      glColor3f( R, G, B );
      glLineStipple( 2, (show_future_flag == show_future_dotted) ? 0xAAAA : 0xFFFF );
      glEnable( GL_LINE_STIPPLE );
    } else {
      unsigned rgb_index = (selected) ? RGB_sel_past: RGB_unsel_past;
      RGB_sets[rgb_index].to_opengl( R, G, B );
      glColor3f( R, G, B );
      glLineWidth( 2.0 );
      glLineStipple( 1, 0xFFFF );
      glEnable( GL_LINE_STIPPLE );
    }
    return true;
  }

  bool points_opengl( unsigned frame_index, unsigned current_index, bool track_selected,
		      bool pt_selected ) const
  {
    unsigned rgb_index = RGB_top;
    if (pt_selected) {
      rgb_index = RGB_pt_sel;
    } else {
      if (frame_index <= current_index) {
	rgb_index = (track_selected) ? RGB_sel_past : RGB_unsel_past;
      } else {
	if (show_future_flag == show_future_none) return false;
	rgb_index = (track_selected) ? RGB_sel_future : RGB_unsel_future;
      }
    }
    double R,G,B;
    RGB_sets[rgb_index].to_opengl( R, G, B );
    glColor3f( R, G, B );
    return true;
  }
};

struct display_state_library
{
  vcl_map< vcl_string, track_display_state > ds_map;
  typedef vcl_map< vcl_string, track_display_state >::const_iterator ds_map_cit;

  vcl_map< vcl_pair< vcl_string, unsigned >, vcl_string > exception_map; // dataset:track_id -> ds_tag
  typedef vcl_map< vcl_pair< vcl_string, unsigned >, vcl_string >::const_iterator ds_map_ex_cit;

  vcl_vector< vcl_string > labels() const
  {
    vcl_vector< vcl_string > r;
    for (ds_map_cit i=ds_map.begin(); i != ds_map.end(); ++i) {
      r.push_back( i->first );
    }
    return r;
  }

  vcl_vector< unsigned > get_exceptions( const vcl_string& dataset ) const
  {
    vcl_vector< unsigned > ex_frames;
    for (ds_map_ex_cit i = exception_map.begin(); i != exception_map.end(); ++i)
    {
      const vcl_pair< vcl_string, unsigned>& key = i->first;
      if (key.first == dataset) ex_frames.push_back( key.second );
    }
    return ex_frames;
  }

  void link_display_state( const GUI_selection& s, const vcl_string& tgt )
  {
    if (!s.valid) return;
    vcl_pair< vcl_string, unsigned > key = vcl_make_pair( s.trackset_tag, s.track_id );
    vcl_ostringstream oss;
    oss << s.trackset_tag << "/" << s.track_id;
    exception_map[ key ] = tgt;
    vcl_cerr << "Linked display state for " << s.to_str() << " to " << tgt << "\n";
  }


  void add_display_state( const GUI_selection& s, const track_display_state& ds )
  {
    if (!s.valid) return;
    vcl_pair< vcl_string, unsigned > key = vcl_make_pair( s.trackset_tag, s.track_id );
    vcl_ostringstream oss;
    oss << s.trackset_tag << "/" << s.track_id;
    exception_map[ key ] = oss.str();
    ds_map[ oss.str() ] = ds;
    vcl_cerr << "Added display state for " << s.to_str() << " to " << oss.str() << "\n";

  }

  track_display_state pick_display_state( const vcl_string& dataset_tag, unsigned track_id ) const
  {
    vcl_string tag = dataset_tag;

    vcl_pair< vcl_string, unsigned > exception_key = vcl_make_pair( dataset_tag, track_id );
    ds_map_ex_cit i = exception_map.find( exception_key );
    if (i != exception_map.end()) {
      tag = i->second;
    }

    ds_map_cit j = ds_map.find( tag );
    if (j == ds_map.end()) {
      vcl_cerr << "DSL: " << dataset_tag << " : " << track_id << " gave key '" << tag
	       << "' but no such display state could be found; using default\n";
      return track_display_state();
    } else {
      //      vcl_cerr << "DSL pick: " << dataset_tag << "/" << track_id << " -> " << tag << "\n";
      return j->second;
    }
  }

};


struct image_track_db_state
{

  vcl_map< unsigned, image_track > track_map;
  typedef vcl_map< unsigned, image_track >::iterator track_map_it;
  typedef vcl_map< unsigned, image_track >::const_iterator track_map_cit;

  unsigned current_track_id;
  unsigned max_track_id;
  track_display_state display_state; // default display state

  vcl_vector<unsigned> active_set;
  unsigned active_set_cf;

  image_track_db_state(): current_track_id( -1 ), max_track_id( -1 ), active_set_cf(-1) {}

  bool id_is_valid( unsigned id ) const { return id != static_cast<unsigned>(-1); }
  bool current_track_is_valid() const { return this->id_is_valid( this->current_track_id ); }

  vcl_pair< unsigned, unsigned> frame_range() const {
    unsigned min_frame=0, max_frame=0;
    bool first_frame = true;
    for (track_map_cit i=track_map.begin(); i != track_map.end(); ++i) {
      const image_track& t = i->second;
      for (unsigned j=0; j<t.locs.size(); j++) {
	const image_track_pos& p = t.locs[j];
	unsigned this_f = p.frame_num.frame_number();
	if ( (first_frame) || (this_f < min_frame) ) min_frame = this_f;
	if ( (first_frame) || (this_f > max_frame) ) max_frame = this_f;
	first_frame = false;
      }
    }
    return vcl_make_pair( min_frame, max_frame );
  }

  vcl_pair<unsigned, unsigned> suppress( unsigned threshold )
  {
    vcl_map< unsigned, unsigned > start_map; // start_frame_id -> count
    for (track_map_cit i=track_map.begin(); i != track_map.end(); ++i) {
      start_map[ i->second.locs[0].frame_num.frame_number() ]++;
    }
    unsigned s_count=0, t_count=0;
    for (track_map_it i=track_map.begin(); i != track_map.end(); ++i) {
      t_count++;
      if (start_map[ i->second.locs[0].frame_num.frame_number() ] > threshold ) {
	i->second.suppressed = true;
	s_count++;
      } else {
	i->second.suppressed = false;
      }
    }
    return vcl_make_pair( s_count, t_count );
  }

  void generate_active_set( unsigned current_frame, const track_display_state& ds )
  {
    if (current_frame != active_set_cf) {
      active_set.clear();
      unsigned time_low = (current_frame < ds.time_window) ? 0 : current_frame - ds.time_window;
      unsigned forward_delta = (ds.show_future_flag == track_display_state::show_future_none) ? 0 : ds.time_window;
      unsigned time_high = current_frame+forward_delta;
      for (track_map_cit i = track_map.begin(); i != track_map.end(); ++i) {
	const vcl_vector< image_track_pos >& tlocs = i->second.locs;

	if (ds.only_show_while_active) {
	  if (tlocs[ 0 ].frame_num.frame_number() > current_frame) continue;
	  if (tlocs[ tlocs.size()-1 ].frame_num.frame_number() < current_frame) continue;
	} else {
	  if (tlocs[ 0 ].frame_num.frame_number() > time_high) continue;
	  if (tlocs[ tlocs.size()-1 ].frame_num.frame_number() < time_low) continue;
	}

	active_set.push_back( i->first );
      }
      active_set_cf = current_frame;
    }
  }


  bool get_linestream( vcl_ifstream& is, vcl_istringstream& ss ) {
    vcl_string tmp;
    vcl_getline( is, tmp );
    if (is.eof()) return false;
    ss.str( tmp );
    return true;
  }

  bool load_from_file( const vcl_string& fn, bool clear_prior ) {
    vcl_ifstream is( fn.c_str() );
    if (!is) {
      vcl_cerr << "Couldn't open '" << fn << "'\n";
      return false;
    }

    vcl_istringstream ss;

    if (clear_prior) track_map.clear();

    unsigned max_id;
    if (!get_linestream( is, ss )) { vcl_cerr << "EOF parsing max_id\n"; return false; }
    if (!(ss >> max_id)) { vcl_cerr << "Couldn't parse max_id\n"; return false; }
    this->max_track_id = max_id;

    unsigned ntracks;
    if (!get_linestream( is, ss )) { vcl_cerr << "EOF parsing ntracks\n"; return false; }
    if (!(ss >> ntracks)) { vcl_cerr << "Couldn't parse ntracks\n"; return false; }


    for (unsigned i=0; i<ntracks; i++) {
      vcl_string label;

      if (!get_linestream( is, ss )) { vcl_cerr << "EOF parsing track " << i << " label\n"; return false; }
      if (!(ss >> label)) { vcl_cerr << "Couldn't parse track " << i << " label\n"; return false; }

      unsigned track_id;

      if (!get_linestream( is, ss )) { vcl_cerr << "EOF parsing track " << i << " id\n"; return false; }
      if (!(ss >> track_id)) { vcl_cerr << "Couldn't parse track " << i << " id\n"; return false; }

      unsigned npts;

      if (!get_linestream( is, ss )) { vcl_cerr << "EOF parsing track " << i << " npts\n"; return false; }
      if (!(ss >> npts)) { vcl_cerr << "Couldn't parse track " << i << " npts\n"; return false; }

      image_track new_track( label, track_id );
      for (unsigned j=0; j<npts; j++) {

	if (!get_linestream( is, ss )) {
	  vcl_cerr << "EOF parsing track id " << track_id << " pt " << j << "\n";
	  return false;
	}

	double cx, cy, bw, bh;
	unsigned f;
	if (!(ss >> cx >> cy >> bw >> bh >> f )) {
	  vcl_cerr << "Couldn't parse track id " << track_id << " pt " << j << " data\n";
	  return false;
	}
	new_track.locs.push_back( image_track_pos( cx, cy, bw, bh, f ));
      }

      if (track_map.find( track_id ) != track_map.end()) {
	vcl_cerr << "Duplicate track id " << track_id << "\n";
	return false;
      }
      track_map[ track_id ] = new_track;
      if ( (! id_is_valid(max_track_id)) || (track_id > max_track_id)) {
	max_track_id = track_id;
      }
    }
    current_track_id = -1;
    return true;
  }

  bool save_native( const vcl_string& fn ) {
    vcl_ofstream os( fn.c_str() );
    if (!os) {
      vcl_cerr << "Couldn't write '" << fn << "'\n";
      return false;
    }
    os << max_track_id << "\n";
    os << track_map.size() << "\n";
    for (track_map_cit i=track_map.begin(); i != track_map.end(); ++i) {
      const image_track& t = i->second;
      os << t.label << "\n";
      os << t.track_id << "\n";
      os << t.locs.size() << "\n";
      for (unsigned j=0; j<t.locs.size(); j++) {
	const image_track_pos& p = t.locs[j];
	unsigned f= p.frame_num.frame_number();
	os << p.ctr_x << " " << p.ctr_y << " " << p.bb_width << " " << p.bb_height
	   << " " << f << "\n";
      }
    }
    return true;
  }

  bool save_matlab( const vcl_string& fn, const vnl_double_3x3& proj, unsigned /*offset*/ ) {
    vcl_ofstream os( fn.c_str() );
    if (!os) {
      vcl_cerr << "Couldn't write '" << fn << "'\n";
      return false;
    }
    for (track_map_cit i=track_map.begin(); i != track_map.end(); ++i) {
      const image_track& t = i->second;
      double last_ground_x=0.0, last_ground_y=0.0;
      for (unsigned j=0; j<t.locs.size(); j++) {
	const image_track_pos& p = t.locs[j];
	vnl_matrix_fixed<double, 1, 3> pt;
	pt.put(0,0, p.ctr_x);
	pt.put(0,1, p.ctr_y);
	pt.put(0,2, 1.0);
	vnl_matrix_fixed<double, 1,3> gpt = pt * proj;
	double ground_x = gpt.get(0,0) / gpt.get(0,2);
	double ground_y = gpt.get(0,1) / gpt.get(0,2);
	double ground_vx = (j == 0) ? 0 : ground_x - last_ground_x;
	double ground_vy = (j == 0) ? 0 : ground_y - last_ground_y;
	vcl_vector<double> bb = p.get_bb_coords();

	os << i->first << " "                // track id
	   << t.locs.size() << " "           // n frames in track
	   << p.frame_num.frame_number() << " "      // frame num
	   << ground_x << " "                // ground pos X
	   << ground_y << " "                // ground pos Y
	   << ground_vx << " "               // ground velocity X
	   << ground_vy << " "               // ground velocity Y
	   << p.ctr_x << " "                 // image pos x
	   << p.ctr_y << " "                 // image pos y
	   << bb[0]   << " "                 // bounding box corner 1 x
	   << bb[1]   << " "                 // bounding box corner 1 y
	   << bb[4]   << " "                 // bounding box corner 2 x
	   << bb[5]   << " "                 // bounding box corner 2 y
	   << -1 << " "                      // MOD size
	   << t.label
	   << "\n";

	last_ground_x = ground_x;
	last_ground_y = ground_y;
      }
    }
    return true;
  }

  bool load_matlab( const vcl_string& fn ) {
    vcl_ifstream is( fn.c_str() );
    if (!is) {
      vcl_cerr << "Couldn't read '" << fn << "'\n";
      return false;
    }
    image_track t;
    vul_awk awk(is);
    while (awk) {
      unsigned track_id, frame_num;    // fields 0, 2
      double bb_c1x, bb_c1y, bb_c2x, bb_c2y; // fields 9, 10, 11, 12
      vcl_ostringstream oss;
      oss << awk[0] << " " << awk[2] << " " << awk[9] << " " << awk[10] << " " << awk[11] << " " << awk[12];
      vcl_istringstream ss( oss.str() );
      if (!(ss >> track_id >> frame_num >> bb_c1x >> bb_c1y >> bb_c2x >> bb_c2y )) {
	vcl_cerr << "LoadMatlab: file '" << fn << "' line " << awk.NR()
		 << ": couldn't parse '" << awk.line() << "'\n";
	return false;
      }

      if (track_id != t.track_id) {
	if (t.track_id != static_cast<unsigned>( -1 )) {
	  if (track_map.find( t.track_id ) != track_map.end()) {
	    vcl_cerr << "Duplicate track ID " << t.track_id << "; aborting load\n";
	    return false;
	  }
	  track_map[ t.track_id ] = t;
	}
	t.reset();
	t.track_id = track_id;
	t.label = "computed";
      }

      double ctr_x = (bb_c1x+bb_c2x)/2.0;
      double ctr_y = (bb_c1y+bb_c2y)/2.0;
      t.locs.push_back( image_track_pos( ctr_x, ctr_y, bb_c2x-bb_c1x, bb_c2y-bb_c1y, frame_num ));
      ++awk;
    }

    // add the straggler
    if (t.track_id != static_cast<unsigned>( -1 )) {
      if (track_map.find( t.track_id ) != track_map.end()) {
	vcl_cerr << "Duplicate track ID " << t.track_id << "; aborting load\n";
	return false;
      }
      track_map[ t.track_id ] = t;
    }
    return true;
  }






  bool point_defined_at_frame( unsigned f ) const {
    if (! current_track_is_valid()) return false;
    track_map_cit i = track_map.find( current_track_id );
    assert( i != track_map.end() );
    const image_track& t = i->second;
    for (unsigned j=0; j<t.locs.size(); j++) {
      if (t.locs[j].frame_num.frame_number() == f) return true;
    }
    return false;
  }

};

struct image_track_db_set
{
  vcl_map< vcl_string, image_track_db_state > tracks;
  typedef vcl_map<vcl_string, image_track_db_state>::const_iterator trackset_cit;
  typedef vcl_map<vcl_string, image_track_db_state>::iterator trackset_it;

  vcl_vector< vcl_string > labels() const {
    vcl_vector< vcl_string > r;
    for (trackset_cit i=tracks.begin(); i != tracks.end(); ++i) {
      r.push_back( i->first );
    }
    return r;
  }

  GUI_selection pick_point( unsigned current_frame, double ix, double iy,
			    bool pick_bb_corners = false, double thresh = 10.0 )
  {
    double min_dist = 1.0e6;
    GUI_selection min_sel;
    for (trackset_cit ts = tracks.begin(); ts != tracks.end(); ++ts)
    {
      const image_track_db_state& tdb = ts->second;
      for (unsigned i=0; i<tdb.active_set.size(); i++)
      {
	unsigned id = tdb.active_set[i];
	image_track_db_state::track_map_cit it = tdb.track_map.find( id );
	assert( it != tdb.track_map.end() );
	const image_track& t = it->second;
	for (unsigned j=0; j<t.locs.size(); j++) {
	  const image_track_pos& p = t.locs[j];
	  // allow future picks if pick_bb_corners == false (bogus use of flag)
	  if ((pick_bb_corners) && p.frame_num.frame_number() > current_frame) continue;
	  double d = vcl_sqrt((p.ctr_x - ix) * (p.ctr_x - ix))+((p.ctr_y - iy) * (p.ctr_y - iy ));
	  if (d < thresh) {
	    if ( (!min_sel.valid) || (d < min_dist) ) {
	      min_sel = GUI_selection( ts->first, id, p.frame_num.frame_number(), j );
	      min_dist = d;
	    }
	  }

	  // pick the bounding box if it's the current frame and we're nearer a corner of it
	  // (and if we're in ground-truth mode)

	  if ((p.frame_num.frame_number() == current_frame) && (pick_bb_corners)) {
	    vcl_vector<double> bb = p.get_bb_coords();
	    for (unsigned k=0; k<8; k+=2) {
	      double d = vcl_sqrt( ((bb[k]-ix)*(bb[k]-ix)) + ((bb[k+1]-iy)*(bb[k+1]-iy)) );
	      if (d < thresh) {
		if ((!min_sel.valid) || (d < min_dist) ) {
		  min_sel = GUI_selection( ts->first, id, p.frame_num.frame_number(), j );
		  min_sel.bb_selected = true;
		  min_dist = d;
		}
	      }
	    }
	  }

	} // locs in track
      } // track in trackset
    } // trackset

    return min_sel;
  }
};


struct image_track_tableau : public vgui_easy2D_tableau
{
  image_track_db_set& track_db_set;
  video_src_type& video_src;
  vgui_statusbar* status_bar;
  vgui_image_tableau_sptr img_tab;
  GUI_selection selected_pt;
  bool add_point_mode;
  display_state_library ds_library;
  bool verbose_draw;
  bool reverse_draw_exceptions;


  image_track_tableau( image_track_db_set& t, video_src_type& v, vgui_image_tableau_sptr child )
    : vgui_easy2D_tableau( child ), track_db_set( t ), video_src( v ), status_bar( 0 ),
      img_tab( child ),
      add_point_mode(false), verbose_draw(false), reverse_draw_exceptions(false)
  {
  }

  void draw_track( const image_track& track, const track_display_state& display_state,
		   const GUI_selection& sel_pt, const vcl_string& src_trackset_tag, bool selected )
  {
    const vcl_vector< image_track_pos > pset = track.locs;
    if (pset.empty()) return;

    unsigned cf = (video_src.is_initialized()) ? video_src.timestamp().frame_number() : 0;
    if (pset.size() > 1) {
      for (unsigned i=0; i<pset.size()-1; i++) {
	if (display_state.linestrip_opengl( pset[i+1].frame_num.frame_number(), cf, selected )) {
	  glBegin( GL_LINE_STRIP );
	  glVertex2f( pset[i].ctr_x, pset[i].ctr_y );
	  glVertex2f( pset[i+1].ctr_x, pset[i+1].ctr_y );
	  glEnd();
	}
      }
    }

    // all points
    glPointSize( 4.0 );
    glBegin( GL_POINTS );
    for (unsigned i=0; i<pset.size(); i++) {
      bool gui_selected = (sel_pt.valid) && (sel_pt.frame_num == pset[i].frame_num.frame_number()) &&
	(track.track_id == sel_pt.track_id) && (src_trackset_tag == sel_pt.trackset_tag);
      if (display_state.points_opengl( pset[i].frame_num.frame_number(), cf, selected, gui_selected )) {
	glVertex2f( pset[i].ctr_x, pset[i].ctr_y );
      }
    }
    glEnd();

    // ...and the bounding box
    if (display_state.show_bounding_box) {
      for (unsigned i=0; i<pset.size(); i++) {
	const image_track_pos& p = pset[i];
	if (p.frame_num.frame_number() == cf) {
	  // set the color as if it were points
	  bool gui_selected = (sel_pt.valid) && (sel_pt.frame_num == pset[i].frame_num.frame_number()) &&
	    (track.track_id == sel_pt.track_id) && (src_trackset_tag == sel_pt.trackset_tag);
	  display_state.points_opengl( pset[i].frame_num.frame_number(), cf, selected, gui_selected );

	  glLineStipple( 1, 0xFFFF );
	  vcl_vector<double> bb = p.get_bb_coords();
	  glBegin( GL_LINE_LOOP );
	  for (unsigned k=0; k<8; k+=2) {
	    glVertex2f( bb[k], bb[k+1] );
	}
	  glEnd();
	}
      }
    }
  }

  bool handle( const vgui_event& e )
  {
    if (e.type == vgui_DRAW) {
      bool rc = vgui_easy2D_tableau::handle( e );
      if (!video_src.is_initialized()) return rc;

      unsigned cf = video_src.timestamp().frame_number();

      if (verbose_draw) vcl_cerr << "DRAW at " << cf << ":\n";
      unsigned sum_drawn = 0;
      unsigned min_ts = 0, max_ts = 0;
      bool first_time = true;
      for (image_track_db_set::trackset_it t = track_db_set.tracks.begin();
	   t != track_db_set.tracks.end();
	   ++t )
      {
	unsigned class_drawn = 0, class_supp = 0, hidden = 0;
	if (verbose_draw) vcl_cerr << t->first << ":\n";

	image_track_db_state& tdb = t->second;
	tdb.generate_active_set( cf, tdb.display_state );
	for (unsigned i=0; i<tdb.active_set.size(); i++)
	{
	  const image_track& track = tdb.track_map[ tdb.active_set[i] ];
	  if (!track.suppressed) {

	    unsigned t0 = track.locs[0].frame_num.frame_number();
	    if ((first_time) || (t0 > max_ts)) max_ts = t0;
	    if ((first_time) || (t0 < min_ts)) min_ts = t0;
	    first_time = false;

	    const track_display_state& ds = ds_library.pick_display_state( t->first, track.track_id );
	    if ((!ds.hide) || (ds.always_show)) {
	      bool track_sel = (track.track_id == tdb.current_track_id);
	      this->draw_track( track, ds, selected_pt, t->first, track_sel );
	      ++sum_drawn;
	      ++class_drawn;
	    } else {
	      ++hidden;
	    }
	  } else {
	    ++class_supp;
	  }
	}

	if (verbose_draw) vcl_cerr << "  " << class_drawn << " drawn; "
				   << class_supp << " suppressed; " << hidden << " hidden\n";
      }

      // draw exceptions over top of prior tracks (some duplication)

      // massive hack!

      vcl_vector< image_track_db_set::trackset_it > draw_list;
      for (image_track_db_set::trackset_it t = track_db_set.tracks.begin();
	   t != track_db_set.tracks.end();
	   ++t )
      {
	draw_list.push_back( t );
      }

      if (reverse_draw_exceptions) {
	vcl_reverse( draw_list.begin(), draw_list.end() );
      }

      for (unsigned foo=0; foo<draw_list.size(); foo++) {
	image_track_db_set::trackset_it t = draw_list[foo];
	image_track_db_state& tdb = t->second;
	vcl_vector< unsigned > ex_frames = ds_library.get_exceptions( t->first );
	for (unsigned i=0; i<ex_frames.size(); i++)
	{
	  const image_track& track = tdb.track_map[ ex_frames[i] ];
	  if (!track.suppressed) {

	    const track_display_state& ds = ds_library.pick_display_state( t->first, track.track_id );
	    if ((!ds.hide) || (ds.always_show)) {
	      bool track_sel = (track.track_id == tdb.current_track_id);
	      this->draw_track( track, ds, selected_pt, t->first, track_sel );
	    }
	  }
	}
      }

      if (verbose_draw) vcl_cerr << "Min TS: " << min_ts << "; max TS: " << max_ts << "\n";
      verbose_draw = false;
      if (this->status_bar) {
	vcl_ostringstream oss;
	if (video_src.is_initialized()) {
	  oss << "Frame " << video_src.timestamp().frame_number() << " ";
	} else {
	  oss << "No video loaded ";
	}

	oss << "; " << track_db_set.tracks.size() << " sets loaded; ";
	oss << sum_drawn << " tracks on screen; ";

	if (! selected_pt.valid) {
	  oss << "Selected: nothing";
	} else {
	  const vcl_string& label =
	    track_db_set.tracks[ selected_pt.trackset_tag ].track_map[ selected_pt.track_id].label;
	  oss << "Selected: " << selected_pt.to_str() << " [ " << label << " ] ";
	}
	this->status_bar->write( oss.str().c_str() );
      }

      return rc;
    }

    if (e.type == vgui_KEY_PRESS) {
      //      vcl_cerr << "Got " << e.key << "\n";
      bool handled = false;

      // delete: remove track point (only if last point, alas)
      if (e.key == vgui_DELETE) {
	vcl_cerr << "delete needs upgrading\n";
	handled = true;
#if 0
	if (! track_db.current_track_is_valid()) return true;
	if (!video_src.is_initialized()) return true;

	assert( track_db.track_map.find( track_db.current_track_id ) != track_db.track_map.end());
	image_track& t = track_db.track_map[ track_db.current_track_id ];
	unsigned fs = video_src.timestamp().frame_number();
	if (t.locs[ t.locs.size()-1 ].frame_num != fs) {
	  vcl_cerr << "Can only delete from end of track (for now) \n";
	  return true;
	}
	t.locs.pop_back();
	if (t.locs.empty()) {
	  track_db.track_map.erase( track_db.current_track_id );
	  track_db.current_track_id = -1;
	  // leave the max ID alone
	}
#endif
      }



      // up-arrow -> select next track
      if (e.key == vgui_CURSOR_UP) {
	vcl_cerr << "up-arrow needs upgrading\n";
	handled = true;
#if 0
	if (add_point_mode && (! track_db.track_map.empty())) {
	  do {
	    track_db.current_track_id++;
	    if (track_db.current_track_id > track_db.max_track_id) track_db.current_track_id = 0;
	  } while ( track_db.track_map.find( track_db.current_track_id ) == track_db.track_map.end() );
	}
#endif
      }

      // down-arrow -> select previous track
      if (e.key == vgui_CURSOR_DOWN) {
	vcl_cerr << "down-arrow needs upgrading\n";
	handled = true;
#if 0
	if (add_point_mode && (! track_db.track_map.empty())) {
	  do {
	    if (track_db.current_track_id == 0) {
	      track_db.current_track_id = track_db.max_track_id;
	    } else {
	      track_db.current_track_id--;
	    }
	  } while ( track_db.track_map.find( track_db.current_track_id ) == track_db.track_map.end()) ;
	}
#endif
      }

      // left-arrow -> back up a frame
      if (e.key == vgui_CURSOR_LEFT) {
	handled = true;
	if (video_src.is_initialized()) {
	  vidtk::timestamp ts = video_src.timestamp();
	  if (ts.has_frame_number()) {
	    unsigned f = ts.frame_number();
	    if (f > 0) {
	      bool b = video_src.seek( f-1 );
	      if (!b) {
		vcl_cerr << "Backstep failed\n";
	      } else {
		b = video_src.step();
		this->img_tab->set_image_view( video_src.image() );
		this->img_tab->reread_image();
	      }
	    } // frame > 0
	  } // have a frame number
	} // initialized
      }

      // right-arrow -> next frame
      if (e.key == vgui_CURSOR_RIGHT) {
	handled = true;
	if (video_src.is_initialized()) {
	  bool b = video_src.step();
	  this->img_tab->set_image_view( video_src.image() );
	  this->img_tab->reread_image();
	  if (!b) {
	    // suffer an awful fate
	  }
	}
      }

      if (handled) post_redraw();
      return handled;
    }

    // #########################################
    if (add_point_mode) {
      vcl_cerr << "ground truth mode needs upgrading\n";
#if 0
      // shift-left-click: start a new track
      if ((e.type == vgui_BUTTON_DOWN) && (e.button == vgui_LEFT) && (e.modifier == vgui_SHIFT)) {
	if (! video_src.is_initialized()) return true;
	// where's the click in image space?
	vgui_projection_inspector pi;
	float ix, iy;
	pi.window_to_image_coordinates( static_cast<int>( e.wx ), static_cast<int>( e.wy ), ix, iy );
	vcl_string label;
	vgui_dialog d("What label");
	d.field( "Label: ", label );
	if (!d.ask()) return true;

	unsigned track_id = track_db.max_track_id + 1;
	assert( track_db.track_map.find( track_id ) == track_db.track_map.end());

	image_track new_track( label, track_id );
	new_track.locs.push_back( image_track_pos( ix, iy, 5, 7, video_src.timestamp().frame_number() ));

	track_db.track_map[ track_id ] = new_track;
	track_db.current_track_id = track_id;
	track_db.max_track_id++;

	post_redraw();
	return true;
      }

      // left-click: insert a point (if none in current track) or select a point (if one in current track)
      if ((e.type == vgui_BUTTON_DOWN) && (e.button == vgui_LEFT) && (e.modifier == 0)) {
	if (! video_src.is_initialized()) return true;
	// where's the click in image space?
	vgui_projection_inspector pi;
	float ix, iy;
	pi.window_to_image_coordinates( static_cast<int>( e.wx ), static_cast<int>( e.wy ), ix, iy );


	// do we have a point already?
	unsigned f = video_src.timestamp().frame_number();
	if (track_db.point_defined_at_frame( f )) {
	  // yes -- select it
	  selected_pt = track_db_set.pick_point( active_set, f, ix, iy, true );
	  if (sel_pt.valid) {
	    track_db.current_track_id = sel_pt.track_id;
	    post_redraw();
	  }
	} else {
	  if (track_db.current_track_is_valid()) {
	    image_track& t = track_db.track_map[ track_db.current_track_id ];
	    assert( !t.locs.empty() );
	    const image_track_pos last_pos = t.locs[ t.locs.size()-1 ];
	    unsigned last_f = last_pos.frame_num;
	    // if there's a time gap, ask if we should fill in the gap
	    if (f - last_f < 0) {
	      vgui_dialog d("No time travel");
	      vcl_string msg =
		vul_sprintf("Last track frame is %d;\nthis frame is %d\nCan only add to end of track",
			    last_f, f );
	      d.message( msg.c_str() );
	      d.ask();
	      return true;
	    }

	    if (f - last_f > 1) {
	      vgui_dialog d("Fill in gaps?");
	      vcl_string msg =
		vul_sprintf("Gap from last frame %d\nto this frame %d\nFill gap?", last_f, f);
	      d.message( msg.c_str() );
	      if (!d.ask()) return true;

	      for (unsigned fill_f = last_f+1; fill_f < f; fill_f++) {
		vcl_cerr << "Filling in frame " << fill_f << "\n";
		t.locs.push_back( image_track_pos( ix, iy, last_pos.bb_width, last_pos.bb_height, fill_f ));
	      }
	    }

	    t.locs.push_back( image_track_pos( ix, iy, last_pos.bb_width, last_pos.bb_height, f ));
	    post_redraw();
	  }
	}
	return true;
      }

      // motion and selected point: move the selected point to the mouse position (or resize box)
      if ((e.type == vgui_MOUSE_MOTION) && (selected_pt.valid)) {
	vgui_projection_inspector pi;
	float ix, iy;
	image_track_db_set::trackset_it ts = track_db_set.tracks.find( selected_pt.trackset_tag );
	assert( ts != track_db_set.end() );
	image_track_db_state& track_db = ts->second;
	assert( track_db.track_map.find( selected_pt.track_id ) != track_db.track_map.end() );
	image_track_pos& p = track_db.track_map[ selected_pt.track_id ].locs[ selected_pt.frame_index_in_track ];
	pi.window_to_image_coordinates( static_cast<int>( e.wx ), static_cast<int>( e.wy ), ix, iy );
	if (selected_pt.bb_selected) {
	  double dx = fabs( ix - p.ctr_x );
	  double dy = fabs( iy - p.ctr_y );
	  p.bb_width = 2*dx;
	  p.bb_height = 2*dy;
	} else {
	  p.ctr_x = ix;
	  p.ctr_y = iy;
	}
	post_redraw();
	return true;
      }

      // mouse up? selection invalid
      if (e.type == vgui_MOUSE_UP) {
	selected_pt.valid = false;
      return true;
      }

#endif
    } else {

    // #########################################
      // left-click: find the clicked point, move to that frame
      if ((e.type == vgui_BUTTON_DOWN) && (e.button == vgui_LEFT) && (e.modifier == 0)) {
	if (! video_src.is_initialized()) return true;
	// where's the click in image space?
	vgui_projection_inspector pi;
	float ix, iy;
	pi.window_to_image_coordinates( static_cast<int>( e.wx ), static_cast<int>( e.wy ), ix, iy );

	// do we have a point already?
	unsigned f = video_src.timestamp().frame_number();
	selected_pt = track_db_set.pick_point( f, ix, iy, false );
	if (selected_pt.valid) {
	  video_src.seek( selected_pt.frame_num );
	  video_src.step();
	  this->img_tab->set_image_view( video_src.image() );
	  this->img_tab->reread_image();
	  post_redraw();
	}
	return true;
      }

    // #########################################
    }




    return vgui_easy2D_tableau::handle( e );
  }
};


typedef vgui_tableau_sptr_t<image_track_tableau> image_track_tableau_sptr;

struct image_track_tableau_new: public image_track_tableau_sptr
{
  image_track_tableau_new( const vgui_image_tableau_sptr& p, image_track_db_set& t, video_src_type& v ) :
    image_track_tableau_sptr( new image_track_tableau( t, v, p )) {}
};




vidtk::generic_frame_process< vxl_byte > frame_src( "src" );

image_track_tableau_new* track_tab_ptr;
vgui_image_tableau_new* img_tab_ptr;
image_track_db_set track_db_set;

void goto_frame_cb()
{
  if (!img_tab_ptr) return;
  vgui_dialog d("goto frame");
  unsigned f = 0;
  d.field("frame: ", f );
  if (!d.ask()) return;
  frame_src.seek(f);
  frame_src.step();
  (*img_tab_ptr)->set_image_view( frame_src.image() );
  (*img_tab_ptr)->reread_image();
  (*img_tab_ptr)->post_redraw();
  unsigned frame_num = frame_src.timestamp().frame_number();
  vcl_cerr << "Frame num now " << frame_num << "\n";
}

void set_frame_cb()
{
  if (!img_tab_ptr) return;
  bool b = frame_src.step( );
  vcl_cerr << "step: " << b << "\n";
  (*img_tab_ptr)->set_image_view( frame_src.image() );
  (*img_tab_ptr)->reread_image();
  (*img_tab_ptr)->post_redraw();
  unsigned frame_num = frame_src.timestamp().frame_number();
  vcl_cerr << "Frame num now " << frame_num << "\n";
}

void export_tracks_cb()
{
  vcl_cerr << "export needs upgrading\n";
#if 0
  // move this into a file!
  const double projdat[] = {
    -1.61002613885668e+01,  -1.66225582120736e+01,   9.87559884503315e+03,
    4.12892982369514e-01,   3.65257005585947e+01,  -1.53581820088824e+04,
    2.88641808357382e-02,  -8.83171467983169e-01,  -9.25930557364776e+01 };


  vnl_double_3x3 proj( projdat );
  proj = proj.transpose();
  /*
  vnl_matrix_fixed<double, 1, 3> pt;
  pt.put(0,0, 561.0);
  pt.put(0,1, 203.0);
  pt.put(0,2, 1.0);
  vnl_matrix_fixed<double, 1, 3> pt2 = pt*proj;
  for (unsigned i=0; i<3; i++) {
    vcl_cerr << i << ": " << pt2.get(0,i) / pt2.get(0,2) << "\n";
  }
  // should be [9.89904, 30.1616, 1]
  */
  vgui_dialog d( "export tracks" );
  vcl_pair<unsigned, unsigned> frame_minmax = track_db.frame_range();
  vcl_string label;
  vcl_string msg = vul_sprintf( "Frames: %06d-%06d", frame_minmax.first, frame_minmax.second );
  vcl_string native_temp = "%s-%06d-gt.trk";
  vcl_string matlab_temp = "%s-%06d-gt-mat.trk";
  vcl_string frames_temp = "%s-%06d.avi";
  d.message( msg.c_str() );
  d.field( "Label: ", label );
  d.field( "Native: ", native_temp );
  d.field( "Matlab: ", matlab_temp );
  d.field( "Frames: ", frames_temp );

  if (!d.ask()) return;

  vcl_string native_fn = vul_sprintf( native_temp.c_str(), label.c_str(), frame_minmax.first );
  vcl_string matlab_fn = vul_sprintf( matlab_temp.c_str(), label.c_str(), frame_minmax.first );
  vcl_string frames_fn = vul_sprintf( frames_temp.c_str(), label.c_str(), frame_minmax.first );

  track_db.save_native( native_fn );
  track_db.save_matlab( matlab_fn, proj, frame_minmax.first );
#endif

  // writer not working at the moment
#if 0
  vidtk::vidl_ffmpeg_writer_process writer( "writer" );
  vidtk::config_block cfg = writer.params();
  cfg.set( "filename", frames_fn );
  writer.set_params( cfg );
  unsigned current_frame = frame_src.timestamp().frame_number();
  frame_src.seek( frame_minmax.first );
  for (unsigned f = frame_minmax.first; f<=frame_minmax.second; f++) {
    frame_src.step();
    writer.set_frame( frame_src.image() );
    writer.step();
  }
  frame_src.seek( current_frame );
#endif
}

void load_video_tracks_cb()
{
  vgui_dialog d( "Load video & tracks" );
  vcl_string v_regexp("*.avi"), t_regexp("*.txt"), v_path, t_path, tag;
  d.file( "video", v_regexp, v_path );
  d.file( "tracks", t_regexp, t_path );
  d.field( "tag", tag );
  if (!d.ask()) return;
  if (tag == "") {
    vcl_cerr << "Must supply a dataset tag!\n";
    return;
  }
  if (track_db_set.tracks.find( tag ) != track_db_set.tracks.end()) {
    vcl_cerr << "Tag '" << tag << "' already used; please choose another\n";
    return;
  }

  vidtk::config_block params = frame_src.params();
  params.set( "type", "vidl_ffmpeg" );
  params.set( "vidl_ffmpeg:filename", v_path );

  frame_src.set_params( params );
  bool b = frame_src.initialize();
  vcl_cerr << "Initialize of " << v_path << ": " << b << "\n";

  image_track_db_state tdb;
  tdb.load_matlab( t_path );
  track_db_set.tracks[ tag ] = tdb;
  (*track_tab_ptr)->ds_library.ds_map[ tag ] = tdb.display_state;

  set_frame_cb();
}

void goto_track_cb()
{
  vcl_vector<vcl_string> labels = track_db_set.labels();
  if (labels.empty()) {
    vcl_cerr << "No datasets loaded\n";
    return;
  }

  vgui_dialog d( "goto track" );
  unsigned id = 0, ds_id = 0;
  d.choice("Dataset", labels, ds_id );
  d.field("Track ID: ", id );
  if (!d.ask()) return;

  vcl_string tag = labels[ds_id];

  image_track_db_state& tdb = track_db_set.tracks[tag];
  image_track_db_state::track_map_cit i = tdb.track_map.find( id );
  if (i == tdb.track_map.end()) {
    vcl_cerr << "Couldn't find track " << id << "\n";
    return;
  }

  (*track_tab_ptr)->selected_pt = GUI_selection(tag, id, i->second.locs[0].frame_num.frame_number(), 0);
  tdb.current_track_id = id;
  frame_src.seek( i->second.locs[0].frame_num.frame_number() );
  set_frame_cb();
}


void load_video_cb()
{
  vgui_dialog d( "choose video" );
  vcl_string regexp("*.avi"), filepath;
  d.file( "video", regexp, filepath );
  if (!d.ask()) return;

  vidtk::config_block params = frame_src.params();
  params.set( "type", "vidl_ffmpeg" );
  params.set( "vidl_ffmpeg:filename", filepath );

  frame_src.set_params( params );
  bool b = frame_src.initialize();
  vcl_cerr << "Initialize of " << filepath << ": " << b << "\n";

  set_frame_cb();
}

void load_tracks_cb()
{
  vgui_dialog d( "load tracks" );
  vcl_string regexp("*.txt"), filepath, tag;
  d.file( "computed tracks", regexp, filepath );
  d.field( "tag", tag );
  if (!d.ask()) return;
  if (tag == "") {
    vcl_cerr << "Must supply a dataset tag!\n";
    return;
  }
  if (track_db_set.tracks.find( tag ) != track_db_set.tracks.end()) {
    vcl_cerr << "Tag '" << tag << "' already used; please choose another\n";
    return;
  }

  image_track_db_state tdb;
  tdb.load_matlab( filepath );
  track_db_set.tracks[ tag ] = tdb;
  (*track_tab_ptr)->ds_library.ds_map[ tag ] = tdb.display_state;

  set_frame_cb();
}

bool gui_display_state_dialog( track_display_state& ds, bool show_remove_option )
{
  vgui_dialog d("Set display state");
  vcl_string *rgb_strs = new vcl_string[ track_display_state::RGB_top ];
  for (unsigned i=0; i<track_display_state::RGB_top; i++) {
    rgb_strs[i] = ds.RGB_sets[i].to_str();
    d.field( RGB_names[i], rgb_strs[i] );
  }

  vcl_vector<vcl_string> future_strs;
  for (unsigned i=0; i<track_display_state::future_top; i++) {
    future_strs.push_back( future_names[i] );
  }
  d.choice("Show future tracks as:", future_strs, ds.show_future_flag );
  d.checkbox("Only show if active:", ds.only_show_while_active );
  d.field("Time window: ", ds.time_window);
  d.checkbox("Show bounding box:", ds.show_bounding_box);
  d.checkbox("Hide: ", ds.hide);
  d.checkbox("Always show: ",ds.always_show);
  bool remove_flag = false;
  if (show_remove_option) {
    d.checkbox("Remove", remove_flag);
  }

  if (!d.ask()) {
    delete [] rgb_strs;
    return false;
  }

  for (unsigned i=0; i<track_display_state::RGB_top; i++) {
    ds.RGB_sets[i] = GUI_RGB( rgb_strs[i] );
  }
  delete [] rgb_strs;
  return true;
}

void gui_change_display_state_cb()
{
  const GUI_selection& s =  (*track_tab_ptr)->selected_pt;
  if ( ! s.valid) {
    vcl_cerr << "No track selected\n";
    return;
  }
  vgui_dialog d( "Choose display state" );
  d.message("Choose a display state for");
  d.message( s.to_str().c_str());
  vcl_vector<vcl_string> choices;
  display_state_library& dsl = (*track_tab_ptr)->ds_library;
  for (display_state_library::ds_map_cit i = dsl.ds_map.begin(); i != dsl.ds_map.end(); ++i)
  {
    choices.push_back( i->first );
  }
  choices.push_back("*new*");
  unsigned index = 0;
  d.choice("DS list: ", choices, index );
  if (!d.ask()) return;

  if (choices[index] == "*new*") {
    track_display_state ds;
    if (!gui_display_state_dialog( ds, false )) return;
    dsl.add_display_state( s, ds );
  } else {
    display_state_library::ds_map_cit i = dsl.ds_map.find( choices[index] );
    if (i == dsl.ds_map.end()) {
      vcl_cerr << "Whoops!  Couldn't find " << choices[index] << "?\n";
      return;
    }
    dsl.link_display_state( s, choices[index] );
    //    dsl.add_display_state( s, i->second );
  }
}




void gui_prefs_cb()
{
  display_state_library& dsl = (*track_tab_ptr)->ds_library;
  vcl_vector<vcl_string> labels = dsl.labels();
  if (labels.empty()) {
    vcl_cerr << "No datasets loaded\n";
    return;
  }
  vgui_dialog d( "Choose display state" );
  unsigned ds_id = 0;
  d.choice("Display state", labels, ds_id );
  if (!d.ask()) return;

  track_display_state& ds = dsl.ds_map[labels[ds_id]];

  gui_display_state_dialog( ds, false );

  // invalidate active sets; copy to default display state if tag matches
  for (image_track_db_set::trackset_it i=track_db_set.tracks.begin(); i != track_db_set.tracks.end(); ++i)
  {
    if (i->first == labels[ds_id]) {
      i->second.display_state = ds;
    }
    i->second.active_set_cf = -1;
  }

  set_frame_cb();
}

void suppress_noise_cb()
{
  vgui_dialog d( "Suppress noise" );
  static unsigned thresh = 4;
  d.field( "Max starts per frame: ", thresh );
  if (!d.ask()) return;
  for (image_track_db_set::trackset_it i=track_db_set.tracks.begin(); i != track_db_set.tracks.end(); ++i)
  {
    vcl_pair<unsigned, unsigned> r = i->second.suppress( thresh );
    double ratio = 1.0*r.first/r.second;
    vcl_cerr << i->first << ": " << r.first << " of " << r.second << " : " << ratio << "\n";
  }
}

void verbose_draw_cb()
{
  (*track_tab_ptr)->verbose_draw = true;
}

void reverse_draw_cb()
{
  (*track_tab_ptr)->reverse_draw_exceptions = !(*track_tab_ptr)->reverse_draw_exceptions;
}

int main( int argc, char *argv[] )
{
  vgui::init( argc, argv );

  vgui_image_tableau_new img_tab;
  image_track_tableau_new track_tab( img_tab, track_db_set, frame_src );
  track_tab_ptr = &track_tab;
  img_tab_ptr = &img_tab;
  vgui_viewer2D_tableau_new viewer_tab( track_tab );
  vgui_shell_tableau_new shell_tab( viewer_tab );

  vgui_menu main_menu, file_menu, GUI_menu;
  file_menu.add( "Load video & tracks", &load_video_tracks_cb, vgui_key('l') );
  file_menu.add( "Load video", &load_video_cb, vgui_key('v') );
  file_menu.add( "Goto frame", &goto_frame_cb, vgui_key('g') );
  file_menu.add( "Goto track", &goto_track_cb, vgui_key('t') );
  file_menu.add( "Export tracks", &export_tracks_cb, vgui_key('e') );
  file_menu.add( "Load tracks", &load_tracks_cb );
  GUI_menu.add( "GUI prefs", &gui_prefs_cb );
  GUI_menu.add( "Track GUI prefs", &gui_change_display_state_cb );
  GUI_menu.add( "Suppress noise", &suppress_noise_cb );
  GUI_menu.add( "Verbose draw", &verbose_draw_cb );
  GUI_menu.add( "Toggle reverse draw", &reverse_draw_cb );
  main_menu.add( "File", file_menu );
  main_menu.add( "GUI", GUI_menu );

  unsigned w=512, h=512;
  vcl_string title="GT Tracker";
  vgui_window* win = vgui::produce_window( w, h, main_menu, title );
  win->get_adaptor()->set_tableau( shell_tab );
  win->set_statusbar( true );
  win->show();

  track_tab->status_bar = win->get_statusbar();

  return vgui::run();

  //  vgui::run( shell_tab, 512, 512, main_menu );
}

