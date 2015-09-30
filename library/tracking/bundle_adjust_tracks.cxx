/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "bundle_adjust_tracks.h"

#include <vcl_set.h>
#include <vcl_algorithm.h>

#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>
#include <vgl/vgl_distance.h>
#include <vgl/vgl_plane_3d.h>
#include <vpgl/algo/vpgl_optimize_camera.h>


using namespace vidtk;


bundle_adjust_tracks
::bundle_adjust_tracks(const vcl_vector< track_sptr >& tracks) :
  tracks_( tracks ),
  estimate_focal_length_(false),
  verbose_(true),
  init_K_(),
  allow_bootstrap_(false)
{
  // compute a very rough guess at a possible principle point and focal length
  // you should always set the correct values when known.
  vgl_box_2d<double> bounds;
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      vgl_point_2d<double> p(tracks_[i]->history()[j]->loc_[0],
                             tracks_[i]->history()[j]->loc_[1]);
      bounds.add(p);
    }
  }
  init_K_.set_focal_length(bounds.width());
  init_K_.set_principal_point(bounds.centroid());
}


/// Set the initial cameras to use during optimization.
/// The input is a map from frame numbers to perspective cameras.
void bundle_adjust_tracks
::set_cameras(const Cam_map& cameras)
{
  cameras_ = cameras;
}


/// Set the initial 3-d points to use during optimization.
/// The input is a map from track IDs to 3-d points
void bundle_adjust_tracks
::set_points(const Point_map& points)
{
  world_points_ = points;
}


/// Set the initial calibration matrix used when generating cameras
/// \note does not modify cameras set by set_cameras
void bundle_adjust_tracks
::set_init_calibration(const vpgl_calibration_matrix<double>& K)
{
  init_K_ = K;
}


/// Enable or disable estimation of a single fixed focal length used
/// throughout the entire sequence
void bundle_adjust_tracks
::set_estimate_focal_length(bool e)
{
  estimate_focal_length_ = e;
}


/// Enable or disable verbose status messages
void bundle_adjust_tracks
::set_verbose(bool v)
{
  verbose_ = v;
}


