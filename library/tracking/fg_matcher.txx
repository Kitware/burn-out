/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "fg_matcher.h"

#include <tracking/fg_matcher_ssd.h>
#include <tracking/fg_matcher_csurf.h>
#include <utilities/log.h>

namespace vidtk
{

template< class PixType >
config_block
fg_matcher< PixType >
::params()
{
  config_block blk;

  blk.add_parameter( "type",
    "ssd",
    "Type of foreground matching algorithm: ssd or csurf." );

  blk.add_subblock( fg_matcher_ssd< PixType >::params(), "ssd" );
  blk.add_subblock( fg_matcher_csurf< PixType >::params(), "csurf" );

  return blk;
}

/********************************************************************/

template< class PixType >
typename fg_matcher< PixType >::sptr_t
fg_matcher< PixType >
::create_with_params( config_block const& blk )
{
  boost::shared_ptr< fg_matcher< PixType > > invalid; // defaults to "0".
  boost::shared_ptr< fg_matcher< PixType > > fgm;

  vcl_string type;
  try
  {
    type = blk.get< vcl_string >( "type" );
  }
  catch( config_block_parse_error & e )
  {
    log_error( "fg_matcher initialize failed, because: "<< e.what() <<"\n" );
    return invalid;
  }

  if( type == "ssd" )
  {
    fgm = typename fg_matcher< PixType >::sptr_t( new fg_matcher_ssd< PixType > );
  }
  else if( type == "csurf" )
  {
    fgm = typename fg_matcher< PixType >::sptr_t( new fg_matcher_csurf< PixType > );
  }
  else
  {
    log_error( "Invalid fg_matcher type '" << type << "'\n " );
    return invalid;
  }

  if( !fgm->set_params( blk.subblock( type ) ) )
  {
    return invalid;
  }
  
  return fgm;
}

/********************************************************************/

} // namespace vidtk
