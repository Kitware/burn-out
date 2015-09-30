/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_bundle_adjust_tracks_h_
#define vidtk_bundle_adjust_tracks_h_

#include <vcl_vector.h>
#include <vcl_map.h>

#include <vgl/vgl_point_3d.h>
#include <vpgl/vpgl_perspective_camera.h>
#include <vpgl/algo/vpgl_bundle_adjust.h>

#include <tracking/track.h>


namespace vidtk
{


/// Apply bundle adjustment to a set of tracked feature points to recover
/// 3-d reconstructed points and perspective cameras.
/// The tracks should represent (primarily) fixed points on a rigid scene
/// with motion in the images due to camera motion.
class bundle_adjust_tracks
{
public:
  typedef vcl_map<unsigned, vpgl_perspective_camera<double> > Cam_map;
  typedef vcl_map<unsigned, vgl_point_3d<double> > Point_map;
  
  /// constructor - specifying tracks
  bundle_adjust_tracks(const vcl_vector< vidtk::track_sptr >& tracks);

  /// Set the initial cameras to use during optimization.
  /// The input is a map from frame numbers to perspective cameras.
  void set_cameras(const Cam_map& cameras);

  /// Set the initial 3-d points to use during optimization.
  /// The input is a map from track IDs to 3-d points
  void set_points(const Point_map& points);

  /// Set the initial calibration matrix used when generating cameras
  /// \note does not modify cameras set by set_cameras
  void set_init_calibration(const vpgl_calibration_matrix<double>& K);

  /// Enable or disable estimation of a single fixed focal length used
  /// throughout the entire sequence
  void set_estimate_focal_length(bool e);

  /// Enable or disable verbose status messages
  void set_verbose(bool v);

  /// Run an initial optimization starting from nothing.
  /// Sample a subset of frames and tracks and run the optimization starting
  /// with default initialization.  
  bool bootstrap_optimization(unsigned num_frames = 10,
                              unsigned num_tracks = 100);

  /// Predict missing cameras in the sequence by interpolating between
  /// available cameras
  void interpolate_missing_cameras();

  /// Decimate the avaiable cameras by a factor of \a step
  void decimate_cameras(unsigned step = 2);

  /// Estimate missing points by linear (DLT) triangulation using all
  /// avaiable cameras
  void triangulate_missing_points();

  /// Remove outlier tracks from the 3-d points collection.
  /// Mark track observations with an M-estimator weight <= \a weight_threshold
  /// as outliers.  Remove tracks with a fraction of outlier observations
  /// greater than \a outlier_frac
  void remove_outliers(double outlier_frac=0.25, unsigned min_inliers = 3,
                       double weight_threshold=0.0);

  /// Optimize all initialized cameras and all initialized points.
  /// Starts from the provided initialization of cameras and points
  bool optimize();

  /// Optimize all initialized cameras keeping points fixed.
  /// Starts from the provided initialization of cameras and points
  bool optimize_cameras();

  /// Compute the reprojections errors of all estimated 3-d points into all
  /// estimated cameras and returns error metrics: maximum, median, and mean
  void reproject_error_stats(double& max, double& median, double& mean);

  /// Return the calibration matrix used when generating new cameras
  const vpgl_calibration_matrix<double>& init_calibration() const;

  /// Return the cameras modified during optimization.
  /// The output is a map from frame numbers to perspective cameras.
  const Cam_map& cameras() const;

  /// Return the 3-d points modified during optimization.
  /// The output is a map from track IDs to 3-d points
  const Point_map& points() const;

  /// Return true if estimation of a single fixed focal length is enabled.
  bool estimate_focal_length() const;

  /// Return true if verbose status messages are enabled.
  bool verbose() const;

private:
  /// compute track_id_index_ by pruning tracks by length and optionally
  /// limiting to maximum number of tracks (if max_num_tracks > 0)
  void track_indices(unsigned min_track_len = 1,
                     unsigned max_num_tracks = 0);

  /// compute frame_index_ by skipping frames 
  void frame_indices(unsigned num_frames = 0);

  /// Collect image points from the tracks using the selected subsets of tracks
  /// and frames.  Also generate visibility mask for bundle adjustment.
  void extract_image_points(vcl_vector<vgl_point_2d<double> >& points,
                            vcl_vector<vcl_vector<bool> >& mask) const;

  /// Returns a count of the weights with a value <= threshold
  unsigned outlier_count(const vcl_vector<double>& weights,
                         double threshold = 0.0) const;

  /// Run multiple optimizations at decreasing scales determined by
  /// \a init_scale, \a final_scale, and \a scale_step
  bool muliscale_ba(vpgl_bundle_adjust& ba,
                    vcl_vector<vpgl_perspective_camera<double> >& cameras,
                    vcl_vector<vgl_point_3d<double> >& world_points,
                    vcl_vector<double>& weights,
                    const vcl_vector<vgl_point_2d<double> >& image_points,
                    const vcl_vector<vcl_vector<bool> >& mask,
                    double init_scale,
                    double final_scale,
                    double scale_step) const;

  /// Update the cameras in the cameras_ map using these cameras and
  /// the frame numbers in frames_
  void update_cameras(const vcl_vector<vpgl_perspective_camera<double> > cameras);

  /// Update the points in the world_points_ map using these points and
  /// the track IDs in track_ids_
  void update_points(const vcl_vector<vgl_point_3d<double> >& world_points);

  /// Assign the weights as properties on the tracks_ using the mask,
  /// track_ids_, and frames_ to do the mapping.
  void update_weights(const vcl_vector<double>& weights,
                      const vcl_vector<vcl_vector<bool> >& mask) const;


  /// Interpolate a camera between \a cam1 and \a cam2 at parameter \a s.
  /// \a s is such that the result is \a cam1 at s==0.0 and \a cam2 at s==1.0
  vpgl_perspective_camera<double>
  interpolate_camera(const vpgl_perspective_camera<double>& cam1,
                     const vpgl_perspective_camera<double>& cam2,
                     double s);

  ///////////////////////////////////////////////////////////////////
  // Member variables
              
  /// the input collection of tracks
  vcl_vector< vidtk::track_sptr > tracks_;

  /// enable estimation of the focal length
  bool estimate_focal_length_;

  /// enable printing of verbose status messages
  bool verbose_;

  /// initial intrinsic calibration matrix to use for creating new cameras
  vpgl_calibration_matrix<double> init_K_;

  /// Allows the indexing of uninitialized tracks for the purpose of bootstraping
  bool allow_bootstrap_;


  /// a map from frame numbers to cameras
  Cam_map cameras_;

  /// a map from track IDs to 3-d world points
  Point_map world_points_;

  /// map from active track IDs to sequential point index
  vcl_map<unsigned,unsigned> track_id_index_;
  /// map from sequential point index back to orginal track ID
  vcl_vector<unsigned> track_ids_;
  
  /// map from active frame numbers to sequential frame index
  vcl_map<unsigned,unsigned> frame_index_;
  /// map from sequental frame index back to original frame numbers
  vcl_vector<unsigned> frames_;
};



} // end namespace vidtk


#endif // vidtk_bundle_adjust_tracks_h_