/// Run an initial optimization starting from nothing.
/// Sample a subset of frames and tracks and run the optimization starting
/// with default initialization.  Interpolate skipped cameras and tringulate
/// skipped points.  This quickly provides better initialization for a full
/// optimization of all the data.
bool bundle_adjust_tracks
::bootstrap_optimization(unsigned num_frames,
                         unsigned num_tracks)
{
  // clear existing cameras and points
  cameras_.clear();
  world_points_.clear();
  
  // compute subset of tracks and frames to use
  allow_bootstrap_ = true;
  track_indices(1,num_tracks);
  frame_indices(num_frames);
  allow_bootstrap_ = false;

  if (verbose_)
  {
    vcl_cout << "selected "<<track_ids_.size() << " tracks and "
             << frames_.size() << " frames for boot strapping"<<vcl_endl;
  }
  
  // collect image points from the subset and generate a point visibility mask
  vcl_vector<vcl_vector<bool> > mask;
  vcl_vector<vgl_point_2d<double> > image_points;
  extract_image_points(image_points, mask);


  // initialize the cameras
  vgl_rotation_3d<double> I; // no rotation
  vgl_homg_point_3d<double> center(0.0, 0.0, 1.0);
  vpgl_perspective_camera<double> init_cam(init_K_,center,I);
  init_cam.look_at(vgl_homg_point_3d<double>(0.0, 0.0, 0.0),
                   vgl_vector_3d<double>(0,-1,0));
  vcl_vector<vpgl_perspective_camera<double> > cameras(frames_.size(), init_cam);


  // initialize the 3-d points
  vcl_vector<vgl_point_3d<double> >
      world_points(track_ids_.size(), vgl_point_3d<double>(0.0, 0.0, 0.0));
  
  // back project the first point in each track onto the x-y plane
  unsigned k=0;
  vcl_vector<bool> track_done(track_ids_.size(),false);
  vgl_plane_3d<double> ground(0,0,1,0);
  for (unsigned fi = 0; fi < mask.size(); ++fi)
  {
    for (unsigned ti = 0; ti < mask[fi].size(); ++ti)
    {
      if (!mask[fi][ti])
        continue;
      const vgl_point_2d<double>& p = image_points[k++];
      if (track_done[ti])
        continue;
      track_done[ti] = true;
      world_points[ti] = vgl_intersection(cameras[fi].backproject(p),ground);
    }
  }

  ////////////////////////////////////////////////////////////////
  // bundle adjustment to optimize the cameras and 3-d points

  vpgl_bundle_adjust ba;
  ba.set_self_calibrate(estimate_focal_length_);
  ba.set_verbose(verbose_);
  ba.set_x_tolerence(1e-5);
  ba.set_g_tolerence(1e-5);
  
  // run a few iterations of non-robust optimization to get an initial guess
  ba.set_max_iterations(15);
  //ba.set_use_gradient(false);
  //ba.set_normalize_data(false);
  bool converge = ba.optimize(cameras, world_points, image_points, mask);
  if (!converge)
  {
    vcl_cerr << "optimization failed to converge"<<vcl_endl;
    return false;
  }
  vcl_vector<double> weights = ba.final_weights();

  // compute the necker reversal for an alternative initial guess to avoid
  // a very common necker reversed local minimum
  vcl_vector<vgl_point_3d<double> > world_points2(world_points);
  vcl_vector<vpgl_perspective_camera<double> > cameras2(cameras);
  ba.depth_reverse(cameras2, world_points2);

  ba.set_use_m_estimator(true);
  ba.set_x_tolerence(1e-5);
  ba.set_g_tolerence(1e-5);
  ba.set_max_iterations(50);
  converge = muliscale_ba(ba, cameras, world_points, weights, image_points, mask, 128, 4, 2);
  if (!converge)
  {
    vcl_cerr << "optimization failed to converge"<<vcl_endl;
    return false;
  }
  //double error1 = ba.end_error();

  // Optimize the necker reversal alternative for comparison
  ba.set_use_m_estimator(false);
  ba.set_x_tolerence(1e-5);
  ba.set_g_tolerence(1e-5);
  ba.set_max_iterations(15);
  converge = ba.optimize(cameras2, world_points2, image_points, mask);
  if (!converge)
  {
    vcl_cerr << "optimization failed to converge"<<vcl_endl;
    return false;
  }
  vcl_vector<double> weights2 = ba.final_weights();

  ba.set_use_m_estimator(true);
  ba.set_x_tolerence(1e-5);
  ba.set_g_tolerence(1e-5);
  ba.set_max_iterations(50);
  converge = muliscale_ba(ba, cameras2, world_points2, weights2, image_points, mask, 128, 4, 2);
  if (!converge)
  {
    vcl_cerr << "optimization failed to converge"<<vcl_endl;
    return false;
  }
  //double error2 = ba.end_error();

  ////////////////////////////////////////////////////////////////
  // find the reprojection errors of the two results using the same subset of
  // points (minimum of weights will find the intersection of inliers)
  vcl_vector<double> min_weights(weights.size());
  for (unsigned i = 0; i < min_weights.size(); ++i)
  {
    min_weights[i] = vcl_min(weights[i],weights2[i]);
  }

  bool old_verbose = verbose_;
  verbose_ = false;
  update_weights(min_weights, mask);
  update_cameras(cameras);
  update_points(world_points);
  remove_outliers();
  double max1, median1, mean1;
  reproject_error_stats(max1, median1, mean1);
  cameras_.clear();
  world_points_.clear();
  
  update_cameras(cameras2);
  update_points(world_points2);
  remove_outliers();
  double max2, median2, mean2;
  reproject_error_stats(max2, median2, mean2);
  cameras_.clear();
  world_points_.clear();
  verbose_ = old_verbose;
  

  if ( verbose_ )
  {
    vcl_cout << "final error 1: max = "<<max1<<", median = "<<median1<<", mean = "<<mean1<<vcl_endl;
    vcl_cout << "final error 2: max = "<<max2<<", median = "<<median2<<", mean = "<<mean2<<vcl_endl;
  }

  // choose the result with the lowest residiual error
  if(mean2 < mean1)
  {
    cameras.swap(cameras2);
    world_points.swap(world_points2);
    weights.swap(weights2);
  }

  // update the camera and points maps with the optimized results
  update_cameras(cameras);
  update_points(world_points);
  // update the tracks with the weights
  update_weights(weights, mask);

  return true;
}


