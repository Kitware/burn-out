/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "conn_comp_pass_thru_process.h"

namespace vidtk
{

template <class PixType>
conn_comp_pass_thru_process<PixType>
::conn_comp_pass_thru_process( std::string const& _name )
  : process( _name, "conn_comp_pass_thru_process" )
{

}


template <class PixType>
conn_comp_pass_thru_process<PixType>
::~conn_comp_pass_thru_process()
{
}


template <class PixType>
config_block
conn_comp_pass_thru_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
conn_comp_pass_thru_process<PixType>
::set_params( config_block const& blk )
{
  config_.update( blk );
  return true;
}


template <class PixType>
bool
conn_comp_pass_thru_process<PixType>
::initialize()
{
  return true;
}


template <class PixType>
bool
conn_comp_pass_thru_process<PixType>
::reset()
{
  return this->initialize();
}

template <class PixType>
bool
conn_comp_pass_thru_process<PixType>
::step()
{
  return true;
}


} // end namespace vidtk
