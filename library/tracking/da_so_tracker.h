/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_da_so_tracker_h_
#define vidtk_da_so_tracker_h_

#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>
#include <vnl/vnl_double_4.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_4x4.h>
#include <vnl/vnl_double_2x2.h>

#include <utilities/timestamp.h>

#include <tracking/image_object.h>
#include <tracking/track.h>

namespace vidtk
{

class da_so_tracker;
typedef vbl_smart_ptr< da_so_tracker > da_so_tracker_sptr;

/// Data-association-based single object tracker.
class da_so_tracker
  : public vbl_ref_count
{
public:
  da_so_tracker();

  virtual void predict( double const& pred_ts_,
                        vnl_vector_fixed<double,2>& pred_pos,
                        vnl_matrix_fixed<double,2,2>& pred_cov ) const = 0;

  void update();
  void update( double const& ts,
               vnl_vector_fixed<double,2> const& loc,
               vnl_matrix_fixed<double,2,2> const& cov );
  void update( timestamp const& ts, image_object & obj,
               vnl_matrix_fixed<double,2,2> const& cov );

  void update_wh( timestamp const& ts, image_object & obj,
                  vnl_matrix_fixed<double,2,2> const& cov );

  virtual vnl_vector<double> get_current_state() const = 0;

  /// Number of consecutive no-measurement updates since the last
  /// update with a measurement.
  unsigned missed_count() const;

  /// The time of the last update with a measurement.
  double last_update_time() const;

  virtual void initialize( track const & init_track ) = 0;

  track_sptr get_track() const;

  virtual void get_current_location( vnl_double_2 & ) const = 0;
  vnl_double_2 get_current_location() const;
  virtual void get_current_velocity( vnl_double_2 & ) const = 0;
  vnl_double_2 get_current_velocity() const;
  virtual void get_current_location( vnl_vector_fixed< double, 3 > & ) const = 0;
  virtual void get_current_velocity( vnl_vector_fixed< double, 3 > & ) const = 0;

  virtual void get_current_location_covar( vnl_double_2x2 & ) const = 0;
  vnl_double_2x2 get_current_location_covar( ) const;

  virtual vnl_double_4x4 get_location_velocity_covar() const = 0;

 virtual void get_current_width( double & ) const = 0;
 double get_current_width() const;

 virtual void get_current_height( double & ) const = 0;
 double get_current_height() const;

  virtual da_so_tracker_sptr clone() const = 0;

protected:

  virtual void update_impl( double const& ts,
                            vnl_vector_fixed<double,2> const& loc,
                            vnl_matrix_fixed<double,2,2> const& cov ) = 0;

  /// The time at which the state was updated with a measurement.
  double last_time_;

  /// The number of consecutive "updates" without a measurement.
  unsigned missed_count_;

  /// Link to the track assocated to this tracker
  track_sptr track_;
};

// Utility functors

// return true if the track id matches.
struct da_so_tracker_id_pred
{
  da_so_tracker_id_pred(unsigned id)
  {
    id_ = id;
  }

  bool operator()(da_so_tracker_sptr const& t)
  {
    return t->get_track()->id() == id_;
  }

  unsigned id_;
};

struct da_so_tracker_clone_functor{
  da_so_tracker_sptr operator()(da_so_tracker_sptr const &x) const
  {
    return x->clone();
  }
};


} // end namespace vidtk


#endif // vidtk_da_so_tracker_h_
