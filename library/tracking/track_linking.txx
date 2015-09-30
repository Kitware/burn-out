/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_linking_txx_
#define track_linking_txx_
#include <limits>

#include "track_linking.h"
#include <vnl/vnl_hungarian_algorithm.h>
#include <utilities/greedy_assignment.h>

#include <vcl_algorithm.h>
#include <vul/vul_string.h>

#include <vcl_fstream.h>
#include <vcl_iostream.h>

namespace vidtk 
{

template <class TTrackMatchFunctor>
track_linking< TTrackMatchFunctor >
::track_linking( TTrackMatchFunctor matchFunc ) : 
  track_termination_cost_( 1000.0 ),
  assignment_algorithm_( HUNGARIAN )
{
  // Assignment operator
  match_functor_ = matchFunc;
}

template <class TTrackMatchFunctor>
track_linking< TTrackMatchFunctor >
::~track_linking()
{

}

template <class TTrackMatchFunctor>
void
track_linking< TTrackMatchFunctor >
::set_track_termination_cost( double c )
{
  track_termination_cost_ = c;
}

template <class TTrackMatchFunctor>
double
track_linking< TTrackMatchFunctor >
::get_track_termination_cost( )
{
  return track_termination_cost_;
}

template <class TTrackMatchFunctor>
void 
track_linking< TTrackMatchFunctor >
::compute_cost_matrix( vcl_vector< TrackTypePointer >& rows,
                       vcl_vector< TrackTypePointer >& cols,
                       vnl_vector< double >& min_row_value, 
                       vnl_matrix<double>& cost_matrix )
{

  unsigned int num_rows_tracks = rows.size();
  unsigned int num_cols_tracks = cols.size();
  assert(cost_matrix.rows() == num_rows_tracks);
  assert(cost_matrix.cols() == num_cols_tracks+num_rows_tracks);
  assert(min_row_value.size() == cost_matrix.rows());

  double impossible_match_cost = match_functor_.get_impossible_match_cost();

  // Compute the cost matrix, not necessarily symmetric,
  // so loop over all indices.
  for ( unsigned int i = 0; i < num_rows_tracks; i++ ) {
    for ( unsigned int j = 0; j < num_cols_tracks; j++ ) {
      if ( rows[i] == cols[j] ) {
        cost_matrix( i, j ) = impossible_match_cost; //track_termination_cost_;
      } else {
        // Call the matching function
        cost_matrix( i, j ) = match_functor_( rows[i], cols[j] );
        if( cost_matrix( i, j ) < min_row_value[i] )
        {
          min_row_value[i] = cost_matrix( i, j );
        }
      }
    } // j
  } // i

//   unsigned int num_cols = cost_matrix.cols();
  // Add termination costs
  for ( unsigned int i = 0; i < num_rows_tracks; i++ ) 
  {
    cost_matrix( i, i+num_cols_tracks ) = track_termination_cost_;
  }
}

template <class TTrackMatchFunctor>
vcl_vector< track_linking_result >
track_linking< TTrackMatchFunctor >
::link( vcl_vector< TrackTypePointer >& tracks )
{
  return this->link(tracks, tracks);
}

template <class TTrackMatchFunctor>
vcl_vector< track_linking_result >
track_linking< TTrackMatchFunctor >
::link( vcl_vector< TrackTypePointer >& tracks,
        vcl_vector<bool> const & can_be_prev )
{
  vcl_vector< TrackTypePointer > cols;
  for(unsigned int i = 0; i < tracks.size(); ++i)
  {
    if(can_be_prev[i])
    {
      cols.push_back(tracks[i]);
    }
  }

  return this->link(tracks,cols);

}

template <class TTrackMatchFunctor>
vcl_vector< track_linking_result >
track_linking< TTrackMatchFunctor >
::link( vcl_vector< TrackTypePointer >& rows,
        vcl_vector< TrackTypePointer >& cols )
{
    // How many tracks?
  unsigned int num_row_tracks = rows.size();
  unsigned int num_col_tracks = cols.size();

  double impossible_match_cost = match_functor_.get_impossible_match_cost();

  vcl_cout << vcl_endl;
  vcl_cout << "Computing cost matrix ";
  // Compute the cost matrix
  vnl_vector<double> min_costs(num_row_tracks,track_termination_cost_);
  vnl_matrix<double> temp_cost_matrix( num_row_tracks,
                                       num_row_tracks+num_col_tracks,
                                       impossible_match_cost );

  this->compute_cost_matrix( rows, cols, min_costs, temp_cost_matrix );
  // Make the cost matrix
  unsigned int number_non_terms = 0;
  vcl_vector< unsigned int > track_to_row(num_row_tracks, num_row_tracks+1);
  for( unsigned int i = 0; i < num_row_tracks; ++i )
  {
    if(min_costs[i] < track_termination_cost_)
    {
      track_to_row[i] = number_non_terms;
      number_non_terms++;
    }
  }
  vnl_matrix<double> cost_matrix( number_non_terms,
                                  num_col_tracks+number_non_terms,
                                  impossible_match_cost );
  vcl_cout << " original (" << temp_cost_matrix.rows() << "x" << temp_cost_matrix.cols() << ")"
           << " updated  (" << cost_matrix.rows() << "x" << cost_matrix.cols() << ")...";
  for( unsigned int i = 0; i < num_row_tracks; ++i)
  {
    unsigned int at_i = track_to_row[i];
    if(at_i != num_row_tracks+1)
    {
      for( unsigned int j = 0; j < num_col_tracks; ++j )
      {
        cost_matrix(at_i,j) = temp_cost_matrix(i,j);
      }
      cost_matrix(at_i,num_col_tracks+at_i) = track_termination_cost_;
    }
  }
//   print_cost_matrix(cost_matrix,track_to_row,tracks);
  vcl_cout << "done." << vcl_endl;

  // this->print_cost_matrix( cost_matrix );

  // Check that the assignment operator copies values
  vcl_vector< unsigned int > assignments;

  vcl_cout << "Running assignment algorithm " << get_assignment_algorithm_name() << "...";

  if ( assignment_algorithm_ == HUNGARIAN ) {
    assignments = vnl_hungarian_algorithm<double>( cost_matrix );
  } else if ( assignment_algorithm_ == GREEDY ) {
    assignments = greedy_assignment( cost_matrix );
  } else {
    vcl_cerr << "Unknown assignment algorithm : " << assignment_algorithm_ << vcl_endl;
  }

  vcl_cout << "done." << vcl_endl;

  vcl_vector< track_linking_result > assignments_with_results;

  for ( unsigned int count = 0; count < num_row_tracks; count++ )
  {
    unsigned int at = track_to_row[count];
    if ( at != num_row_tracks+1 )
    {
      double temp_cost = cost_matrix( at, assignments[ at ] );

      track_linking_result::link_category link_type = track_linking_result::ASSIGNED;
      if ( temp_cost == track_termination_cost_ ) {
        link_type = track_linking_result::TERMINATED;
      } else if ( temp_cost == match_functor_.get_impossible_match_cost() ) {
        link_type = track_linking_result::INFEASIBLE;
      }

      unsigned int assigned_track_index = assignments[at];
      unsigned int assigned_track_id = static_cast<unsigned int>(-1);
      if ( assigned_track_index < cols.size() )
      {
        assigned_track_id = cols[ assigned_track_index ]->id();
      }
      assignments_with_results.push_back( track_linking_result( rows[ count ]->id(),
                                                                assigned_track_id,
                                                                temp_cost, link_type ) );
    }
    else
    {
      track_linking_result::link_category link_type = track_linking_result::TERMINATED;
      unsigned int assigned_track_id = static_cast<unsigned int>(-1);
      assignments_with_results.push_back( track_linking_result( rows[ count ]->id(),
                                                                assigned_track_id,
                                                                track_termination_cost_,
                                                                link_type ) );
    }
  }

//   vcl_ofstream out("links.csv");
//   for( unsigned int j = 0; j < assignments_with_results.size(); ++j)
//   {
//     out << assignments_with_results[j].src_track_id << "," << assignments_with_results[j].dst_track_id;
//     if(track_to_row[j]!= num_tracks+1)
//     {
//       out << "," << assignments[track_to_row[j]] << "," /*<< tracks[ assignments[track_to_row[j]] ]->id()*/ << "," << assignments.size();
//     }
//     else
//     {
//       out << "," << num_tracks+1 << ",000,000";
//     }
//     out << "," << assignments_with_results[j].cost << "," 
//         << assignments_with_results[j].enum_names[assignments_with_results[j].link_type] << vcl_endl;
//   }

  return assignments_with_results;

}

template <class TTrackMatchFunctor>
void
track_linking< TTrackMatchFunctor >
::print_cost_matrix( vnl_matrix<double>& cost_matrix,
                     vcl_vector< unsigned int > & mapping,
                     vcl_vector< TrackTypePointer >& tracks )
{
  vcl_ofstream out("cost_matrix.csv");
  for(unsigned int c = 0; c < tracks.size(); ++c)
  {
    out << "," << tracks[c]->id();
  }
  out << ",NULL" << vcl_endl;
//   vcl_cout << vcl_endl << vcl_endl << vcl_endl;
  unsigned int at = 0;
  for ( unsigned int r = 0; r < cost_matrix.rows(); r++ ) {
    while(mapping[at] == tracks.size()+1) at++;
    out << tracks[at]->id() << ",";
    for ( unsigned int c = 0; c < tracks.size(); c++ ) {
      out << cost_matrix( r, c ) << ",";
    }
    at++;
    out << track_termination_cost_ << vcl_endl;
  }
  out.close();
//   vcl_cout << vcl_endl << vcl_endl << vcl_endl;
}

template <class TTrackMatchFunctor>
void
track_linking< TTrackMatchFunctor >
::set_track_match_functor( TTrackMatchFunctor const & matchFunc )
{
  // Assignment operator.
  match_functor_ = matchFunc;
}


template <class TTrackMatchFunctor>
void
track_linking< TTrackMatchFunctor >
::set_assignment_algorithm_type( assignment_alg_type alg )
{
  assignment_algorithm_ = alg;
}

template <class TTrackMatchFunctor>
bool 
track_linking< TTrackMatchFunctor >
::set_assignment_algorithm_type( vcl_string & alg )
{
  vul_string_capitalize( alg );
  if(alg == "HUNGARIAN")
  {
    this->set_assignment_algorithm_type(HUNGARIAN);
    return true;
  }
  if(alg == "GREEDY")
  {
    this->set_assignment_algorithm_type(GREEDY);
    return true;
  }
  return false;
}


template <class TTrackMatchFunctor>
void
track_linking< TTrackMatchFunctor >
::build_tracks( vcl_vector< TrackTypePointer >& tracks, vcl_vector< track_linking_result >& links,
                vcl_vector< TrackTypePointer >& result )
{
  //Typedefs
  typedef vcl_vector< typename track_linking<TTrackMatchFunctor>::TrackTypePointer > ResultVectorType;
  typedef typename vcl_vector< TrackTypePointer >::iterator TrackTypePointerVectorIterator;
  typedef vcl_map< int, int >::iterator LinkMapIter;

  // Order tracks by time.
  track_time_compare_type track_time_compare;
  vcl_sort( tracks.begin(), tracks.end(), track_time_compare );

  // Walk through the track_linking_results to create a map from
  // src ids to dest ids
  vcl_map< int, int > link_map;

  // Keep a map of tracks which are linked to, so that
  // we do not write them out separately.
  vcl_map< int, bool > is_a_destination_map;

  for( vcl_vector< track_linking_result >::iterator liter = links.begin();
        liter != links.end();
        liter++ ) 
  {
    unsigned int dst_id = (*liter).dst_track_id;
    unsigned int src_id = (*liter).src_track_id;
    if ( liter->link_type == track_linking_result::ASSIGNED &&
         dst_id != static_cast<unsigned int>(-1) &&
         src_id != dst_id )
    {
      link_map[ static_cast<int>(src_id) ] = static_cast<int>(dst_id);
      is_a_destination_map[ static_cast<int>(dst_id) ] = true;
    }
  }

  // Walk through the input tracks to create a map from track id to track
  vcl_map< unsigned int, TrackTypePointer > track_map;
  for ( TrackTypePointerVectorIterator triter = tracks.begin();
        triter != tracks.end();
        triter++ ) 
  {
    track_map[ (*triter)->id() ] = *triter;
  }

  // Will contain the list of tracks to append.
  vcl_vector< unsigned int > append_list;

  // Walk through tracks in time order. 
  for( TrackTypePointerVectorIterator triter = tracks.begin();
       triter != tracks.end();
       triter++ )
  {

    unsigned int src_id = (*triter)->id();

    // If this track is linked to by another track, skip it
    if ( is_a_destination_map.find( static_cast<int>(src_id) ) != is_a_destination_map.end() ) 
    {
      vcl_cout << src_id << " is in the destination map." << vcl_endl;
      continue;
    }

    append_list.clear();

    // Find link for each track from map from src id to dst id
    LinkMapIter miter = link_map.find( static_cast<int>(src_id) );
    append_list.push_back( src_id );

    // Keep appending the destinations if there are links.
    while( miter != link_map.end() ) {

      unsigned int dest_id = static_cast<unsigned int>((*miter).second);
      // Remove src id from the map so we don't follow it later.
      link_map.erase( static_cast<int>(src_id) );

      append_list.push_back( dest_id );

      miter = link_map.find( static_cast<int>(dest_id) );
      src_id = dest_id;
    } // while

    // Append list contains all the track fragments that should be stitched together.
    if ( append_list.size() > 0 ) {
      // Make a copy of the track.
      //TrackTypePointer first_track = new TrackType( (*(track_map.find( append_list.front() ))).second );
      TrackTypePointer first_track = (*(track_map.find( append_list.front() ))).second ;
      vcl_cout << first_track->id() << " ";

      for( vcl_vector< unsigned int >::iterator aiter = (append_list.begin() + 1);
            aiter != append_list.end();
            aiter++ ) {
        vcl_cout << *aiter << " ";
        first_track->append_track( *(*(track_map.find( *aiter ))).second );
      }
      vcl_cout << vcl_endl;

      // Should make another copy of first_track. Ugh.
      result.push_back( first_track );
    } // if append_list

  } // for

}

template <class TTrackMatchFunctor>
vcl_string const 
track_linking< TTrackMatchFunctor >
::get_assignment_algorithm_name()
{
  switch ( assignment_algorithm_ )
  {
  case HUNGARIAN :
    return "HUNGARIAN";
  case GREEDY : 
    return "GREEDY";
  default :
    return "UNKNOWN";
  }
}

vcl_string track_linking_result::enum_names[] = { vcl_string("ASSIGNED"),
                                                  vcl_string("TERMINATED"),
                                                  vcl_string("INFEASIBLE") };


vcl_ostream& operator<<(vcl_ostream &stream, track_linking_result const& link_res)
{
  stream << link_res.src_track_id << " -> " << link_res.dst_track_id;
  stream << " " << track_linking_result::enum_names[ link_res.link_type ];
  stream << " ( " << link_res.cost << " )" << vcl_endl;

  return stream;
}

} // namespace vidtk

#endif // track_linking_txx_