/// Optimize all initialized cameras and all initialized points.
/// Starts from the provided initialization of cameras and points
bool bundle_adjust_tracks
::optimize()
{
  // compute subset of tracks and frames to use
  track_indices();
  frame_indices();

  if (verbose_)
  {
  vcl_cout << "Optimizing over "<<track_ids_.size() << " tracks and "
           << frames_.size() << " frames"<<vcl_endl;
  }

  // collect image points from the subset and generate a point visibility mask
  vcl_vector<vcl_vector<bool> > mask;
  vcl_vector<vgl_point_2d<double> > image_points;
  extract_image_points(image_points, mask);

  // initialize the cameras
  vcl_vector<vpgl_perspective_camera<double> > cameras;
  for (unsigned i=0; i<frames_.size(); ++i)
  {
    Cam_map::const_iterator c_itr = cameras_.lower_bound(frames_[i]);
    if (c_itr == cameras_.end())
    {
      --c_itr;
    }
    if(c_itr->first != frames_[i])
    {
      vcl_cerr << "Warning: camera not initialized for frame "<<frames_[i]
               << ", using camera from frame "<<c_itr->first<<vcl_endl;
    }
    cameras.push_back(c_itr->second);
  }

  // initialize the 3-d points
  vcl_vector<vgl_point_3d<double> > world_points;
  for (unsigned i=0; i<track_ids_.size(); ++i)
  {
    Point_map::const_iterator p_itr = world_points_.find(track_ids_[i]);
    if (p_itr == world_points_.end())
    {
      vcl_cerr << "Warning: 3-d point not initialized for track "<<track_ids_[i]
               << ", using origin"<<vcl_endl;
      world_points.push_back(vgl_point_3d<double>(0.0,0.0,0.0));
    }
    else
    {
      world_points.push_back(p_itr->second);
    }
  }

  ////////////////////////////////////////////////////////////////
  // bundle adjustment to optimize the cameras and 3-d points

  vpgl_bundle_adjust ba;
  ba.set_self_calibrate(estimate_focal_length_);
  ba.set_verbose(verbose_);
  ba.set_x_tolerence(1e-5);
  ba.set_g_tolerence(1e-5);
  ba.set_use_m_estimator(true);
  ba.set_m_estimator_scale(16.0);

  ba.set_max_iterations(25);
  bool converge = ba.optimize(cameras, world_points, image_points, mask);
  if (!converge)
  {
    vcl_cerr << "optimization failed to converge"<<vcl_endl;
    return false;
  }

  if (verbose_)
  {
    const vcl_vector<double>& w = ba.final_weights();
    vcl_cout << "fraction outliers "<<outlier_count(w)<<"/"<<w.size()<<vcl_endl;
  }

  // update the camera and points maps with the optimized results
  update_cameras(cameras);
  update_points(world_points);
  // update the tracks with the weights
  update_weights(ba.final_weights(), mask);

  return true;
}


