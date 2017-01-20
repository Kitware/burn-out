/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief Generic shot stitching algorithm template method definitions
 */

#include "generic_shot_stitching_algo.h"

#include <sstream>
#include <string>
#include <vector>

#include <boost/foreach.hpp>

#include <logger/logger.h>
VIDTK_LOGGER("generic_shot_stitching_algo");


namespace vidtk
{


//============================================================================
// generic_shot_stitching_algo_impl
//----------------------------------------------------------------------------

template< typename PixType >
class generic_shot_stitching_algo_impl
{
public:
  typedef std::map<std::string, shot_stitching_algo<PixType>* > algo_map_t;
  typedef typename algo_map_t::iterator                         algo_map_iter_t;

  generic_shot_stitching_algo_impl();
  virtual ~generic_shot_stitching_algo_impl();

  // Possible implementations to be used
  algo_map_t algo_map_;
  // hold pointer of implementation to use of possible implementations
  shot_stitching_algo<PixType> *algo_;
  // String name of the current algorithm. Undefined if there is no current
  // algorithm set
  std::string algo_name_;

  // method subject to template specialization that initializes our map with
  // valid algorithm implementations for the given PixType. This method will
  // create new instances which will be released by destructor.
  void populate_algo_map();

};


template< typename PixType >
generic_shot_stitching_algo_impl<PixType>
::generic_shot_stitching_algo_impl()
  : algo_map_(),
    algo_( NULL ),
    algo_name_()
{
  this->populate_algo_map();
}


template< typename PixType >
generic_shot_stitching_algo_impl<PixType>
::~generic_shot_stitching_algo_impl()
{
  BOOST_FOREACH( typename algo_map_t::value_type &p, this->algo_map_ )
  {
    delete p.second;
  }
}


//============================================================================
// generic_shot_stitching_algo
//----------------------------------------------------------------------------

template< typename PixType >
generic_shot_stitching_algo<PixType>
::generic_shot_stitching_algo()
  : impl_( new generic_shot_stitching_algo_impl<PixType>() )
{
  // TODO: Start with default, generally available algo type setting?
}


template< typename PixType >
generic_shot_stitching_algo<PixType>
::~generic_shot_stitching_algo()
{
  delete this->impl_;
}


template< typename PixType >
config_block
generic_shot_stitching_algo<PixType>
::params() const
{
  config_block blk;

  // gather available algorithm type names from algo map
  std::stringstream ss;
  ss << "Type of algorithm to use for shot stitching." << std::endl
     << "Available types: [" << std::endl;

  // build up type comment list as well as add to config the blocks for each
  // algorithm currently in our map
  BOOST_FOREACH( typename generic_shot_stitching_algo_impl<PixType>::algo_map_t::value_type const &p,
                 this->impl_->algo_map_ )
  {
    // for type comment string
    ss << "\t" << p.first << std::endl;

    // add implementation parameters to config block
    blk.add_subblock( p.second->params(), p.first );
  }
  ss << "]";

  // provide the type if there is one set, else add the parameter with no
  // default, which forces the user to set something.
  if( this->impl_->algo_ )
  {
    blk.add_parameter( "type", this->impl_->algo_name_, ss.str());
  }
  else
  {
    blk.add_parameter( "type", ss.str());
  }

  return blk;
}


template< typename PixType >
bool
generic_shot_stitching_algo<PixType>
::set_params( config_block const &blk )
{
  try
  {
    // see if there is a valid implementation for the given algorithm type
    std::string algo_name = blk.get<std::string>( "type" );
    typename generic_shot_stitching_algo_impl<PixType>::algo_map_iter_t it = this->impl_->algo_map_.find(algo_name);
    if( it != this->impl_->algo_map_.end())
    {
      LOG_DEBUG("generic_shot_stitching_algo: Setting configuration for algo type \"" << algo_name << "\"");
      try
      {
        if( it->second->set_params( blk.subblock( algo_name ) ) )
        {
          this->impl_->algo_ = it->second;
          this->impl_->algo_name_ = it->first;
          return true;
        }
      }
      catch( config_block_parse_error const &e )
      {
        LOG_ERROR( "generic_shot_stitching_algo: "
                   << algo_name << ": "
                   << "Failed to set params: " << e.what() );
        return false;
      }
      LOG_ERROR( "generic_shot_stitching_algo: "
                 << algo_name << ": "
                 << "Failed to set params: Set function returned false." );
    }
    else
    {
      LOG_ERROR( "generic_shot_stitching_algo: "
                 << "Invalid algorithm implementation type given: "
                 << "\"" << algo_name << "\"" );
    }
  }
  catch( config_block_parse_error const &e)
  {
    LOG_ERROR( "generic_shot_stitching_algo: "
               << "Failed to set params: " << e.what() );
  }

  return false;
}


template< typename PixType >
image_to_image_homography
generic_shot_stitching_algo<PixType>
::stitch( vil_image_view<PixType> const &src,
          vil_image_view<bool> const &src_mask,
          vil_image_view<PixType> const &dest,
          vil_image_view<bool> const &dest_mask )
{
  if ( this->impl_->algo_ == NULL )
  {
    LOG_ERROR("generic_shot_stitching_algo: No shot stitching algorithm set");
    return image_to_image_homography();
  }
  return this->impl_->algo_->stitch(src, src_mask, dest, dest_mask);
}


} // end vidtk namespace
