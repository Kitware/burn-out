/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_adaboost_classifier.h"

#include <learning/learner_data.h>

#include <map>
#include <algorithm>
#include <vector>
#include <limits>
#include <sstream>
#include <fstream>
#include <iostream>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "tot_adaboost_classifier_cxx" );

tot_adaboost_classifier
::tot_adaboost_classifier()
{
  this->set_sort_flag( true );
}

tot_adaboost_classifier
::~tot_adaboost_classifier()
{
}

bool
load_classifier( std::string fn, adaboost& output )
{
  if( !output.read_from_file( fn ) )
  {
    LOG_ERROR( "Unable to load " << fn );
    return false;
  }
  return true;
}

bool
tot_adaboost_classifier
::configure( const tot_adaboost_settings& params )
{
  settings_ = params;
  pvo_priors_.clear();

  if( !settings_.extract_features )
  {
    if( !load_classifier( params.main_person_classifier, main_classifiers_[0] ) ||
        !load_classifier( params.main_vehicle_classifier, main_classifiers_[1] ) ||
        !load_classifier( params.main_other_classifier, main_classifiers_[2] ) )
    {
      return false;
    }

    if( settings_.gsd_pivot > 0.0 &&
        ( !load_classifier( params.high_gsd_person_classifier, high_gsd_classifiers_[0] ) ||
          !load_classifier( params.high_gsd_vehicle_classifier, high_gsd_classifiers_[1] ) ||
          !load_classifier( params.high_gsd_other_classifier, high_gsd_classifiers_[2] ) ) )
    {
      return false;
    }
  }

  if( settings_.post_proc_mode == "none" )
  {
    postproc_mode_ = NONE;
  }
  else if( settings_.post_proc_mode == "scaled_ratio" )
  {
    postproc_mode_ = SCALED_RATIO;
  }
  else
  {
    LOG_ERROR( "Invalid mode string: " << settings_.post_proc_mode );
    return false;
  }

  if( settings_.frame_pivot % 2 == 1 )
  {
    LOG_ERROR( "Frame pivot must be an even number" );
    return false;
  }

  std::stringstream ss( settings_.normalization_additive_factor );

  for( unsigned i = 0; i < 3; ++i )
  {
    if( !( ss >> norm_factors_[i] ) )
    {
      LOG_ERROR( "Normalization additive factor length invalid" );
      return false;
    }
  }

  return true;
}

void
tot_adaboost_classifier
::terminate_track( const track_sptr_t& track_object )
{
  if( settings_.enable_temporal_averaging )
  {
    typedef std::map< unsigned, cumulative_pvo >::iterator map_itr_t;

    map_itr_t p = pvo_priors_.find( track_object->id() );

    if( p != pvo_priors_.end() )
    {
      pvo_priors_.erase( p );
    }
  }
}

class tot_ab_learner_wrapper : public learner_data
{
public:

  explicit tot_ab_learner_wrapper( const raw_descriptor::vector_t& descriptors )
  {
    unsigned total_size = 0;

    for( unsigned i = 0; i < descriptors.size(); ++i )
    {
      total_size += descriptors[i]->features_size();
    }

    features_.set_size( total_size );

    unsigned bin = 0;

    for( unsigned i = 0; i < descriptors.size(); ++i )
    {
      for( unsigned j = 0; j < descriptors[i]->features_size(); ++j )
      {
        features_[bin] = descriptors[i]->at( j );
        ++bin;
      }
    }
  }

  ~tot_ab_learner_wrapper() {}

  virtual vnl_vector<double> get_value( int ) const
  {
    return features_;
  }

  virtual double get_value_at(int, unsigned int loc) const
  {
    return features_[loc];
  }

  virtual unsigned int number_of_descriptors() const
  {
    return 1;
  }

  virtual unsigned int number_of_components(unsigned int) const
  {
    return features_.size();
  }

  virtual vnl_vector<double> vectorize() const
  {
    return features_;
  }

  virtual void write(std::ostream&) const
  {
  }

private:

  vnl_vector< double > features_;
};