/// Optimize all initialized cameras keeping points fixed.
/// Starts from the provided initialization of cameras and points
bool bundle_adjust_tracks
::optimize_cameras()
{
  // compute subset of tracks and frames to use
  track_indices();
  frame_indices();

  if (verbose_)
  {
  vcl_cout << "Optimizing cameras for "<< frames_.size() << " frames"
           << "using "<<track_ids_.size() << " tracks"<<vcl_endl;
  }

  // collect image points from the subset and generate a point visibility mask
  vcl_vector<vcl_vector<bool> > mask;
  vcl_vector<vgl_point_2d<double> > image_points;
  extract_image_points(image_points, mask);


  // initialize the cameras
  vcl_vector<vpgl_perspective_camera<double> > cameras;
  for (unsigned i=0; i<frames_.size(); ++i)
  {
    Cam_map::const_iterator c_itr = cameras_.lower_bound(frames_[i]);
    if (c_itr == cameras_.end())
    {
      --c_itr;
    }
    if(c_itr->first != frames_[i])
    {
      vcl_cerr << "Warning: camera not initialized for frame "<<frames_[i]
               << ", using camera from frame "<<c_itr->first<<vcl_endl;
    }

    cameras.push_back(c_itr->second);
  }

  // initialize the 3-d points
  vcl_vector<vgl_point_3d<double> > world_points;
  for (unsigned i=0; i<track_ids_.size(); ++i)
  {
    Point_map::const_iterator p_itr = world_points_.find(track_ids_[i]);
    if (p_itr == world_points_.end())
    {
      vcl_cerr << "Warning: 3-d point not initialized for track "<<track_ids_[i]
               << ", using origin"<<vcl_endl;
      world_points.push_back(vgl_point_3d<double>(0.0,0.0,0.0));
    }
    else
    {
      world_points.push_back(p_itr->second);
    }
  }

  vnl_crs_index crs(mask);
  for (unsigned i=0; i<cameras.size(); ++i)
  {
    // world points seen by this camera
    vcl_vector<vgl_homg_point_3d<double> > points3d;
    vcl_vector<vgl_point_2d<double> > points2d;
    for (unsigned j=0; j<mask[i].size(); ++j)
    {
      if (mask[i][j])
      {
        points3d.push_back(vgl_homg_point_3d<double>(world_points[j]));
        points2d.push_back(image_points[crs(i,j)]);
      }
    }

    if (verbose_)
    {
      vcl_cout << "optimizing camera "<<i<<vcl_endl;
    }
    cameras[i] = vpgl_optimize_camera::opt_orient_pos(cameras[i],
                                                      points3d,
                                                      points2d);
  }

  update_cameras(cameras);
  return true;
}
  

/// Compute the reprojections errors of all estimated 3-d points into all
/// estimated cameras and returns error metrics: maximum, median, and mean
void bundle_adjust_tracks
::reproject_error_stats(double& max, double& median, double& mean)
{
  vcl_vector<double> errors;
  mean = 0.0;
  max = 0.0;
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    // skip tracks that are not reconstructed
    Point_map::const_iterator p_itr = world_points_.find(tracks_[i]->id());
    if (p_itr == world_points_.end())
    {
      continue;
    }

    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      //skip cameras that are not reconstructed
      unsigned frame_num = tracks_[i]->history()[j]->time_.frame_number();
      Cam_map::const_iterator c_itr = cameras_.find(frame_num);
      if (c_itr == cameras_.end())
        continue;

      vgl_point_2d<double> p0(tracks_[i]->history()[j]->loc_[0],
                              tracks_[i]->history()[j]->loc_[1]);
      vgl_point_2d<double> p1 = c_itr->second.project(p_itr->second);
      double dist = vgl_distance(p0,p1);
      errors.push_back(dist);
      mean += dist;
      if (dist > max)
      {
        max = dist;
      }
    }
  }
  mean /= errors.size();
  vcl_vector<double>::iterator med_pos = errors.begin()+errors.size()/2;
  vcl_nth_element(errors.begin(), med_pos, errors.end());
  median = *med_pos;
}


/// Return the calibration matrix used when generating new cameras
const vpgl_calibration_matrix<double>&
bundle_adjust_tracks
::init_calibration() const
{
  return init_K_;
}

/// Return the cameras modified during optimization.
/// The output is a map from frame numbers to perspective cameras.
const bundle_adjust_tracks::Cam_map&
bundle_adjust_tracks
::cameras() const
{
  return cameras_;
}


/// Set the 3-d points modified during optimization.
/// The output is a map from track IDs to 3-d points
const bundle_adjust_tracks::Point_map&
bundle_adjust_tracks
::points() const
{
  return world_points_;
}


/// Return true if estimation of a single fixed focal length is enabled.
bool bundle_adjust_tracks
::estimate_focal_length() const
{
  return estimate_focal_length_;
}


/// Return true if verbose status messages are enabled.
bool bundle_adjust_tracks
::verbose() const
{
  return verbose_;
}


