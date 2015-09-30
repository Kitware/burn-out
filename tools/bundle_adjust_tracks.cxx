/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_arg.h>
#include <vcl_string.h>
#include <vcl_map.h>
#include <vcl_fstream.h>
#include <vgl/vgl_plane_3d.h>
#include <vgl/algo/vgl_fit_plane_3d.h>


#include <tracking/track.h>
#include <tracking/bundle_adjust_tracks.h>


/// load tracks from a file
vcl_vector< vidtk::track_sptr >
load_tracks(const vcl_string& filename)
{
  vcl_vector< vidtk::track_sptr > tracks;
  vcl_ifstream ifs(filename.c_str());
  if (!ifs)
  {
    return tracks;
  }

  unsigned last_id=0;

  vidtk::track_sptr track = NULL;
  unsigned track_id, count, frame_num, is_good;
  while (ifs >> track_id)
  {
    // make a new track if needed
    if (!track || track_id != last_id)
    {
      track = new vidtk::track;
      track->set_id(track_id);
      tracks.push_back(track);
    }
    last_id = track_id;

    vidtk::track_state_sptr state = new vidtk::track_state;

    ifs >> count >> frame_num;
    state->time_.set_frame_number(frame_num);
    ifs >> state->loc_(0) >> state->loc_(1);
    ifs >> state->vel_(0) >> state->vel_(1);
    ifs >> is_good; // ignore this value
    track->add_state(state);
  }

  return tracks;
}


/// Transform the points into a special gauge (coordinate frame)
/// the centroid of the points maps to the orgin
/// the normal of a fit plane maps to the Z-axis
void enforce_gauge(vidtk::bundle_adjust_tracks::Cam_map& camera_map,
                   vidtk::bundle_adjust_tracks::Point_map& point_map)
{
  typedef vidtk::bundle_adjust_tracks::Cam_map Cam_map;
  typedef vidtk::bundle_adjust_tracks::Point_map Point_map;

  vgl_fit_plane_3d<double> fit_plane;
  vcl_vector<vgl_point_3d<double> > points;
  for ( Point_map::const_iterator i=point_map.begin(); i!=point_map.end(); ++i)
  {
    points.push_back(i->second);
    fit_plane.add_point(i->second.x(), i->second.y(), i->second.z());
  }
  if (!fit_plane.fit(10.0))
  {
    vcl_cerr << "Unable to enforce gauge"<<vcl_endl;
    return;
  }
  vgl_plane_3d<double> ground = fit_plane.get_plane();
  vgl_point_3d<double> centroid = centre(points);
  vgl_vector_3d<double> z_axis(0,0,1);
  vgl_vector_3d<double> ground_normal = ground.normal();
  if (dot_product(z_axis,ground_normal) < 0.0)
  {
    ground_normal *= -1.0;
  }

  // rotation that takes ground_normal to z_axis
  vgl_rotation_3d<double> R(ground_normal, z_axis);

  // translation that moves centroid to origin
  vgl_vector_3d<double> t = vgl_point_3d<double>(0,0,0) - R*centroid;

  // scale not modified for now
  double s = 1.0;

  for ( Point_map::iterator i=point_map.begin(); i!=point_map.end(); ++i)
  {
    vgl_point_3d<double>& p = i->second;
    p = R*p;
    p.set(s*p.x(), s*p.y(), s*p.z());
    p += t;
  }

  for ( Cam_map::iterator i=camera_map.begin(); i!=camera_map.end(); ++i)
  {
    vpgl_perspective_camera<double>& P = i->second;
    P.set_rotation(P.get_rotation()*R.inverse());
    vgl_point_3d<double> c = R*P.get_camera_center();
    c.set(s*c.x(), s*c.y(), s*c.z());
    P.set_camera_center(c + t);
  }
  
}