online_track_type_classifier::object_class_sptr_t
tot_adaboost_classifier
::classify( const track_sptr_t& track_object,
            const raw_descriptor::vector_t& descriptors )
{
  double gsd = this->average_gsd();
  double p, v, o;

  // Identify GSD if provided
  /*if( !settings_.gsd_descriptor.empty() )
  {
    for( unsigned i = 0; i < descriptors.size(); ++i )
    {
      if( descriptors[i]->type_ == settings_.gsd_descriptor )
      {
        LOG_ASSERT( !descriptors[i]->get_features().empty(), "Invalid descriptor" );
        gsd = descriptors[i]->get_features().back();
      }
    }
  }*/

  bool is_high = ( gsd > settings_.gsd_pivot && gsd != 0 );

  // Create learner data around descriptor values
  tot_ab_learner_wrapper desc_wrapper( descriptors );

  if( settings_.extract_features )
  {
    std::string fn = settings_.feature_filename_prefix + ( is_high ? "_high" : "_low" );
    std::ofstream ofs( fn.c_str(), std::ios::app );
    ofs << "[" << track_object->id() + settings_.track_id_adj << "]";
    for( unsigned i = 0; i < desc_wrapper.number_of_components(0); ++i )
    {
      ofs << " " << desc_wrapper.get_value_at( 0, i ) << " ";
    }
    ofs << std::endl;
    ofs.close();
    return object_class_sptr_t(NULL);
  }

  // Apply the correct classifiers
  if( is_high )
  {
    p = high_gsd_classifiers_[0].classify_raw( desc_wrapper );
    v = high_gsd_classifiers_[1].classify_raw( desc_wrapper );
    o = high_gsd_classifiers_[2].classify_raw( desc_wrapper );
  }
  else
  {
    p = main_classifiers_[0].classify_raw( desc_wrapper );
    v = main_classifiers_[1].classify_raw( desc_wrapper );
    o = main_classifiers_[2].classify_raw( desc_wrapper );
  }

  // Average probs
  if( settings_.enable_temporal_averaging )
  {
    this->average_pvo_values( track_object->id(), p, v, o );
  }

  // Convert classifier output to probabilities
  if( postproc_mode_ == SCALED_RATIO )
  {
    this->scale_pvo_values( p, v, o );
  }

  // Return latest probabilities to attach to track
  return object_class_sptr_t( new object_class_t( p, v, o ) );
}

void
tot_adaboost_classifier
::scale_pvo_values( double& p, double& v, double& o )
{
  const double positive_benefit = settings_.positive_scale_percent;
  const double max_reported_pvo = settings_.max_score_to_report;
  const double expansion_ratio = settings_.normalization_expansion_ratio;

  p = ( p < 0 ? p : p * positive_benefit );
  v = ( v < 0 ? v : v * positive_benefit );
  o = ( o < 0 ? o : o * positive_benefit );

  double minv = 0.0;

  if( minv > p )
  {
    minv = p;
  }
  if( minv > v )
  {
    minv = v;
  }
  if( minv > o )
  {
    minv = o;
  }

  p = p - minv;
  v = v - minv;
  o = o - minv;

  double sum = p + v + o;

  if( sum == 0.0 )
  {
    p = 1.0/3.0;
    v = 1.0/3.0;
    o = 1.0/3.0;
  }
  else
  {
    p = p / sum;
    v = v / sum;
    o = o / sum;
  }

  p += norm_factors_[0];
  v += norm_factors_[1];
  o += norm_factors_[2];

  sum = p + v + o;

  if( sum != 0.0 )
  {
    p /= sum;
    v /= sum;
    o /= sum;
  }
  else
  {
    p = 1.0/3.0;
    v = 1.0/3.0;
    o = 1.0/3.0;
  }

  if( expansion_ratio > 0.0 )
  {
    double to_add = 0;

    double* o1;
    double* o2;

    if( p > v && p > o )
    {
      to_add = ( 1.0 - p ) * expansion_ratio;
      p += to_add;

      o1 = &v;
      o2 = &o;
    }
    else if( v > p && v > o )
    {
      to_add = ( 1.0 - v ) * expansion_ratio;
      v += to_add;

      o1 = &p;
      o2 = &o;
    }
    else
    {
      to_add = ( 1.0 - o ) * expansion_ratio;
      o += to_add;

      o1 = &p;
      o2 = &v;
    }

    double other_sum = *o1 + *o2;

    if( to_add > 0.0 && other_sum > 0.0 )
    {
      double other_scale = to_add / other_sum;

      (*o1) *= other_scale;
      (*o2) *= other_scale;
    }
  }

  // Final boundary normalization
  if( p > max_reported_pvo )
  {
    double diff = p - max_reported_pvo;
    p = max_reported_pvo;
    v += diff / 2;
    o += diff / 2;
  }

  if( v > max_reported_pvo )
  {
    double diff = v - max_reported_pvo;
    p += diff / 2;
    v = max_reported_pvo;
    o += diff / 2;
  }

  if( o > max_reported_pvo )
  {
    double diff = o - max_reported_pvo;
    p += diff / 2;
    v += diff / 2;
    o = max_reported_pvo;
  }

  sum = p + v + o;

  if( sum != 0.0 )
  {
    p /= sum;
    v /= sum;
    o /= sum;
  }
  else
  {
    p = 1.0/3.0;
    v = 1.0/3.0;
    o = 1.0/3.0;
  }
}

void
tot_adaboost_classifier
::average_pvo_values( unsigned track_id, double& p, double& v, double& o )
{
  cumulative_pvo& net_pvo = pvo_priors_[track_id];

  net_pvo.p_sum += p;
  net_pvo.v_sum += v;
  net_pvo.o_sum += o;

  ++net_pvo.count;

  if( !net_pvo.hit_pivot && net_pvo.count == settings_.frame_pivot )
  {
    net_pvo.p_sum = net_pvo.p_sum / 2;
    net_pvo.v_sum = net_pvo.v_sum / 2;
    net_pvo.o_sum = net_pvo.o_sum / 2;

    net_pvo.count = net_pvo.count / 2;
    net_pvo.hit_pivot = true;
  }

  p = net_pvo.p_sum / net_pvo.count;
  v = net_pvo.v_sum / net_pvo.count;
  o = net_pvo.o_sum / net_pvo.count;
}

}
