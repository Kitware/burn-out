/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_greedy_assignment_h_
#define vidtk_greedy_assignment_h_

#include <vcl_vector.h>
#include <vnl/vnl_matrix.h>

namespace vidtk
{


/// A greedy algorithm for bi-partite matching.
///
/// The MxN cost matrix \a cost captures the pairwise costs of
/// assigning one of M "sources" to one of "N" targets, with \a
/// cost(i,j) being the cost of assigning source i to target j.
///
/// The output is a vector v of length M, with v(i)=j if, in the
/// solution, source i should be assigned to target j, and
/// v(i)=unsigned(-1) if source i should not be assigned to any
/// target.
///
/// The solution is not guaranteed to be optimal, but the runtime cost
/// is O( MN log MN ).
///
/// \sa vnl_hungarian_algorithm
///
vcl_vector< unsigned >
greedy_assignment( vnl_matrix<double> const& cost );


} // end namespace vidtk


#endif // vidtk_greedy_assignment_h_
