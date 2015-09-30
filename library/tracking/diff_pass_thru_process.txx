/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/diff_pass_thru_process.h>
#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <vcl_cassert.h>
#include <vcl_algorithm.h>

namespace vidtk
{

template <class PixType>
diff_pass_thru_process<PixType>
::diff_pass_thru_process( vcl_string const& name )
  : process( name, "diff_pass_thru_process" )
{

}


template <class PixType>
diff_pass_thru_process<PixType>
::~diff_pass_thru_process()
{
}


template <class PixType>
config_block
diff_pass_thru_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
diff_pass_thru_process<PixType>
::set_params( config_block const& blk )
{
  config_.update( blk );
  return true;
}


template <class PixType>
bool
diff_pass_thru_process<PixType>
::initialize()
{
  return true;
}


template <class PixType>
bool
diff_pass_thru_process<PixType>
::reset()
{
  return this->initialize();
}

template <class PixType>
bool
diff_pass_thru_process<PixType>
::step()
{
  return true;
}


} // end namespace vidtk
