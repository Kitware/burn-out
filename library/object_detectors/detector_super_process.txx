/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detector_super_process.h"
#include "detector_super_process_port_def.h"

#include <string>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __detector_super_process_txx__

namespace vidtk
{

VIDTK_LOGGER( "detector_super_process" );


// ----------------------------------------------------------------
template< class PixType >
detector_super_process< PixType >
::detector_super_process( std::string const& n, // process instance name
    typename detector_implementation_base< PixType >::sptr impl )
  : super_process( n, "detector_super_process" ),
    m_impl_ptr( impl )
{
  // Just to make sure -- the pointer to the implementation must point
  // to something. If NULL pointer, then we can not proceed. Something
  // must have gone tragically wrong.
  if ( ! m_impl_ptr )
  {
    LOG_AND_DIE( "NULL pointer passed for implementation." );
  }
}


template< class PixType >
detector_super_process< PixType >
::~detector_super_process()
{
}


// ----------------------------------------------------------------
template< class PixType >
config_block
detector_super_process< PixType >
::params() const
{
  return m_impl_ptr->params();
}


// ----------------------------------------------------------------
template< class PixType >
bool
detector_super_process< PixType >
::set_params( config_block const& blk )
{
  // Select config block for our implementation.
  LOG_INFO( "Selecting config for " << m_impl_ptr->name() );
  config_block impl_config = blk.subblock( m_impl_ptr->name() );

  // Pass required parameters down to the implementation type
  impl_config.set( "run_async",
                   blk.get<bool>( "run_async" ) );
  impl_config.set( "gui_feedback_enabled",
                   blk.get<bool>( "gui_feedback_enabled" ) );
  impl_config.set( "pipeline_edge_capacity",
                   blk.get<unsigned>( "pipeline_edge_capacity" ) );
  impl_config.set( "masking_enabled",
                   blk.get<bool>( "masking_enabled" ) );
  impl_config.set( "location_type",
                   blk.get<std::string>( "location_type" ) );

  // pass config to the implementation
  bool status = m_impl_ptr->set_params( impl_config );

  // Get the implementation pipeline and save as ours.
  this->pipeline_ = m_impl_ptr->pipeline();

  return ( status );
}


// ----------------------------------------------------------------
template< class PixType >
bool
detector_super_process< PixType >
::initialize()
{
  // Called after set_params()
  return m_impl_ptr->initialize();
}


// ----------------------------------------------------------------
template< class PixType >
bool
detector_super_process< PixType >
::reset()
{
  // called to reset the implementation and reload config
  // reset needs to go to the impl, which passes it to the pipeline.
  return m_impl_ptr->reset();
}


// ----------------------------------------------------------------
template< class PixType >
process::step_status
detector_super_process< PixType >
::step2()
{
  // -- run our pipeline -- sync mode only --
  process::step_status p_status = pipeline_->execute();

  return (p_status);
}


//
// Out input and output ports/methods just bridge to the the
// implementation pad processes.
//

#define link_input_pads(T,N)                            \
template< class PixType >                               \
void detector_super_process< PixType >                  \
::input_ ## N (T const& val )                           \
{ m_impl_ptr->pad_source_ ## N ->set_value( val ); }

DETECTOR_SUPER_PROCESS_INPUTS( link_input_pads )


#define link_output_pads(T,N)                           \
template< class PixType >                               \
T detector_super_process< PixType >                     \
::output_ ## N () const                                 \
{ return m_impl_ptr->pad_output_ ## N ->value(); }

DETECTOR_SUPER_PROCESS_OUTPUTS( link_output_pads )


#undef DETECTOR_SUPER_PROCESS_OUTPUTS
#undef DETECTOR_SUPER_PROCESS_INPUTS

} // end namespace vidtk
