/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/track_initializer_process.h>
#include <tracking/track_velocity_init_functions.h>

#include <vnl/vnl_hungarian_algorithm.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_det.h>
#include <vnl/vnl_inverse.h>
#include <vcl_algorithm.h>
#include <vcl_vector.h>
#include <vcl_limits.h>
#include <utilities/image_histogram.h>
#include <utilities/greedy_assignment.h>
#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <tracking/tracking_keys.h>
#include <rsdl/rsdl_bins_2d.h>
#include <rsdl/rsdl_bins_2d.txx>
#include <rrel/rrel_linear_regression.h>
#include <rrel/rrel_irls.h>
#include <rrel/rrel_tukey_obj.h>

#include <vnl/algo/vnl_svd.h>


namespace vidtk
{

track_initializer_process
::track_initializer_process( vcl_string const& name )
  : process( name, "track_initializer_process" ),
    mod_buf_( NULL ),
    timestamp_buf_( NULL ),
    mf_params_( NULL ),
    min_area_similarity_(0.2),
    min_color_similarity_(0.2)
{
  config_.add( "delta", "1" );
  config_.add_parameter( "kinematics_sigma_gate", "1", "units in Mahalanobis distance" );
  config_.add( "allowed_miss_count", "0" );
  config_.add( "miss_penalty", "1000000" );
  config_.add_parameter( "tangential_sigma", "0.4", "units in meters" );
  config_.add_parameter( "normal_sigma", "0.4", "units in meters" );
  config_.add( "use_bins", "false" );
  config_.add( "bin_lower_left", "0 0" );
  config_.add( "bin_upper_right", "5000 5000" );
  config_.add( "bin_size", "100 100" );
  config_.add( "assignment_algorithm", "hungarian" );
  config_.add( "maximum_assignment_size", "4294967295" );
  config_.add( "init_interval", "0" );
  config_.add_parameter( "init_max_speed_filter", "-1", "Don't initialize tracks whose speed is greater than this number." );
  config_.add_parameter( "init_min_speed_filter", "-1", "Don't initialize tracks whose speed is smaller than this number." );
  config_.add( "directional_prior:disabled", "true" );
  config_.add( "directional_prior:vector", "-1 -1" );
  config_.add( "directional_prior:min_similarity", "0.5" );
  config_.add_parameter( "init_vel_method", "last_state",
                         "How the velocities are initalize.  (1) last_state uses the "
                         "last detection and beginning to calculate velocity.  All states "
                         "are set to the same value.  (2) smooth_speed calculates velocity"
                         " by assuming constant acceleration and the direction is the same"
                         " as between the first and last detection (robust estimation).  "
                         "(3) fit_points fits the points to a constant acceleration model"
                         " to estimate the initial velocities (robust estimation)" );
  config_.add_parameter( "init_vel_tukey", "1.5",
                         "Beaton-Tukey normalised residual value." );
  config_.add_parameter( "init_vel_look_back", "2", "the number of detections to look back for calculating velocity" );
  config_.add_parameter( "min_area_similarity", "0.2", "ignore object with area similarity"
                         "less than this. Range is [0,1]" );
  config_.add_parameter( "min_color_similarity", "0.2", "ignore object with color similarity"
                         "less than this. Range is [0,1]" );
}


track_initializer_process
::~track_initializer_process()
{
}


config_block
track_initializer_process
::params() const
{
  return config_;
}


bool
track_initializer_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "min_area_similarity", min_area_similarity_ );
    blk.get( "min_color_similarity", min_color_similarity_ );
    blk.get( "delta", delta_ );
    blk.get( "kinematics_sigma_gate", kinematics_sigma_gate_ );
    blk.get( "allowed_miss_count", allowed_miss_count_ );
    blk.get( "miss_penalty", miss_penalty_ );
    double t;
    blk.get( "tangential_sigma", t );
    tang_sigma_sqr_ = t * t;
    one_over_tang_sigma_sqr_ = 1./tang_sigma_sqr_;
    double n;
    blk.get( "normal_sigma", n );
    norm_sigma_sqr_ = n * n;
    one_over_norm_sigma_sqr_ = 1/norm_sigma_sqr_;
    max_query_radius_ = vcl_max(t*kinematics_sigma_gate_,n*kinematics_sigma_gate_);

    blk.get( "use_bins", use_bins_ );
    if( use_bins_ )
    {
      blk.get( "bin_lower_left", bin_lo_ );
      blk.get( "bin_upper_right", bin_hi_ );
      blk.get( "bin_size", bin_size_ );
    }

    vcl_string assn_alg;
    blk.get( "assignment_algorithm", assn_alg );
    if( assn_alg == "greedy" )
    {
      assignment_alg_ = GREEDY;
    }
    else if( assn_alg == "hungarian" )
    {
      assignment_alg_ = HUNGARIAN;
    }
    else
    {
      log_error( "Unknown assignment algorithm " << assn_alg << "\n" );
      throw unchecked_return_value( "unknown assignment algorithm" );
    }
    blk.get( "maximum_assignment_size", max_assignment_size_ );
    blk.get( "init_interval", init_interval_ );
    blk.get( "init_max_speed_filter", init_max_speed_filter_ );
    blk.get( "init_min_speed_filter", init_min_speed_filter_ );
    blk.get( "directional_prior:disabled", directional_prior_disabled_ );
    blk.get( "directional_prior:vector", directional_prior_ );
    blk.get( "directional_prior:min_similarity", min_directional_similarity_ );

    blk.get( "init_vel_tukey" ,init_bt_norm_ris_val_ );
    blk.get( "init_vel_look_back", init_look_back_);
    vcl_string init_mod;
    blk.get( "init_vel_method", init_mod);
    if( init_mod == "last_state" )
    {
       method_ = USE_N_BACK_END_VEL;
    }
    else if( init_mod == "smooth_speed" )
    {
      method_ = USE_ROBUST_SPEED_FIT;
    }
    else if ( init_mod == "fit_points" )
    {
      method_ = USE_ROBUST_POINTS_FIT;
    }
    else
    {
      log_error( "Unknown assignment algorithm " << init_mod << "\n" );
      throw unchecked_return_value( "unknown assignment algorithm" );
    }
  }
  catch( unchecked_return_value& )
  {
    // restore old values
    set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
track_initializer_process
::initialize()
{
  if( delta_ < 1 )
  {
    log_error( "Delta must be at least 1, found " << delta_ << "\n" );
    return false;
  }

  if( allowed_miss_count_ > delta_ - 2 )
  {
    log_error( "Can't miss more than " << delta_-2
      << " detections, since first and last frame must have them!\n" );
    return false;
  }

  last_time_.set_time( 0 );
  last_time_.set_frame_number( 0 );

  if( !directional_prior_disabled_ )
  {
    //normalize
    directional_prior_.normalize();
    //TODO: move to step() & add img2wld homography mutiplication?
    directional_prior_wld_[0] = directional_prior_[0];
    directional_prior_wld_[1] = directional_prior_[1];
    directional_prior_wld_[2] = 0;
  }

#ifdef TEST_INIT
  next_track_id_ = 1;
#endif
  return true;
}

bool
track_initializer_process
::step()
{
  assert( mod_buf_ != NULL );
  assert( timestamp_buf_ != NULL );
  assert( wld2img_H_buf_ != NULL );

  if ( mod_buf_->capacity() < delta_+1 )
  {
    log_fatal_error( "MOD buffer doesn't have enough capacity: has " <<
                     mod_buf_->capacity() << "; needs at least " << delta_+1 );
  }
  if ( timestamp_buf_->capacity() < delta_+1 )
  {
    log_fatal_error( "Timestamp buffer doesn't have enough capacity: has " <<
                     timestamp_buf_->capacity() << "; needs at least " << delta_+1 );
  }
  if ( wld2img_H_buf_->capacity() < delta_+1 )
  {
    log_fatal_error( "Homography buffer doesn't have enough capacity: has " <<
                     wld2img_H_buf_->capacity() << "; needs at least " << delta_+1 );
  }

  tracks_.clear();


  spatial_cov_.fill( 0 );
  spatial_cov_(0,0) = tang_sigma_sqr_;
  spatial_cov_(1,1) = norm_sigma_sqr_;


  // The source detections come from the earliest frame.  The target
  // detections come from the latest frame.  The intermediate frames
  // are used to build up the cost.
  //
  // Build an assignment matrix with links from every MOD in the
  // source frame to every MOD in the target frame.  The cost of the
  // link is the amount of support in the intermediate frames,
  // assuming linear motion.  So, in 1D, if there is an MOD at
  // location 10 at t=0, and this is linked to an MOD at location 20
  // at t=5, we expect MODs at locations 12, 14, 16 and 18 on at
  // t=1,2,3,4, repectively.  The cost measures the degree to which
  // this is true.  The more this is true, the more plausible that
  // this is a real moving object.  (In essence, we are doing some
  // sort of tracking here.)
  //
  // The MODs can be flagged by (have the property) "used_in_track".
  // If they have this flag, the MOD is ignored by this algorithm, and
  // hence will not be used to hypothesize or support a new track.

  // If we don't have enough data yet, return success without any
  // detected objects.  We'll just wait until we have enough data.
  if( ! ( mod_buf_->has_datum_at( delta_ ) &&
          timestamp_buf_->has_datum_at( delta_ ) &&
          wld2img_H_buf_->has_datum_at( delta_ ) ) )
  {
    return true;
  }

  vcl_vector<image_object_sptr> const& tmp_src_objs = mod_buf_->datum_at( delta_ );
  vcl_vector<image_object_sptr> const& tmp_tgt_objs = mod_buf_->datum_at( 0 );
  vcl_vector<image_object_sptr> src_objs;
  src_objs.reserve(tmp_src_objs.size());
  vcl_vector<image_object_sptr> tgt_objs;
  tgt_objs.reserve(tmp_tgt_objs.size());
  for(vcl_vector<image_object_sptr>::const_iterator i = tmp_src_objs.begin(); i != tmp_src_objs.end(); ++i)
  {
    image_object_sptr const & ios = *i;
    if( !ios->data_.is_set( tracking_keys::used_in_track ) )
    {
      src_objs.push_back(ios);
    }
  }
  for(vcl_vector<image_object_sptr>::const_iterator i = tmp_tgt_objs.begin(); i != tmp_tgt_objs.end(); ++i)
  {
    image_object_sptr const & ios = *i;
    if( !ios->data_.is_set( tracking_keys::used_in_track ) )
    {
      tgt_objs.push_back(ios);
    }
  }
  log_debug( "Src from: " << tmp_src_objs.size() << " to " << src_objs.size() << vcl_endl );
  log_debug( "Tgt from: " << tmp_tgt_objs.size() << " to " << tgt_objs.size() << vcl_endl );

  timestamp const src_time = timestamp_buf_->datum_at( delta_ );
  timestamp const tgt_time = timestamp_buf_->datum_at( 0 );
  double const full_interval = tgt_time.time_in_secs() - src_time.time_in_secs();

  if( tgt_time.time_in_secs() - last_time_.time_in_secs() < init_interval_ )
  {
    vcl_cout << name() << ": only "
             << tgt_time.time_in_secs() - last_time_.time_in_secs()
             << " (< "
             << init_interval_
             << ") seconds haven't elapsed, so not initializing\n";
    return true;
  }
  last_time_ = tgt_time;


  unsigned const Ns = src_objs.size();
  unsigned const Nt = tgt_objs.size();

  log_debug( "track_init: from " << Ns << " -> " << Nt << "\n" );


  if( Ns * Nt > max_assignment_size_ )
  {
    log_warning( name() << ": problem size " << Ns << "x" << Nt
                 << " (=" << Ns*Nt << ") exceeds " << max_assignment_size_
                 << ". Skipping.\n" );
    return true;
  }

  vnl_matrix<double> cost( Ns, Nt, 0.0 );

  // Build a binning structure to find the closest points, if
  // requested.  If we are not using bins, this vector will be empty.
  vcl_vector< bin_type > bins;
  vcl_vector< vcl_vector< image_object_sptr > > brut_force;

  if( use_bins_ )
  {
    log_debug( "building " << delta_-1 << " bins." << vcl_endl );

    bins.resize(delta_-1);
    for( unsigned f = 1; f < delta_; ++f )
    {
      log_debug( "  populating bin " << f << "/" << bins.size() << vcl_endl );
      bins[f-1].reset( vnl_double_2( bin_lo_[0], bin_lo_[1] ),
                       vnl_double_2( bin_hi_[0], bin_hi_[1] ),
                       vnl_double_2( bin_size_[0], bin_size_[1] ) );
      // Intermediate objects.
      vcl_vector<image_object_sptr> const& int_objs = mod_buf_->datum_at( f );
      unsigned const Ni = int_objs.size();
      for( unsigned i = 0; i < Ni; ++i )
      {
        if( ! int_objs[i]->data_.is_set( tracking_keys::used_in_track ) )
        {
          vnl_vector_fixed<double,3> const& loc = int_objs[i]->world_loc_;
          bins[f-1].add_point( vnl_double_2( loc[0], loc[1] ),
                               int_objs[i] );
        }
      }
    }
  }
  else
  {
    brut_force.resize(delta_-1);
    for( unsigned f = 1; f < delta_; ++f )
    {
      // Intermediate objects.
      vcl_vector<image_object_sptr> const& int_objs = mod_buf_->datum_at( f );
      unsigned const Ni = int_objs.size();
      for( unsigned i = 0; i < Ni; ++i )
      {
        if( ! int_objs[i]->data_.is_set( tracking_keys::used_in_track ) )
        {
          brut_force[f-1].push_back(int_objs[i]);
        }
      }
    }
  }

  log_debug( "Building cost matrix." << vcl_endl );
  static double const Inf = vcl_numeric_limits<double>::infinity();
  double max_computed_cost = -Inf;

  min_area_similarity_ =
    (min_area_similarity_ < 0.0 || min_area_similarity_ >= 1.0) ?
    0.1 : min_area_similarity_;

  min_color_similarity_ =
    (min_color_similarity_ < 0.0 || min_color_similarity_ >= 1.0) ?
    0.5 : min_color_similarity_;

  for( unsigned sidx = 0; sidx < Ns; ++sidx )
  {
    image_object const& src_obj = *src_objs[sidx];

    for( unsigned tidx = 0; tidx < Nt; ++tidx )
    {
      image_object const& tgt_obj = *tgt_objs[tidx];

      // If track speed is outside the *reasonable range*, then bail out on
      // this source-target detection pair.
      if( init_max_speed_filter_ >= 0.0 || init_min_speed_filter_ >= 0.0 )
      {
        vnl_double_3 dist_vec = src_obj.world_loc_ - tgt_obj.world_loc_;
        double dt = tgt_time.time_in_secs() - src_time.time_in_secs();
        double speed = dist_vec.magnitude() / dt;

        if( (init_max_speed_filter_ >= 0.0 && speed > init_max_speed_filter_)
         || (init_min_speed_filter_ >= 0.0 && speed < init_min_speed_filter_) )
        {
#ifdef DEBUG
          vcl_cout<<name()<<":init_max_speed_filter: from "<< src_obj.world_loc_ << " to " << tgt_obj.world_loc_
            << " in " << dt << " secs with "<< speed <<" pix/sec speed" <<vcl_endl;
#endif
          //cost(sidx,tidx) = (allowed_miss_count_+1)*miss_penalty_;
          cost(sidx,tidx) =  Inf;
          continue;
        }
      }

      if( !directional_prior_disabled_ )
      {
        vnl_double_3 motion_dir = tgt_obj.world_loc_ - src_obj.world_loc_;
        motion_dir.normalize();
        if( dot_product( motion_dir, directional_prior_wld_ )
          < min_directional_similarity_ )
        {
          cost(sidx,tidx) =  Inf;
          continue;
        }
      }

      if( mf_params_ && mf_params_->test_enabled )
      {
        // kick out src-tgt pairs when src and tgt have significant area difference
        double tmp_area_prob, area_diff;
        if(vcl_min( src_obj.area_ , tgt_obj.area_ ) <= 0)
        {
          tmp_area_prob = 0.0;
        }
        else
        {
          area_diff = vcl_max( src_obj.area_ , tgt_obj.area_ )
            / vcl_min( src_obj.area_ , tgt_obj.area_ ) - 1.0;
          tmp_area_prob= vcl_exp( - area_diff);
        }
        if(tmp_area_prob < min_area_similarity_)
        {
          cost(sidx,tidx) = Inf;
          continue;
        }

        // kick out src-tgt paris when src and tgt have significant color difference
        image_histogram<vxl_byte, bool> src_hist;
        image_histogram<vxl_byte, bool> tgt_hist;
        if(! src_obj.data_.get( tracking_keys::histogram, src_hist ) ||
          ! tgt_obj.data_.get( tracking_keys::histogram, tgt_hist ))
        {
          cost(sidx,tidx) = Inf;
          continue;
        }
        else
        {
          double tmp_color_sim = src_hist.compare(tgt_hist.get_h());
          double tmp_color_prob =
            mf_params_->get_similarity_score( tmp_color_sim, VIDTK_COLOR );
          if(tmp_color_prob < min_color_similarity_)
          {
            cost(sidx,tidx) = Inf;
            continue;
          }
        }
      }

      //bool first_valid_best_dist = true;
      //bool bail_on_max_velocity_filter = false;
      for( unsigned f = delta_-1; f > 0; --f )
      {
        double best_dist;
        image_object_sptr best_obj;

        if( mf_params_ && mf_params_->test_enabled )
        {
          closest_object2(src_obj,
                          tgt_obj,
                          src_time.time_in_secs(),
                          full_interval,
                          f,
                          bins,
                          best_dist,
                          best_obj );
        }
        else
        {
          closest_object( src_obj.world_loc_,
                          tgt_obj.world_loc_,
                          src_time.time_in_secs(),
                          full_interval,
                          f,
                          bins,
                          brut_force,
                          best_dist,
                          best_obj );
        }

        if( best_obj )
        {
          cost(sidx,tidx) += best_dist;
        }
        else
        {
          cost(sidx,tidx) += miss_penalty_;
        }

        if( cost(sidx, tidx) > max_computed_cost )
        {
          max_computed_cost = cost( sidx, tidx );
        }
      }// for f
    }// for Nt
  }// for Ns

  log_debug( "Cost computed.  Finding assignment." << vcl_endl );


  //Modifying cost matrix 'before' running Hungarian

  //Crieteria for modificaiton:
  //1. Cases with misses > allowed_miss_count_
  //    ==> Make them Inf
  //2. Cases with misses <= allowed_miss_count_
  //    ==> Reduce the value where miss_penalty_ -> 1.0
  //        because according to closest_object2() the
  //        distance lies in [0,1].
  // NOTE: miss_penalty_ should be substantially larger than 1.0 * (delta_-2)
  //3. Make Inf --> max_computed_cost + 1


  if( ! vnl_math::isfinite( max_computed_cost ) )
  {
    // no point falls in a gate.  Set the max_cost to some arbitrary
    // value. Then, every assignment derived from the hungarian
    // algorithm will be rejected because the individual cost will
    // exceed max_cost.
    max_computed_cost = 0;
  }

  // The vnl_hungarian_algorithm implementation cannot handle Inf.
  for( unsigned i = 0; i < Ns; ++i )
  {
    for( unsigned j = 0; j < Nt; ++j )
    {
      if( cost(i,j) <= max_computed_cost )
      {
        //1. # of misses > allowed_miss_count_
        if( cost(i,j) >= (allowed_miss_count_+1)*miss_penalty_ )
        {
          cost(i,j) = Inf;
        }
        //2. 1 miss_penalty_ --> 1.0
        // NOTE: miss_penalty_ should be substantially larger than 1.0 * (delta_-2)
        else if ( cost(i,j) >= miss_penalty_ )
        {
          if( mf_params_ && mf_params_->test_enabled )
          {
            unsigned n_misses = unsigned( cost(i,j) / miss_penalty_ );
            double rem = cost(i,j) - n_misses * miss_penalty_;
            cost(i,j) = n_misses * 1.0 + rem ; //1.0 is the cost of a miss with
          }
        }
        else
        {
          //do nothing to the 'good' values.
        }
      }

      //Not using 'else' here because new Inf values are being assigned
      //inside the 'if' above (case 1.)
      if( cost(i,j) > max_computed_cost )  // is Inf
      {
        cost(i,j) = max_computed_cost + 1;
      }
    }
  }

#ifdef DEBUG
  vcl_cout << "Cost=\n" << cost << "\n";
#endif

  vcl_vector<unsigned> assn;
  if( assignment_alg_ == HUNGARIAN )
  {
    assn = vnl_hungarian_algorithm<double>( cost );
  }
  else
  {
    assn = greedy_assignment( cost );
  }

  log_debug( "Assignment computed.  Creating possible tracks." << vcl_endl );

  for( unsigned sidx = 0; sidx < Ns; ++sidx )
  {
#ifdef DEBUG
    vcl_cout << "assn[" << sidx << "] = " << assn[sidx] <<", --> "<< cost(sidx,assn[sidx]) << "\n";
#endif
    if( assn[sidx] != unsigned(-1) )
    {
      unsigned const tidx = assn[sidx];
//      if( cost(sidx, tidx) < (allowed_miss_count_+1)*miss_penalty_ )
      if( cost(sidx, tidx) <= max_computed_cost )
      {
#ifdef DEBUG
    vcl_cout << "assn[" << sidx << "] = " << assn[sidx] <<", --> "<< cost(sidx,assn[sidx]) << "\n";
#endif
        image_object const& src_obj = *src_objs[sidx];
        image_object const& tgt_obj = *tgt_objs[tidx];

        // Set the velocity of each MOD on the track using the first
        // and last points, instead of doing a 1-step difference.  The
        // most important thing is that the velocity of the last point
        // is as accurate as possible, because that'll likely be used
        // to initialize a tracker.

        track_sptr trk = new track;

        add_new_state( trk, src_objs[sidx], src_time );

        // Find the best matching locations (again).  We could've
        // stored them earlier, when we computed the cost matrix, but
        // then, we'd have to essentially create N^2 tracks only to
        // keep O(N) of them.  Seems inefficient, especially given the
        // dynamic allocation that'd need to take place.  We could
        // store the relevant indicies in a vbl_array_3d(M,N,delta_).
        // Don't know if the cost of doing that is cheaper than just
        // recomputing the closest objects for the O(N) tracks that we
        // are going to create.
        for( unsigned f = delta_-1; f > 0; --f )
        {
          double best_dist;
          image_object_sptr best_obj;
          if( mf_params_ && mf_params_->test_enabled )
          {
            closest_object2( src_obj,
              tgt_obj,
              src_time.time_in_secs(),
              full_interval,
              f,
              bins,
              best_dist,
              best_obj );
          }
          else
          {
            closest_object( src_obj.world_loc_,
              tgt_obj.world_loc_,
              src_time.time_in_secs(),
              full_interval,
              f,
              bins,
              brut_force,
              best_dist,
              best_obj );
          }

          if( best_obj )
          {
#ifdef TEST_INIT
            best_obj->data_.set( tracking_keys::used_in_track , true );
            //TODO: need to up date the bin and brut force data structure if they exist
            if( use_bins_ )
            {
              log_debug( "building " << delta_-1 << " bins." << vcl_endl );

              bins.resize(delta_-1);
              for( unsigned f = 1; f < delta_; ++f )
              {
                log_debug( "  populating bin " << f << "/" << bins.size() << vcl_endl );
                bins[f-1].reset( vnl_double_2( bin_lo_[0], bin_lo_[1] ),
                                 vnl_double_2( bin_hi_[0], bin_hi_[1] ),
                                 vnl_double_2( bin_size_[0], bin_size_[1] ) );
                // Intermediate objects.
                vcl_vector<image_object_sptr> const& int_objs = mod_buf_->datum_at( f );
                unsigned const Ni = int_objs.size();
                for( unsigned i = 0; i < Ni; ++i )
                {
                  if( ! int_objs[i]->data_.is_set( tracking_keys::used_in_track ) )
                  {
                    vnl_vector_fixed<double,3> const& loc = int_objs[i]->world_loc_;
                    bins[f-1].add_point( vnl_double_2( loc[0], loc[1] ),
                                         int_objs[i] );
                  }
                }
              }
            }
            else
            {
              brut_force.resize(delta_-1);
              for( unsigned f = 1; f < delta_; ++f )
              {
                // Intermediate objects.
                vcl_vector<image_object_sptr> const& int_objs = mod_buf_->datum_at( f );
                unsigned const Ni = int_objs.size();
                brut_force[f-1].clear();
                for( unsigned i = 0; i < Ni; ++i )
                {
                  if( ! int_objs[i]->data_.is_set( tracking_keys::used_in_track ) )
                  {
                    brut_force[f-1].push_back(int_objs[i]);
                  }
                }
              }
            }
#endif
            add_new_state( trk,
              best_obj,
              timestamp_buf_->datum_at( f ) );
          }//if( best_obj )
      else
      {
        vcl_cout<<"some intermediate object is rejected. look into it\n";
      }
        }// for f

        add_new_state( trk, tgt_objs[tidx], tgt_time );

        // Update the velcotiy field in the new track state.

        // World velocity
        bool result = false;
        switch( method_ )
        {
          case USE_N_BACK_END_VEL:
            initialize_velocity_const_last_detection(trk);
            break;
          case USE_ROBUST_SPEED_FIT:
            result = initialize_velocity_robust_const_acceleration( trk, init_look_back_, init_bt_norm_ris_val_ );
            break;
          case USE_ROBUST_POINTS_FIT:
            result = initialize_velocity_robust_point( trk, init_bt_norm_ris_val_ );
          default:
            log_error( "Unknown vel init method\n" );
        };

        if(!result)
        {
          initialize_velocity_const_last_detection(trk);
        }

        // Image velocity
        vcl_vector< track_state_sptr > const& hist = trk->history();
        int buf_len = wld2img_H_buf_->size();
        int track_len = hist.size();
        int buf_idx, track_hist_idx;
        // Note: hist(track_len-1) corresponds to the current frame
        // For both timestamp and homog buffers, index 0 corresponds
        // to current frame. We count from current frame and backward
        // to earlier frames
        for(buf_idx = 0, track_hist_idx = track_len-1;
            buf_idx < buf_len && track_hist_idx >= 0; buf_idx++)
        {
          if(timestamp_buf_->has_datum_at( buf_idx )
             && wld2img_H_buf_->has_datum_at( buf_idx ))
          {
            if( hist[track_hist_idx]->time_
                == timestamp_buf_->datum_at(buf_idx) )
            {
              hist[track_hist_idx]->add_img_vel( wld2img_H_buf_->datum_at(buf_idx) );
              track_hist_idx--;
            }
          }
          else
          {
            log_error(this->name()<<"Fail to find timestamp or wld2img "
                                  <<"homography at expected location\n");
          }
        }
        if(track_hist_idx >= 0)
        {
          log_error(this->name()<<"Not all initial states "
                                <<"were assigned image velocity\n");
        }

        // Update the last_mod_match field in track.
        if( tgt_time.has_time() )
        {
          trk->set_last_mod_match( tgt_time.time_in_secs() );
        }
        else
        {
          log_warning( this->name() << ": Last MOD flag could not be set on "
            << "the new track, because the vidtk::timestamp does not contain "
            << " \"time\".\n" );
        }

        tracks_.push_back( trk );
#ifdef TEST_INIT
        trk->set_id( next_track_id_++ );
#else
        trk->set_id( tracks_.size() );
#endif
      }
    }
  }

  log_debug( "Found " << tracks_.size() << " possible new tracks\n" );
  log_debug( "Done with track initialization." << vcl_endl );

  return true;
}

void
track_initializer_process
::set_mod_buffer( buffer< vcl_vector<image_object_sptr> >  const& buf )
{
  mod_buf_ = &buf;
}


void
track_initializer_process
::set_timestamp_buffer( buffer<timestamp> const& buf )
{
  timestamp_buf_ = &buf;
}

void
track_initializer_process
::set_mf_params( multiple_features const& mf )
{
  mf_params_ = &mf;
}

vcl_vector<track_sptr> const&
track_initializer_process
::new_tracks() const
{
  return tracks_;
}


/// Insert a new state into \a trk at time \a t for the object \a
/// obj, with velocity \a vel.
void
track_initializer_process
::add_new_state( track_sptr& trk,
                 image_object_sptr const& obj,
                 timestamp const& t )
{
  track_state_sptr s = new track_state;
  s->loc_ = obj->world_loc_;
  s->time_ = t;

  s->data_.set( tracking_keys::img_objs,
                vcl_vector< image_object_sptr >( 1, obj ) );

  trk->add_state( s );
}


/// Output is \a best_cost and \a best_idx.  \a best_cost is bounded
/// above by the parameter max_dist_.  \a best_idx is set to
/// <tt>unsigned(-1)</tt> if none of the objects in the frame at
/// offset \a offset are closer than max_dist_.
void
track_initializer_process
::closest_object( image_object::float3d_type const& src_loc,
                  image_object::float3d_type const& tgt_loc,
                  double src_time,
                  double full_interval,
                  unsigned offset,
                  vcl_vector<bin_type> const& bins,
                  vcl_vector< vcl_vector< image_object_sptr > > const & brut_force,
                  double& best_cost_out,
                  image_object_sptr& best_obj_out ) const
{
  double const int_interval = timestamp_buf_->datum_at( offset ).time_in_secs() - src_time;

  // Expected location of the object in frame f.
  image_object::float_type frac = int_interval / full_interval;
  image_object::float3d_type loc
    = src_loc * (1-frac) + tgt_loc * frac;

  // The unit vector for the src->tgt line
  vnl_double_3 line_dir = tgt_loc - src_loc;
  line_dir.normalize();

  // Objects present in the intemediate frame that could match the
  // expected position.  We only need the objects within max_dist_
  // from the expected location.
  vcl_vector<image_object_sptr> int_objs;

  bool no_bins = bins.empty();

  if( no_bins && brut_force.size()==0)
  {
    log_error("Need to have brut force or query bins");
  }
  else if( no_bins )
  {
    int_objs = brut_force[ offset-1 ];
  }
  else
  {
    bins[offset-1].points_within_radius( vnl_double_2( loc[0], loc[1] ),
                                         max_query_radius_,
                                         int_objs );
  }


  // Measure support as the distance to the closest object in
  // frame f, with a cut-off at max_dist_.
  double max_sqr = max_query_radius_ * max_query_radius_;
  double best_cost_sqr = kinematics_sigma_gate_ * kinematics_sigma_gate_;
  unsigned best_idx = unsigned(-1);
  unsigned const Ni = int_objs.size();

#ifdef DEBUG
  vcl_cout << "\n\nsrc loc = " << src_loc << "\n"
           << "tgt loc = " << tgt_loc << "\n"
           << "line dir = " << line_dir << "\n"
           << "exp loc = " << loc << "\n";
#endif

  for( unsigned i = 0; i < Ni; ++i )
  {
    //if( ! int_objs[i]->data_.is_set( tracking_keys::used_in_track ) )
    //{
      // There are two components to the difference vector.  One is
      // along the line, and the other perpendicular to the line.
      // Variations in the former imply changes in acceleration, could
      // still have a linear motion.  Variations in the latter
      // indicate non-linear motion.  Often, the latter is more
      // important, and more constrained, that the former.  Here, we
      // compute a Mahanalobis distance in the space of (tangential
      // motion, normal motion).
      vnl_double_3 del = loc - int_objs[i]->world_loc_;
      if( no_bins && del.squared_magnitude()>max_sqr )
      {
        continue;
      }
      double tang_dist = dot_product( del, line_dir );
      vnl_double_3 norm_vec = del - tang_dist * line_dir;
      double tang_dist_sqr = tang_dist * tang_dist;
      double norm_dist_sqr = norm_vec.squared_magnitude();

      double d = tang_dist_sqr * one_over_tang_sigma_sqr_
                 + norm_dist_sqr * one_over_norm_sigma_sqr_;

#ifdef DEBUG
      vcl_cout << "  dist to " << int_objs[i]->world_loc_ << "\n";
      vcl_cout << "          td^2 = " << tang_dist_sqr << "\n";
      vcl_cout << "          nd^2 = " << norm_dist_sqr << "\n";
      vcl_cout << "           d^2 = " << tang_dist_sqr+norm_dist_sqr << "\n";
      vcl_cout << "     mahan d^2 = " << d << "\n";
      vcl_cout << "     max dist^2= " << max_dist_ * max_dist_ << vcl_endl;
      double oldd = (loc - int_objs[i]->world_loc_).squared_magnitude();
      vcl_cout << "  (old) dist sqr to " << int_objs[i]->world_loc_ << " = " << oldd << "\n";
#endif
      if( d < best_cost_sqr )
      {
        best_cost_sqr = d;
        best_idx = i;
      }
    }

  best_cost_out = vcl_sqrt( best_cost_sqr );
  if( best_idx != unsigned(-1) )
  {
    best_obj_out = int_objs[best_idx];
  }
  else
  {
    best_obj_out = image_object_sptr();
  }
}

//A variant of 'closest_object()' which uses multiple features (color, area
//and kinematics) to find a better similarity between objects. It's a linear
//combination of the three similarity scores which have already been
//equalized through the inverse CDF of their distributions.

void
track_initializer_process
::closest_object2( image_object const& src_obj,
                  image_object const& tgt_obj,
                  double src_time,
                  double full_interval,
                  unsigned offset,
                  vcl_vector<bin_type> const& bins,
                  double& best_cost_out,
                  image_object_sptr& best_obj_out )
{
  double const int_interval = timestamp_buf_->datum_at( offset ).time_in_secs() - src_time;

  // Expected location of the object in frame f.
  image_object::float_type frac = int_interval / full_interval;

  curr_loc_ = src_obj.world_loc_ * (1-frac)
            + tgt_obj.world_loc_ * frac;

  // The unit vector for the src->tgt line
  line_dir_ = tgt_obj.world_loc_ - src_obj.world_loc_;
  line_dir_.normalize();

  // Objects present in the intemediate frame that could match the
  // expected position.  We only need the objects within max_dist_
  // from the expected location.
  vcl_vector<image_object_sptr> int_objs;

  if( bins.empty() )
  {
    vcl_vector<image_object_sptr> all_objs;

    all_objs = mod_buf_->datum_at( offset ); //copy ref
    int_objs = all_objs;//"deep" copy

    vcl_vector<image_object_sptr>::iterator end;
    end = vcl_remove_if( int_objs.begin(), int_objs.end(),
      boost::bind( &track_initializer_process::find_close_objs, this, _1) );
    //TODO: make curr_loc_ a function argument inside bind()

    int_objs.erase( end, int_objs.end() );
  }
  else
  {
    bins[offset-1].points_within_radius( vnl_double_2( curr_loc_[0], curr_loc_[1] ),
                                         kinematics_sigma_gate_,
                                         int_objs );
  }
  // Measure support as the distance to the closest object in
  // frame f, with a cut-off at max_dist_.
  //double best_cost_sqr = max_dist_ * max_dist_;
  double best_cost = vcl_numeric_limits<double>::infinity();
  unsigned best_idx = unsigned(-1);
  unsigned const Ni = int_objs.size();

#ifdef DEBUG
  vcl_cout << "\n\nsrc loc = " << src_obj.world_loc_ << "\n"
           << "tgt loc = " << tgt_obj.world_loc_ << "\n"
           << "line dir = " << line_dir << "\n"
           << "exp loc = " << loc << "\n";
#endif

  double p1, p2, x, xdx,  kinematics_log_prob;
  double sim1, sim2, color_similarity, area_similarity, kinematics_similarity;
  static double const two_pi = 2 * vnl_math::pi;
  static double const sqrt_two_pi = vcl_sqrt( two_pi );

  for( unsigned i = 0; i < Ni; ++i )
  {
    if( ! int_objs[i]->data_.is_set( tracking_keys::used_in_track ) )
    {

      //P_color
      image_histogram<vxl_byte, bool> hist1;
      image_histogram<vxl_byte, bool> hist2;
      if( ! int_objs[i]->data_.get( tracking_keys::histogram, hist1 ) ||
          ! src_obj.data_.get( tracking_keys::histogram, hist2 ) )
      {
        color_similarity = 0.0;
      }
      else
      {
        p1 = hist1.compare( hist2.get_h() );
        sim1 = mf_params_->get_similarity_score( p1, VIDTK_COLOR );
        if( ! tgt_obj.data_.get( tracking_keys::histogram, hist2 ) )
        {
          color_similarity = sim1;
        }
        else
        {
          p2 = hist1.compare( hist2.get_h() );
          sim2 = mf_params_->get_similarity_score( p2, VIDTK_COLOR );
          color_similarity = sim1 * (1-frac) + sim2 * frac;  // interpolation
        }
        // color similarity below threshold or not
        color_similarity = (color_similarity > min_color_similarity_) ?
                            color_similarity : -0.1;
      }

      //P_kinematics
      vnl_double_3 del = curr_loc_ - int_objs[i]->world_loc_;
      double tang_dist = dot_product( del, line_dir_ );
      vnl_double_3 norm_vec = del - tang_dist * line_dir_;
      double norm_dist = norm_vec.magnitude();
      vnl_double_2 delta( tang_dist, norm_dist );
      if(spatial_cov_(0,0) <= 0 || spatial_cov_(1,1) <= 0)
      {
         spatial_cov_(0,0) = 0.4 * 0.4;
         spatial_cov_(1,1) = 0.4 * 0.4;
      }
      double mahalanobis_dist_sqr = delta[0] * delta[0] / spatial_cov_(0,0)
                                  + delta[1] * delta[1] / spatial_cov_(1,1);
      // This is normalized probability density (pd) by the maximum pd. It is in (0,1]
      kinematics_similarity = vcl_exp( -0.5 * mahalanobis_dist_sqr );


      //P_area
      //compare with source obj
      double area_diff_src = vcl_max( int_objs[i]->area_ , src_obj.area_ ) /
                             vcl_min( int_objs[i]->area_ , src_obj.area_ ) - 1.0;
      //compare with target obj
      double area_diff_tgt = vcl_max( int_objs[i]->area_ , tgt_obj.area_ ) /
                             vcl_max( int_objs[i]->area_ , tgt_obj.area_ ) - 1.0;
      //final area similarity
      double area_diff = area_diff_src * (1-frac) + area_diff_tgt * frac; // interpolation
      area_similarity = vcl_exp( - area_diff );
      // area similarity below threshold or not
      area_similarity = (area_similarity > min_area_similarity_ ) ?
                         area_similarity : -0.1;



      double final_distance;
      double cumm_similarity;
      // either color or area similarity is below threshold
      if(color_similarity < 0 || area_similarity < 0 )
      {
        final_distance = vcl_numeric_limits<double>::infinity();
      }
      else
      {
        // combine three features
        double cumm_similarity = kinematics_similarity * mf_params_->w_kinematics
          + color_similarity * mf_params_->w_color
          + area_similarity * mf_params_->w_area;

        assert( cumm_similarity >=0 && cumm_similarity <= 1 );

        //Not using log because indiviudal histograms have already been 'equalized'.
        final_distance = 1 - cumm_similarity;
      }

#ifdef DEBUG

#endif

      if( final_distance < best_cost )
      {
        best_cost = final_distance;
        best_idx = i;
      }
    }
  }

  best_cost_out = best_cost;
  if( best_idx != unsigned(-1) )
  {
    best_obj_out = int_objs[best_idx];
  }
  else
  {
    best_obj_out = image_object_sptr();
  }
}

bool
track_initializer_process
::find_close_objs( image_object_sptr const& obj) const
{
  vnl_double_3 del = curr_loc_ - obj->world_loc_;
  double tang_dist = dot_product( del, line_dir_ );
  vnl_double_3 norm_vec = del - tang_dist * line_dir_;
  double tang_dist_sqr = tang_dist * tang_dist;
  double norm_dist_sqr = norm_vec.squared_magnitude();

  bool decision = (tang_dist_sqr/spatial_cov_(0,0)
                   + norm_dist_sqr/spatial_cov_(1,1) )
                   > kinematics_sigma_gate_*kinematics_sigma_gate_;

#ifdef DEBUG
  vcl_cout<< "Close obj: " << del <<", decision = "<< decision <<vcl_endl;
#endif
  return decision;
}

void
track_initializer_process
::set_wld2img_homog_buffer( buffer< vgl_h_matrix_2d<double> > const& buf )
{
  wld2img_H_buf_ = &buf;
}

} // end namespace vidtk