/// compute track_id_index_ by pruning tracks by length and optionally
/// limiting to maximum number of tracks (if max_num_tracks > 0)
void bundle_adjust_tracks
::track_indices(unsigned min_track_len, unsigned max_num_tracks)
{
  // collect and sort by length the track IDs with length more than min_track_len
  typedef vcl_pair<unsigned,unsigned> Uint_pair;
  vcl_vector<Uint_pair> track_pairs;
  for (unsigned i=0; i<tracks_.size(); ++i)
  {
    if (!allow_bootstrap_ &&
        world_points_.find(tracks_[i]->id()) == world_points_.end() )
    {
      continue;
    }
    if (tracks_[i]->history().size() < min_track_len)
    {
      continue;
    }
    track_pairs.push_back( Uint_pair(tracks_[i]->history().size(),
                                     tracks_[i]->id()) );
  }
  // sort in decending order by track length
  vcl_sort(track_pairs.begin(), track_pairs.end(), vcl_greater<Uint_pair>());

  if (max_num_tracks == 0)
  {
    max_num_tracks = track_pairs.size();
  }

  // generate a sequential numbering of the remaining tracks
  track_id_index_.clear();
  for (unsigned i=0; i<max_num_tracks; ++i)
  {
    unsigned ind = track_id_index_.size();
    track_id_index_[track_pairs[i].second] = ind;
  }

  // compute the inverse mapping from indices back to orginal track IDs
  typedef vcl_map<unsigned,unsigned>::const_iterator Map_itr;
  track_ids_.resize(track_id_index_.size());
  for (Map_itr i=track_id_index_.begin(); i!=track_id_index_.end(); ++i)
  {
    track_ids_[i->second] = i->first;
  }
}


/// compute frame_index_ by skipping frames
void bundle_adjust_tracks
::frame_indices(unsigned num_frames)
{
  // collect all frame numbers that contain at least
  // one track id found in track_id_index_
  vcl_set<unsigned> frames;
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    // only use frames from valid tracks
    if(track_id_index_.find(tracks_[i]->id()) == track_id_index_.end())
      continue;
    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      frames.insert(tracks_[i]->history()[j]->time_.frame_number());
    }
  }

  unsigned frame_step = 1; 
  if (num_frames > 0)
  {
    frame_step = frames.size() / num_frames;
  }
  if (frame_step == 0)
  {
    frame_step = 1;
  }

  // map frame numbers to continuous indices stepping by frame_step
  frame_index_.clear();
  unsigned c=0;
  for (vcl_set<unsigned>::const_iterator i=frames.begin();
       i!=frames.end(); ++i, ++c)
  {
    if (!allow_bootstrap_ &&
        cameras_.find(*i) == cameras_.end() )
    {
      continue;
    }
    if(c % frame_step == 0){
      unsigned ind = frame_index_.size();
      frame_index_[*i] = ind;
    }
  }

  // compute the inverse mapping from indices back to orginal frame numbers
  typedef vcl_map<unsigned,unsigned>::const_iterator Map_itr;
  frames_.resize(frame_index_.size());
  for (Map_itr i=frame_index_.begin(); i!=frame_index_.end(); ++i)
  {
    frames_[i->second] = i->first;
  }
}


/// Collect image points from the tracks using the selected subsets of tracks
/// and frames.  Also generate visibility mask for bundle adjustment.
void bundle_adjust_tracks
::extract_image_points(vcl_vector<vgl_point_2d<double> >& points,
                       vcl_vector<vcl_vector<bool> >& mask) const
{
  mask.clear();
  mask.resize(frame_index_.size(),
              vcl_vector<bool>(track_id_index_.size(),false));

  // first, collect and reorder points by frame index then track index
  typedef vcl_map<vcl_pair<unsigned,unsigned>,vgl_point_2d<double> > Pmap;
  Pmap points_map;
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    typedef vcl_map<unsigned,unsigned>::const_iterator Map_itr;
    // only use valid tracks
    Map_itr t_itr = track_id_index_.find(tracks_[i]->id());
    if (t_itr == track_id_index_.end())
      continue;
    unsigned ti = t_itr->second;

    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      unsigned frame_num = tracks_[i]->history()[j]->time_.frame_number();
      Map_itr f_itr = frame_index_.find(frame_num);
      if (f_itr == frame_index_.end())
        continue;
      unsigned fi = f_itr->second;

      mask[fi][ti] = true;
      vgl_point_2d<double> p(tracks_[i]->history()[j]->loc_[0],
                             tracks_[i]->history()[j]->loc_[1]);
      points_map[vcl_pair<unsigned,unsigned>(fi,ti)] = p;
    }
  }
  // next, extract the image point vector in order (maps are sorted)
  points.clear();
  for(Pmap::const_iterator p=points_map.begin(); p!=points_map.end(); ++p)
  {
    points.push_back(p->second);
  }
}


