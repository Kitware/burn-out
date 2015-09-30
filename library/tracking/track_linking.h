/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_linking_h_
#define vidtk_track_linking_h_

#include <vcl_vector.h>

#include <vnl/vnl_matrix.h>

#include <tracking/track.h>

namespace vidtk
{

struct track_linking_result
{
  enum link_category { 
    ASSIGNED = 0,
    TERMINATED = 1,
    INFEASIBLE = 2
  }; 

  track_linking_result( unsigned int src,
                        unsigned int dst,
                        double incost,
                        link_category ltype ) :
    src_track_id( src ),
    dst_track_id( dst ),
    cost( incost ),
    link_type( ltype ) 
  { }

  static vcl_string enum_names[]; 

  unsigned int    src_track_id;
  unsigned int    dst_track_id;
  double          cost;
  link_category   link_type;
  bool is_assigned() const
  { return link_type == ASSIGNED; }
};

vcl_ostream& operator<< (vcl_ostream &stream, track_linking_result const& link_res);

template < class TTrackMatchFunctor >
class track_linking 
{
public:

  // Typedef for a track
  typedef track       TrackType;
  typedef track_sptr  TrackTypePointer;

  // Typedef for matching function
  typedef TTrackMatchFunctor MatchFunctorType;

  // Constructor
  track_linking( TTrackMatchFunctor matchFunc );
  track_linking(){}

  // Virtual destructor
  virtual ~track_linking();

  // Set the function used to evaluate track matching cost.
  void set_track_match_functor( TTrackMatchFunctor const & matchFunc );

  // Link the tracks
  vcl_vector< track_linking_result > link( vcl_vector< TrackTypePointer >& tracks );
  vcl_vector< track_linking_result > link( vcl_vector< TrackTypePointer >& tracks,
                                           vcl_vector<bool> const & can_be_col );
  vcl_vector< track_linking_result > link( vcl_vector< TrackTypePointer >& rows,
                                           vcl_vector< TrackTypePointer >& cols );

  // Set the cost used to represent track termination.
  void set_track_termination_cost( double c );

  // Get the cost used to represent track termination.
  double get_track_termination_cost();

  // Types of available assignment algorithms.
  enum assignment_alg_type { GREEDY, HUNGARIAN };
  
  // Set the type of assignment algorithm.
  void set_assignment_algorithm_type( assignment_alg_type alg );
  bool set_assignment_algorithm_type( vcl_string & alg );

  // Get the type of assignment algorithm.
  assignment_alg_type get_assignment_algorithm_type() {
    return assignment_algorithm_;
  }

  // Get the name of the assignment algorithm
  vcl_string const get_assignment_algorithm_name();

  // Make tracks from linking results.
  void build_tracks( vcl_vector< TrackTypePointer >& tracks, 
                     vcl_vector< track_linking_result >& links,
                     vcl_vector< TrackTypePointer >& result );

protected:

  // Class to compare tracks by time.
  class track_time_compare_type {

  public:
    bool operator() ( TrackTypePointer const & t1, TrackTypePointer const & t2 ) {
      return t1->last_state()->time_ < t2->last_state()->time_;
    }

  } the_comparer_;

  // Virtual function so that it can be reimplemented in a subclass.
  // This would allow computation of constraints potentially not 
  // available through the MatchFunctor. These would involve conditional
  // constraints across tracks.
  virtual void compute_cost_matrix( vcl_vector< TrackTypePointer >& rows,
                                    vcl_vector< TrackTypePointer >& cols,
                                    vnl_vector< double >& min_row_value,
                                    vnl_matrix<double>& cost_matrix );

  void print_cost_matrix( vnl_matrix<double>& cost_matrix,
                          vcl_vector< unsigned int > & mapping,
                          vcl_vector< TrackTypePointer >& tracks );

  // Function that computes the cost of associating one track with another.
  MatchFunctorType match_functor_;

  // Cost for terminating a track.
  double track_termination_cost_;

  // Defines the algorithm type.
  assignment_alg_type assignment_algorithm_;

}; // class track_linking


} // vidtk

#endif // #ifndef vidtk_track_linking_h_

