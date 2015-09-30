/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_da_so_tracker_kalman_extended_h_
#define vidtk_da_so_tracker_kalman_extended_h_

#include <tracking/da_so_tracker.h>

#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>

#include <utilities/config_block.h>

#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>

namespace vidtk
{


/// Data-association-based single object tracker using an extended kalman filter.
/// This is an approximation of non-linear functions
template< class function >
class da_so_tracker_kalman_extended
  : public da_so_tracker
{
  public:

    struct const_parameters : public vbl_ref_count
    {
      const_parameters(){}
      ~const_parameters(){}

      vnl_matrix<double> init_cov_;
      /// Process noise Q
      vnl_matrix<double> process_cov_;

      static void add_parameters_defaults(config_block & blk);
      void set_parameters(config_block const& blk);
    };

    typedef vbl_smart_ptr< const_parameters > const_parameters_sptr;

    da_so_tracker_kalman_extended( function fun,
                                   const_parameters_sptr kp = NULL);
    ~da_so_tracker_kalman_extended();

    void set_state( vnl_vector<double> state,
                    vnl_matrix<double> covar,
                    double time );

    void set_parameters(const_parameters_sptr kp)
    {
      params_ = kp;
    }

    vnl_vector<double> const& state() const;

    virtual vnl_vector<double> get_current_state() const
    {
      return state();
    }

    vnl_matrix<double> const& state_covar() const;

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

    virtual void predict( double const& pred_ts_,
                          vnl_vector<double>& state,
                          vnl_matrix<double>& covar ) const;

    virtual vnl_double_4x4 get_location_velocity_covar() const;

    virtual da_so_tracker_sptr clone() const;

  protected:
    virtual void update_impl( double const& ts,
                              vnl_vector_fixed<double,2> const& loc,
                              vnl_matrix_fixed<double,2,2> const& cov );
    /// The (non)linear function use to calculate state and covariance
    function fun_;

    /// This is a pointer to parameters that should never change.
    const_parameters_sptr params_;
};


} // end namespace vidtk


#endif
