/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_collector_process.h"

#include <descriptors/tot_collector_prob_product.h>
#include <descriptors/tot_adaboost_classifier.h>

#include <boost/lexical_cast.hpp>

#include <limits>
#include <sstream>
#include <string>

#include <logger/logger.h>

VIDTK_LOGGER( "tot_collector_process_cxx" );

namespace vidtk
{

tot_collector_process
::tot_collector_process( std::string const& _name )
  : process( _name, "tot_collector_process" ),
    mode_( DISABLED ),
    input_des_(),
    input_trks_(),
    input_gsd_( 0 ),
    output_trks_(),
    classifier_( NULL )
{
}

tot_collector_process
::~tot_collector_process()
{
}

config_block
tot_collector_process
::params() const
{
  config_block cfg;

  cfg.add_parameter(
    "disabled",
    "false",
    "Disable this process?" );

  cfg.add_parameter(
    "method",
    "prob_product",
    "Method to generate TOT probability scores, can be either: adaboost, "
    "or prob_product." );

  cfg.add_parameter(
    "target_descriptors",
    "",
    "A comma separated list of descriptor ids to use for TOT formulation. "
    "If empty, all descriptors will be used." );

  cfg.add_parameter(
    "min_descriptor_count",
    "0",
    "Minimum descriptor count required to formulate TOT probabilities." );

  cfg.add_parameter(
    "max_descriptor_count",
    boost::lexical_cast<std::string>(std::numeric_limits<unsigned>::max()),
    "Minimum descriptor count required to formulate TOT probabilities." );

  cfg.add_subblock( tot_adaboost_settings().config(), "adaboost" );

  return cfg;
}

bool
tot_collector_process
::set_params( config_block const& blk )
{
  try
  {
    if( blk.get<bool>( "disabled" ) )
    {
      mode_ = DISABLED;
    }
    else
    {
      // Load method specific settings
      std::string method = blk.get<std::string>( "method" );

      if( method == "prob_product" )
      {
        mode_ = PROB_PRODUCT;
        classifier_.reset( new tot_collector_prob_product() );
      }
      else if( method == "adaboost" )
      {
        mode_ = ADABOOST;
        classifier_.reset( new tot_adaboost_classifier() );

        tot_adaboost_settings adaboost_options;
        adaboost_options.read_config( blk.subblock( "adaboost" ) );

        if( !static_cast<tot_adaboost_classifier&>(*classifier_).configure( adaboost_options) )
        {
          throw config_block_parse_error( "Unable to set tot classifier parameters" );
        }
      }
      else
      {
        throw config_block_parse_error( "Invalid method string:" + method );
      }

      // Load general settings
      classifier_->set_min_required_count( blk.get<unsigned>( "min_descriptor_count" ) );
      classifier_->set_max_required_count( blk.get<unsigned>( "max_descriptor_count" ) );

      std::string id_filter = blk.get<std::string>( "target_descriptors" );

      if( !id_filter.empty() )
      {
        std::string single_id;
        std::vector< std::string > id_list;
        std::stringstream ss( id_filter );
        while( ss >> single_id )
        {
          id_list.push_back( single_id );

          if( ss.peek() == ',' )
          {
            ss.ignore();
          }
        }

        classifier_->set_ids_to_filter( id_list );
      }
    }
  }
  catch( const config_block_parse_error &e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  return true;
}

bool
tot_collector_process
::initialize()
{
  return true;
}

bool
tot_collector_process
::step()
{
  output_trks_ = input_trks_;

  if( mode_ == DISABLED )
  {
    return true;
  }

  classifier_->set_average_gsd( input_gsd_ );
  classifier_->apply( output_trks_, input_des_ );

  input_des_.clear();
  input_trks_.clear();

  return true;
}

void
tot_collector_process
::set_descriptors( raw_descriptor::vector_t const& descriptors )
{
  input_des_ = descriptors;
}

void
tot_collector_process
::set_source_tracks( track::vector_t const& trks )
{
  input_trks_ = trks;
}

void
tot_collector_process
::set_source_gsd( double gsd )
{
  input_gsd_ = gsd;
}


vidtk::track::vector_t
tot_collector_process
::output_tracks() const
{
  return output_trks_;
}

} // end namespace vidtk
