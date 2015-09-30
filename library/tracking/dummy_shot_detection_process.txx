/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "dummy_shot_detection_process.h"

namespace vidtk
{

template< class PixType >
dummy_shot_detection_process< PixType >
::dummy_shot_detection_process( vcl_string const& name )
  : process( name, "dummy_shot_detection_process" ),
    H_src2ref_( NULL ),
    is_end_of_shot_( false )
{  
}


template< class PixType >
dummy_shot_detection_process< PixType >
::~dummy_shot_detection_process()
{
}


template< class PixType >
config_block
dummy_shot_detection_process< PixType >
::params() const
{
  return config_block();
}


template< class PixType >
bool
dummy_shot_detection_process< PixType >
::set_params( config_block const& blk )
{
  return true;
}


template< class PixType >
bool
dummy_shot_detection_process< PixType >
::initialize()
{
  is_end_of_shot_ = false;
  return true;
}

template< class PixType >
bool
dummy_shot_detection_process< PixType >
::reset()
{
  return this->initialize();
}

template< class PixType >
bool
dummy_shot_detection_process< PixType >
::step()
{
  H_src2ref_ = NULL;

  // Temporary criteria for dummy testing 
  static unsigned counter = 0;
  //if( counter++ % 200 )
  //{
  //  // Publishing to the pipeline that a new shot has been detected. 
  //  // Current way to recover is for this fail to trickle down to the 
  //  // bottom of the tracking pipline and have it reset() to the 
  //  // beginning of the next shot. 
  //  is_end_of_shot_ = true;
  //}

  return true;
}

template< class PixType >
void
dummy_shot_detection_process< PixType >
::set_src_to_ref_homography( vgl_h_matrix_2d<double> const& H ) 
{
  H_src2ref_ = &H;
}

} // end namespace vidtk
