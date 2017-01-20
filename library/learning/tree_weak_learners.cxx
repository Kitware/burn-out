/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tree_weak_learners.h"

#include <cassert>
#include <iostream>
#include <algorithm>

#include <vnl/vnl_math.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_tree_weak_learners_cxx__
VIDTK_LOGGER("tree_weak_learners_cxx");


namespace vidtk
{

tree_weak_learner
::tree_weak_learner( std::string const & _name, int desc )
      : weak_learner(_name, desc)
{
}

weak_learner_sptr
tree_weak_learner
::train( training_feature_set const &,
         vnl_vector<double> const & )
{
  return static_cast<weak_learner*>(new tree_weak_learner( name_, descriptor_) );
}

int
tree_weak_learner
::classify(learner_data const & data)
{
  for( unsigned i = 0; i < tree_.size(); i++ )
  {
    double v = data.get_value_at( 0 , tree_[i].dim_ );

    if( tree_[i].sign_ )
    {
      if( v <= tree_[i].thresh_ )
      {
        return 0;
      }
    }
    else
    {
      if( v >= tree_[i].thresh_ )
      {
        return 0;
      }
    }
  }
  return 1;
}

bool
tree_weak_learner
::read(std::istream & in)
{
  unsigned int tree_size;
  in >> tree_size;

  tree_.resize( tree_size );

  for( unsigned i = 0; i < tree_size; i++ )
  {
    in >> tree_[i].dim_;
    in >> tree_[i].thresh_;
    in >> tree_[i].sign_;

    if( !in )
    {
      LOG_ERROR( "Error reading tree learner" );
      return false;
    }
  }
  return true;
}

bool
tree_weak_learner
::write(std::ostream & out) const
{
  bool r = weak_learner::write(out);
  return r && out;
}


std::string
tree_weak_learner
::unique_id() const
{
  std::stringstream ss;
  ss << name_ << "_tl_" << tree_.size() << "_";
  for( unsigned int lvl = 0; lvl < tree_.size(); lvl++ )
  {
    boundary bd = tree_[lvl];
    ss << bd.sign_ << "_";
    ss << bd.dim_ << "_";
    ss << bd.thresh_;
  }
  std::string r;
  ss >> r;
  return r;
}

} //namespace vidtk
