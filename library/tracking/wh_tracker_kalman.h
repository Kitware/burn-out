/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_wh_tracker_kalman_h_
#define vidtk_wh_tracker_kalman_h_

#include <tracking/da_so_tracker.h>
#include <tracking/da_so_tracker_kalman.h>
#include <tracking/image_object.h>

#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>

#include <utilities/config_block.h>

namespace vidtk
{


/// Single object tracker for the width and height of bbox using a kalman filter.
class wh_tracker_kalman
  : public da_so_tracker_kalman
{
  public:

    struct const_parameters : public da_so_tracker_kalman::const_parameters
    {
      // placeholder for any future wh_tracker specific parameters
    };

    wh_tracker_kalman(const_parameters_sptr kp = NULL);
    ~wh_tracker_kalman();

    virtual void get_current_width( double & ) const;
    virtual void get_current_height( double & ) const;
    virtual void get_current_width_dot( double & ) const;
    virtual void get_current_height_dot( double & ) const;
    virtual void get_current_wh( vnl_double_2 & ) const;
    virtual void get_current_wh_dot( vnl_double_2 & ) const;
    virtual void get_current_wh_covar( vnl_double_2x2 & ) const;
    virtual void initialize( track const &init_track );
    virtual vnl_double_4x4 get_wh_whdot_covar() const
    { return state_covar(); }

    virtual vnl_vector<double> get_current_state() const
    {
      vnl_vector<double> r = state();
      return r;
    }

    virtual da_so_tracker_sptr clone() const;

  protected:

    /// Names for each of the elements in the state vector
    struct state_elements
    { enum {width=0,height=1,width_dot=2,height_dot=3}; };

    double wh_init_dw_;
    double wh_init_dh_;


};


} // end namespace vidtk


#endif
