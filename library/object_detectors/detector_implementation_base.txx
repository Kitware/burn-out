/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detector_implementation_base.h"

#include <object_detectors/detector_super_process_port_def.h>
#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>
#include <utilities/apply_functor_to_vector_process.txx>
#include <boost/foreach.hpp>

namespace vidtk
{

template < class PixType >
detector_implementation_base< PixType >
::detector_implementation_base( std::string const& n )
  :
#define init_input_pads(T,N) pad_source_ ## N( NULL ),
  DETECTOR_SUPER_PROCESS_INPUTS( init_input_pads )
#undef init_input_pads

#define init_output_pads(T,N) pad_output_ ## N( NULL ),
  DETECTOR_SUPER_PROCESS_OUTPUTS( init_output_pads )
#undef init_output_pads

  m_name( n )
{
  // Create detector source config enum
  image_object::source_code_enum_list_t list = image_object::source_code_enum_list();
  BOOST_FOREACH( image_object::source_code_enum_list_t::value_type val, list )
  {
    m_detector_source.add( val.second, val.first );
  }
}


template < class PixType >
detector_implementation_base< PixType >
::~detector_implementation_base()
{ }



template < class PixType >
bool
detector_implementation_base< PixType >
::reset()
{
  return pipeline()->reset();
}


template < class PixType >
bool
detector_implementation_base< PixType >
::initialize()
{
  return pipeline()->initialize();
}


// ----------------------------------------------------------------
template < class PixType >
void
detector_implementation_base< PixType >
::create_base_pads()
{
  //input pads
#define create_input_pads(T,N) pad_source_ ## N = new vidtk::super_process_pad_impl< T > ( "pad_source_" # N );
  DETECTOR_SUPER_PROCESS_INPUTS( create_input_pads )
#undef create_input_pads

#define create_output_pads(T,N) pad_output_ ## N = new vidtk::super_process_pad_impl< T > ( "pad_output_" # N );
  DETECTOR_SUPER_PROCESS_OUTPUTS( create_output_pads )
#undef create_output_pads
}


template < class PixType >
template < class Pipeline >
void
detector_implementation_base< PixType >
::setup_base_pipeline( Pipeline* p )
{
  this->m_pipeline = p; // save generic pipeline pointer

#define add_input_pads(T,N) p->add( this->pad_source_ ## N );
  DETECTOR_SUPER_PROCESS_INPUTS( add_input_pads )
#undef add_input_pads

#define add_output_pads(T,N) p->add( this->pad_output_ ## N );
  DETECTOR_SUPER_PROCESS_OUTPUTS( add_output_pads )
#undef add_output_pads

}


template < class PixType >
vidtk::pipeline_sptr
detector_implementation_base< PixType >
::pipeline()
{
  return m_pipeline;
}


template < class PixType >
std::string const&
detector_implementation_base< PixType >
::name() const
{
  return m_name;
}

// instantiate the apply functor process
template class apply_functor_to_vector_process< image_object_sptr, image_object_sptr, set_type_functor >;


#undef DETECTOR_SUPER_PROCESS_OUTPUTS
#undef DETECTOR_SUPER_PROCESS_INPUTS

} //end namespace vidtk
