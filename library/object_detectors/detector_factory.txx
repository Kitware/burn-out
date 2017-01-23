/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detector_factory.h"
#include "detector_implementation_factory.h"

#include <object_detectors/detector_super_process.h>

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <string>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("detector_factory");

template< class PixType >
detector_factory< PixType >
::detector_factory( std::string const& n )
  : m_name( n )
{
  add_variants_for_type();
}


template< class PixType >
detector_factory< PixType >
::~detector_factory()
{
}


// ----------------------------------------------------------------
template< class PixType >
vidtk::config_block
detector_factory< PixType >
::params() const
{
  config_block blk;

  // These parameters are set at the top factory level and are
  // available to the implementation which passed down to the
  // processes by the implementation.
  blk.add_parameter( "run_async",
                     "true",
                     "Whether or not to run asynchronously" );

  blk.add_parameter( "gui_feedback_enabled",
                     "false",
                     "Whether we should enable gui feedback support in the pipeline" );

  blk.add_parameter( "pipeline_edge_capacity",
                     boost::lexical_cast<std::string>(10 / sizeof(PixType)),
                     "Maximum size of the edge in an async pipeline." );

  blk.add_parameter( "masking_enabled",
                     "false",
                     "Whether or not some image mask should be used for filtering" );

  blk.add_parameter( "location_type",
                     "centroid",
                     "Location type for detection locations; centroid or bottom" );

  // build documentation string and get variant configs
  std::string descr = "Detector variant type. Allowable types are: ";

  BOOST_FOREACH( typename map_t::value_type const& val, this->m_variants)
  {
    // Add name of variant to description string.
    descr += val.first + ", ";

    // Collect parameters from variants
    blk.add_subblock( val.second->params(), val.first );
  }

  blk.add_parameter( "detector_type", descr );

  return blk;
}


// ----------------------------------------------------------------
template < class PixType >
process_smart_pointer< detector_super_process< PixType > >
detector_factory< PixType >
::create_detector( config_block const& blk )
{
  // "detector_type" entry contains the name of detector to create
  std::string variant = blk.get< std::string > ( name() + ":detector_type" );

  // Find variant in list of factories
  if ( m_variants.count( variant ) == 0 )
  {
    LOG_ERROR( "Detector variant type\"" << variant << "\" is not registered.");
    return process_smart_pointer< detector_super_process< PixType > >();
  }

  typename detector_implementation_base< PixType >::sptr detector_impl;

  LOG_DEBUG( "Creating detector from factory \"" << this->name()
             << "\". Detector implementation name \"" << variant << "\"." );

  // Use factory to create detector variant process.
  detector_impl = m_variants[variant]->create_detector_implementation( variant );

  // Instantiate a new detector super process and have it use our new
  // implementation. The detector variant is named with the factory
  // name and variant type.
  process_smart_pointer< detector_super_process< PixType > > new_detector(
    new detector_super_process< PixType >( this->name(), detector_impl ) );

  return new_detector;
}


// ----------------------------------------------------------------
template < class PixType >
std::string const&
detector_factory< PixType >
::name() const
{
  return m_name;
}

} // end namespace vidtk