/// Returns a count of the weights with a value <= threshold
unsigned bundle_adjust_tracks
::outlier_count(const vcl_vector<double>& weights,
                double threshold) const
{
  unsigned num_outliers = 0;
  for(unsigned i=0; i<weights.size(); ++i)
  {
    if(weights[i] <= threshold)
    {
      ++num_outliers;
    }
  }
  return num_outliers;
}


/// Run multiple optimizations at decreasing scales determined by
/// \a init_scale, \a final_scale, and \a scale_step
bool bundle_adjust_tracks
::muliscale_ba(vpgl_bundle_adjust& ba,
               vcl_vector<vpgl_perspective_camera<double> >& cameras,
               vcl_vector<vgl_point_3d<double> >& world_points,
               vcl_vector<double>& weights,
               const vcl_vector<vgl_point_2d<double> >& image_points,
               const vcl_vector<vcl_vector<bool> >& mask,
               double init_scale,
               double final_scale,
               double scale_step) const
{
  vcl_vector<vpgl_perspective_camera<double> > cameras_try(cameras);
  vcl_vector<vgl_point_3d<double> > points_try(world_points);
  for (double scale = init_scale; scale >= final_scale; scale /= scale_step)
  {
    ba.set_m_estimator_scale(scale);
    if (verbose_){
      vcl_cout << "setting M-estimator scale to "<<scale<<vcl_endl;
    }
    bool converge = ba.optimize(cameras_try, points_try, image_points, mask);
    if (!converge)
    {
      return false;
    }
    const vcl_vector<double>& w = ba.final_weights();
    if (outlier_count(w)*2 > w.size())
    {
      vcl_cout << "too many outliers. "<<outlier_count(w)<<"/"<<w.size()<<vcl_endl;
      break;
    }

    if (verbose_)
    {
      vcl_cout << "fraction outliers "<<outlier_count(w)<<"/"<<w.size()<<vcl_endl;
    }
    cameras = cameras_try;
    world_points = points_try;
    weights = w;
  }
  return true;
}


/// Update the cameras in the cameras_ map using these cameras and
/// the frame numbers in frames_
void bundle_adjust_tracks
::update_cameras(const vcl_vector<vpgl_perspective_camera<double> > cameras)
{
  assert(cameras.size() == frames_.size());
  for(unsigned i=0; i<cameras.size(); ++i)
  {
    cameras_[frames_[i]] = cameras[i];
  }
}


/// Update the points in the world_points_ map using these points and
/// the track IDs in track_ids_
void bundle_adjust_tracks
::update_points(const vcl_vector<vgl_point_3d<double> >& world_points)
{
  assert(world_points.size() == track_ids_.size());
  for(unsigned i=0; i<world_points.size(); ++i)
  {
    world_points_[track_ids_[i]] = world_points[i];
  }
}


/// Assign the weights as properties on the tracks_ using the mask,
/// track_ids_, and frames_ to do the mapping.
void bundle_adjust_tracks
::update_weights(const vcl_vector<double>& weights,
                 const vcl_vector<vcl_vector<bool> >& mask) const
{
  // compressed row storage indexer
  // to map sparse matrix indices into dense weight vector indices.
  vnl_crs_index crs(mask);
  
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    typedef vcl_map<unsigned,unsigned>::const_iterator Map_itr;
    // only use valid tracks
    Map_itr t_itr = track_id_index_.find(tracks_[i]->id());
    if (t_itr == track_id_index_.end())
    {
      continue;
    }
    unsigned ti = t_itr->second;

    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      unsigned frame_num = tracks_[i]->history()[j]->time_.frame_number();
      Map_itr f_itr = frame_index_.find(frame_num);
      if (f_itr == frame_index_.end())
      {
        continue;
      }
      unsigned fi = f_itr->second;

      if (mask[fi][ti])
      {
        property_map& data = tracks_[i]->history()[j]->data_;
        data.set<double>("weight", weights[crs(fi,ti)]);
      }
    }
  }
}
                 

