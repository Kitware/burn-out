/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "greedy_assignment.h"

#include <algorithm>

namespace
{

// This class provides the "less-than" ordering for indicies into the
// matrix based on the component value in the matrix.
struct cost_sorter
{
  vnl_matrix<double> const* cost_mat_;

  bool operator()( unsigned int const& idx1, unsigned int const& idx2 ) const
  {
    return this->cost_mat_->data_block()[idx1] < this->cost_mat_->data_block()[idx2];
  }

};


} // end anonymous namespace


namespace vidtk
{

std::vector< unsigned >
greedy_assignment( vnl_matrix<double> const& cost )
{
  // The implementation of the algorithm is pretty simple: sort the
  // cost values, and assign the cheapest one first, and then the next
  // cheapest that hasn't been assigned, and so on.  However, this
  // algorithm will likely be used when the problem size is very large
  // (e.g. 15000x15000 matrix).  At these problem sizes, memory usage
  // is critical.  For example, it takes ~1.7GB just to store the cost
  // matrix.  So, we want to make sure that we don't use more memory
  // in running this algorithm than we have to (and still ensure that
  // it runs fast).
  //
  // The implementation here sorts the indicies into a vectorized form
  // of the matrix (so that one integer can represent both the row and
  // the column), thus saving memory.

  std::vector< unsigned int > cost_ind;
  cost_ind.reserve( cost.rows() * cost.cols() );
  for( unsigned i = 0; i < cost.rows() * cost.cols(); ++i )
  {
    cost_ind.push_back( i );
  }

  cost_sorter sort;
  sort.cost_mat_ = &cost;
  std::sort( cost_ind.begin(), cost_ind.end(), sort );

  std::vector<bool> used_src( cost.rows(), false );
  std::vector<bool> used_tgt( cost.cols(), false );
  std::vector<unsigned> assn( cost.rows(), unsigned(-1) );

  for( unsigned idx = 0; idx < cost_ind.size(); ++idx )
  {
    unsigned i = cost_ind[idx] / cost.cols();
    unsigned j = cost_ind[idx] % cost.cols();
    if( ! used_src[i] &&
        ! used_tgt[j] )
    {
      assn[i] = j;
      used_src[i] = true;
      used_tgt[j] = true;
    }
  }

  return assn;
}


} // end namespace vidtk
