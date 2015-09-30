/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//multi_track_linking_functors_functor.cxx
#include "multi_track_linking_functors_functor.h"

namespace vidtk
{

double
multi_track_linking_functors_functor
::operator()( TrackTypePointer track_i, TrackTypePointer track_j )
{
  double cost = 0;
  for ( unsigned int i = 0; i < functors_.size(); ++i )
  {
    if(weights_[i] != 0 && cost != impossible_match_cost_)
    {
      cost += weights_[i] * ((*(functors_[i]))(track_i, track_j));
    }
  }
  if(cost > impossible_match_cost_)
  {
    return impossible_match_cost_;
  }
  return cost;
}

multi_track_linking_functors_functor
::multi_track_linking_functors_functor( vcl_vector< track_linking_functor_sptr > const & fun,
                                        vcl_vector< double > const & w, double imc )
    : impossible_match_cost_(imc), functors_(fun), weights_(w)
{
  double sum = 0;
  for(unsigned int i = 0; i < weights_.size(); ++i)
  {
    sum += weights_[i];
  }
  for(unsigned int i = 0; i < weights_.size(); ++i)
  {
    weights_[i] = weights_[i]/sum;
  }
}

}//namespace vidtk