/// Predict missing cameras in the sequence by interpolating between
/// available cameras
void bundle_adjust_tracks
::interpolate_missing_cameras()
{
  if( cameras_.size() < 2 )
  {
    vcl_cerr << "Error in bundle_adjust_tracks::interpolate_cameras: cannot "
             << "interpolate with "<<cameras_.size()<<" cameras"<<vcl_endl;
    return;
  }
  
  // collect all frame numbers 
  vcl_set<unsigned> all_frames;
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      all_frames.insert(tracks_[i]->history()[j]->time_.frame_number());
    }
  }

  // compute interpolated cameras
  Cam_map interp_cameras;

  // prev_cam_itr and curr_cam_itr point to a pair of frames that are
  // sequential in the camera map.  Frames in all_frames are expected to
  // fall between the frame numbers in camera map.  These in between frames
  // are interpolated between the bounding prev_cam_itr and curr_cam_itr
  Cam_map::const_iterator curr_cam_itr = cameras_.begin();
  Cam_map::const_iterator prev_cam_itr = curr_cam_itr++;
  for (vcl_set<unsigned>::const_iterator frame_itr = all_frames.begin();
       frame_itr != all_frames.end(); ++frame_itr)
  {
    // shift the bounding camera_map iterators to contain the current frame
    Cam_map::const_iterator next_cam_itr = curr_cam_itr;
    ++next_cam_itr;
    while (next_cam_itr != cameras_.end() &&
           curr_cam_itr->first <= *frame_itr)
    {
      prev_cam_itr = curr_cam_itr;
      curr_cam_itr = next_cam_itr;
      ++next_cam_itr;
    }

    if (*frame_itr == prev_cam_itr->first)
    {
      interp_cameras[*frame_itr] = prev_cam_itr->second;
    }
    else if (*frame_itr == curr_cam_itr->first)
    {
      interp_cameras[*frame_itr] = curr_cam_itr->second;
    }
    else
    {
      // compute the interpolation parameter, usually between 0 and 1
      // it can be < 0 before the first camera and > 1 after the last
      double s = (double(*frame_itr)-prev_cam_itr->first);
      s /= double(curr_cam_itr->first) - prev_cam_itr->first;

      interp_cameras[*frame_itr] = interpolate_camera(prev_cam_itr->second,
                                                      curr_cam_itr->second,
                                                      s);
    }

  }
  cameras_.swap(interp_cameras);
}


/// Decimate the avaiable cameras by a factor of \a step
void bundle_adjust_tracks
::decimate_cameras(unsigned step)
{
  Cam_map::iterator cam_itr = cameras_.begin();
  for (unsigned c=0; cam_itr != cameras_.end(); ++c)
  {
    Cam_map::iterator curr_cam_itr = cam_itr++;
    if (c%step != 0)
    {
      cameras_.erase(curr_cam_itr);
    }
  }
}


/// Estimate missing points by linear (DLT) triangulation using all
/// avaiable cameras
void bundle_adjust_tracks
::triangulate_missing_points()
{
  typedef vcl_map<unsigned,vgl_point_2d<double> > Obs_map;
 
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    // skip points that are already reconstructed
    Point_map::const_iterator p_itr = world_points_.find(tracks_[i]->id());
    if (p_itr != world_points_.end())
    {
      continue;
    }
    
    // collect all the observations of this track with known cameras
    Obs_map observations;
    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      unsigned frame_num = tracks_[i]->history()[j]->time_.frame_number();
      Cam_map::const_iterator c_itr = cameras_.find(frame_num);
      if (c_itr == cameras_.end())
        continue;

      vgl_point_2d<double> p(tracks_[i]->history()[j]->loc_[0],
                             tracks_[i]->history()[j]->loc_[1]);
      observations[frame_num] = p;
    }

    vnl_matrix<double> A(2*observations.size(), 4);
    unsigned int k=0;
    for (Obs_map::const_iterator o_itr = observations.begin();
         o_itr != observations.end(); ++o_itr, ++k)
    {
      const vnl_matrix_fixed<double,3,4>& P = cameras_[o_itr->first].get_matrix();
      A.set_row(2*k, o_itr->second.x()*P.get_row(2) - P.get_row(0));
      A.set_row(2*k+1, o_itr->second.y()*P.get_row(2) - P.get_row(1));
    }
    vnl_svd<double> svd_solver(A);
    vnl_vector_fixed<double,4> p = svd_solver.nullvector();

    world_points_[tracks_[i]->id()] = vgl_point_3d<double>(p[0]/p[3],
                                                           p[1]/p[3],
                                                           p[2]/p[3]);
  }

}