int main(int argc, char *argv[])
{
  vul_arg<vcl_string> input_tracks( "--input-tracks", "input file containing KLT point tracks", "" );
  vul_arg<vcl_string> input_cameras( "--input-cameras", "input file containing initial cameras", "" );
  vul_arg<vcl_string> input_points( "--input-points", "input file containing initial points", "" );
  vul_arg<vcl_string> output_cameras( "--output-cameras", "output camera matrices to a file", "" );
  vul_arg<vcl_string> output_points( "--output-points", "output 3-d points to a file", "" );
  vul_arg<vcl_string> output_vrml( "--output-vrml", "output file to write vrml", "" );
  vul_arg<vcl_string> in_cam_centers( "--cam-centers", "input camera centers", "" );
  vul_arg<unsigned>   num_frames( "--num-frames", "number of frames for frame decimation", 20);
  vul_arg<unsigned> num_tracks( "--num-tracks", "number of tracks for track decimation", 1000);
  vul_arg<double> focal_length( "-f", "initial guess of focal length", 100.0 );
  vul_arg<double> pp_x( "-pp-x", "principal point horizontal coordinate", 640.0 );
  vul_arg<double> pp_y( "-pp-y", "principal point vertical coordinate", 480.0 );
  vul_arg<bool>  est_f( "--est-f", "enable estimation of focal length", false);
  vul_arg<bool>  no_fill_cam( "--no-fill-cam", "skip interpolation to fill in missing cameras", false);
  vul_arg<bool>  no_fill_pts( "--no-fill-pts", "skip triangulation to fill in missing points", false);
  vul_arg<bool>  no_optimize( "--no-optimize", "skip the full optimization step", false);
  vul_arg<bool>  camera_only( "--camera-only", "only optimize cameras, points are fixed", false);
  vul_arg<unsigned> init_cam_step( "--init-cam-step", "initial camera step to speed optimization", 100);

  vul_arg_parse( argc, argv );

  typedef vidtk::bundle_adjust_tracks::Cam_map Cam_map;
  typedef vidtk::bundle_adjust_tracks::Point_map Point_map;

  if (!input_tracks.set())
  {
    vcl_cout << "An input track file is required.  "
             << "Run with -? for details."<<vcl_endl;
    return -1;
  }

  vcl_vector< vidtk::track_sptr > tracks = load_tracks(input_tracks());
  vcl_cout << "loaded "<<tracks.size() << " tracks"<<vcl_endl;

  vidtk::bundle_adjust_tracks bat(tracks);
  bat.set_estimate_focal_length(est_f());
  bat.set_verbose(true);

  if (input_cameras.set())
  {
    vcl_cout << "Reading cameras from "<<input_cameras() << vcl_flush;
    Cam_map camera_map;
    vcl_ifstream ifs(input_cameras().c_str());
    unsigned int frame = 0;
    vpgl_perspective_camera<double> camera;
    while (ifs >> frame)
    {
      ifs >> camera;
      camera_map[frame] = camera;
    }
    ifs.close();
    bat.set_cameras(camera_map);
    vcl_cout << " --> loaded "<<camera_map.size()<<vcl_endl;
    vcl_cout << camera_map[0].get_matrix() << vcl_endl;
  }

  if (input_points.set())
  {
    vcl_cout << "Reading points from "<<input_points() << vcl_flush;
    Point_map point_map;
    vcl_ifstream ifs(input_points().c_str());
    unsigned frame = 0;
    vgl_point_3d<double> point;
    while (ifs >> frame)
    {
      ifs >> point;
      point_map[frame] = point;
    }
    ifs.close();
    bat.set_points(point_map);
    vcl_cout << " --> loaded "<<point_map.size()<<vcl_endl;
  }

  vpgl_calibration_matrix<double> K(focal_length(),
                                    vgl_homg_point_2d<double>(pp_x(),pp_y()));

  bat.set_init_calibration(K);

  double max, median, mean;

  // bootstrap the optimization if no inputs are provided
  if (!input_cameras.set() && !input_points.set())
  {
    bat.bootstrap_optimization(num_frames(), num_tracks());

    bat.reproject_error_stats(max, median, mean);
    vcl_cout << "Reprojection errors: (max="<<max<<", median="<<median
             <<", mean="<<mean<<")"<<vcl_endl;
  }


  if (!no_fill_cam())
  {
    vcl_cout << "Interpolating missing cameras"<<vcl_endl;
    bat.interpolate_missing_cameras();
  }

  if (!no_fill_pts())
  {
    vcl_cout << "Triangulating missing points"<<vcl_endl;
    bat.triangulate_missing_points();
  }

  bat.reproject_error_stats(max, median, mean);
  vcl_cout << "Reprojection errors: (max="<<max<<", median="<<median
           <<", mean="<<mean<<")"<<vcl_endl;

  if (!no_optimize())
  {
    if (camera_only())
    {
      bat.optimize_cameras();
    }
    else
    {
      unsigned cam_step = init_cam_step();
      while (cam_step > 1)
      {
        vcl_cout << "Sampling every "<<cam_step<<" cameras, ";
        bat.decimate_cameras(cam_step);
        vcl_cout << "leaving "<<bat.cameras().size()<<" cameras"<<vcl_endl;
        bat.optimize();
        bat.interpolate_missing_cameras();
        cam_step /= 2;
      }
      vcl_cout << "Optimizing over all cameras and points"<< vcl_endl;
      bat.optimize();
    }
  }

  bat.reproject_error_stats(max, median, mean);
  vcl_cout << "Reprojection errors: (max="<<max<<", median="<<median
           <<", mean="<<mean<<")"<<vcl_endl;

  vcl_cout << "Removing outliers" << vcl_endl;
  bat.remove_outliers(0.25, 40, 0.0);

  bat.reproject_error_stats(max, median, mean);
  vcl_cout << "Reprojection errors: (max="<<max<<", median="<<median
           <<", mean="<<mean<<")"<<vcl_endl;

  
  Cam_map camera_map = bat.cameras();
  Point_map point_map = bat.points();

  // map the points and cameras into a specify gauge (reference frame)
  enforce_gauge(camera_map, point_map);
  
  vcl_vector<vpgl_perspective_camera<double> > cameras;
  for ( Cam_map::const_iterator i=camera_map.begin(); i!=camera_map.end(); ++i)
  {
    cameras.push_back(i->second);
  }

  vcl_vector<vgl_point_3d<double> > points;
  for ( Point_map::const_iterator i=point_map.begin(); i!=point_map.end(); ++i)
  {
    points.push_back(i->second);
  }

  vpgl_bundle_adjust::write_vrml(output_vrml(),cameras,points);

  if (output_cameras.set())
  {
    vcl_ofstream ofs(output_cameras().c_str());
    for ( Cam_map::const_iterator i=camera_map.begin(); i!=camera_map.end(); ++i)
    {
      ofs << i->first <<'\n'<<i->second <<'\n';
    }
    ofs.close();
  }

  if (output_points.set())
  {
    vcl_ofstream ofs(output_points().c_str());
    for ( Point_map::const_iterator i=point_map.begin(); i!=point_map.end(); ++i)
    {
      ofs << i->first << ' '
          <<i->second.x() << ' ' <<i->second.y() << ' ' << i->second.z() <<'\n';
    }
    ofs.close();
  }

  return 0;
}
