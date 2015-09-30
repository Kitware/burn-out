/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_da_so_tracker_kalman_h_
#define vidtk_da_so_tracker_kalman_h_

#include <tracking/da_so_tracker.h>

#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>

#include <utilities/config_block.h>

namespace vidtk
{


/// Data-association-based single object tracker using a kalman filter.
class da_so_tracker_kalman
  : public da_so_tracker
{
  public:

    struct const_parameters : public vbl_ref_count
    {
      vnl_double_4x4 init_cov_;
      /// Process noise Q
      vnl_double_4x4 process_cov_;

      /// Do we want fixed (user defined) or variable (user defined + velocity
      /// derived ) process noise Q_
      bool use_fixed_process_noise_;
      vnl_double_2 process_noise_speed_scale_;

      static void add_parameters_defaults(config_block & blk);
      void set_parameters(config_block const& blk);

      double init_dw_;
      double init_dh_;

      const_parameters();
    };

    typedef vbl_smart_ptr< const_parameters > const_parameters_sptr;

    da_so_tracker_kalman(const_parameters_sptr kp = NULL);
    ~da_so_tracker_kalman();

    void set_state( vnl_vector_fixed<double,4> state,
                    vnl_matrix_fixed<double,4,4> covar,
                    double time );

    void set_parameters(const_parameters_sptr kp)
    {
      params_ = kp;
    }

    vnl_vector_fixed<double,4> const& state() const;

    vnl_matrix_fixed<double,4,4> const& state_covar() const;

    virtual void get_current_location( vnl_double_2 & ) const;
    virtual void get_current_velocity( vnl_double_2 & ) const;
    virtual void get_current_location( vnl_vector_fixed< double, 3 > & ) const;
    virtual void get_current_velocity( vnl_vector_fixed< double, 3 > & ) const;
    virtual void get_current_location_covar( vnl_double_2x2 & ) const;
    virtual void get_current_width( double & ) const{};
    virtual void get_current_height( double & ) const{};
    virtual void get_current_width_dot( double & ) const{};
    virtual void get_current_height_dot( double & ) const{};
    virtual void get_current_wh( vnl_double_2 & ) const{};
    virtual void get_current_wh_dot( vnl_double_2 & ) const{};
    virtual void get_current_wh_covar( vnl_double_2x2 & ) const{};

    virtual void initialize( track const & init_track );

    virtual void predict( double const& pred_ts_,
                        vnl_vector_fixed<double,2>& pred_pos,
                        vnl_matrix_fixed<double,2,2>& pred_cov ) const;

    virtual void predict( double const& pred_ts_,
                          vnl_vector_fixed<double,4>& state,
                          vnl_matrix_fixed<double,4,4>& covar ) const;

    virtual vnl_double_4x4 get_location_velocity_covar() const
    { return state_covar(); }

    virtual vnl_vector<double> get_current_state() const
    {
      vnl_vector<double> r = state();
      return r;
    }

    virtual da_so_tracker_sptr clone() const;

  protected:
    virtual void update_impl( double const& ts,
                              vnl_vector_fixed<double,2> const& loc,
                              vnl_matrix_fixed<double,2,2> const& cov );
    vnl_double_4 x_;
    vnl_double_4x4 P_;
    /// State transition.  This is not const because some parameters are
    /// really "t" and not "1", and need to be modified each time.
    mutable vnl_double_4x4 F_;

    /// Observation matrix
    vnl_matrix_fixed<double,2,4> H_;

    /// This is a pointer to parameters that should never change.
    const_parameters_sptr params_;

    /// Names for each of the elements in the state vector
    struct state_elements
    { enum {loc_x=0,loc_y=1,vel_x=2,vel_y=3}; };
};


} // end namespace vidtk


#endif