/// Remove outlier tracks from the 3-d points collection.
/// Mark track observations with an M-estimator weight <= \a weight_threshold
/// as outliers.  Remove tracks with a fraction of outlier observations
/// greater than \a outlier_frac
void bundle_adjust_tracks
::remove_outliers(double outlier_frac, unsigned min_inliers,
                  double weight_threshold)
{
  for(unsigned i=0; i<tracks_.size(); ++i)
  {
    // skip points that are not reconstructed
    Point_map::iterator p_itr = world_points_.find(tracks_[i]->id());
    if (p_itr == world_points_.end())
    {
      continue;
    }

    // collect the weights
    unsigned num_weights = 0;
    unsigned num_outliers = 0;
    for(unsigned j=0; j<tracks_[i]->history().size(); ++j)
    {
      const property_map& data = tracks_[i]->history()[j]->data_;
      const double* w = data.get_if_avail<double>("weight");
      if (w)
      {
        ++num_weights;
        if (*w <= weight_threshold)
        {
          ++num_outliers;
        }
      }
    }
    if (num_weights == 0)
    {
      continue;
    }

    if (num_outliers > num_weights*outlier_frac ||
        num_weights - num_outliers < min_inliers)
    {
      if (verbose_)
      {
        vcl_cout << "Removing outlier track "<<tracks_[i]->id()
                 << " with outlier ratio "
                 << num_outliers<<"/"<<num_weights<<vcl_endl;
      }
      world_points_.erase(p_itr);
    }
  }
}
  

/// Interpolate a camera between \a cam1 and \a cam2 at parameter \a s.
/// \a s is such that the result is \a cam1 at s==0.0 and \a cam2 at s==1.0
vpgl_perspective_camera<double>
bundle_adjust_tracks
::interpolate_camera(const vpgl_perspective_camera<double>& cam1,
                     const vpgl_perspective_camera<double>& cam2,
                     double s)
{
  // extract references to the components of the cameras
  const vpgl_calibration_matrix<double>& K1 = cam1.get_calibration();
  const vpgl_calibration_matrix<double>& K2 = cam2.get_calibration();
  const vgl_rotation_3d<double>& R1 = cam1.get_rotation();
  const vgl_rotation_3d<double>& R2 = cam2.get_rotation();
  const vgl_point_3d<double>& c1 = cam1.get_camera_center();
  const vgl_point_3d<double>& c2 = cam2.get_camera_center();

  double s1 = 1.0 - s;

  // interpolate calibration
  double focal_len = s1*K1.focal_length() + s*K2.focal_length();
  vgl_point_2d<double> pp = midpoint(K1.principal_point(),K2.principal_point(),s);
  double x_scale = s1*K1.x_scale() + s*K2.x_scale();
  double y_scale = s1*K1.y_scale() + s*K2.y_scale();
  double skew = s1*K1.skew() + s*K2.skew();
  vpgl_calibration_matrix<double> K(focal_len,pp,x_scale,y_scale,skew);

  // interpolate rotation
  // map orientation R2 into the tangent space of R1
  // interpolate in the Rodrigues vector space (Lie Algebra)
  // then map back.
  vgl_rotation_3d<double> R(s*(R1.inverse()*R2).as_rodrigues());
  R = R1 * R;

  // interpolate position
  // for now, linearly interpolate the postion
  // interpolation in se(3) Lie Algebra would likely be better because
  // it takes rotation into account
  vgl_point_3d<double> c = midpoint(c1,c2,s);

  return vpgl_perspective_camera<double>(K,c,R);
}
