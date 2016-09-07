/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "da_so_tracker_generator.h"

#include <vul/vul_string.h>

#include <utilities/log.h>

namespace vidtk
{


da_so_tracker_generator
::da_so_tracker_generator()
  : kalman_parameters_(NULL),
    wh_kalman_parameters_(NULL),
    ekalman_heading_parameters_(NULL)
{
}

da_so_tracker_generator
::~da_so_tracker_generator()
{
  kalman_parameters_ = NULL;
  ekalman_heading_parameters_ = NULL;
}

bool
da_so_tracker_generator
::set_so_type(vcl_string type)
{
  type = vul_string_upcase(type);
  if(type == "KALMAN")
  {
    tracker_to_generate_ = so_tracker_types::kalman;
    return true;
  }
    else if (type == "WH_KALMAN")
  {
    tracker_to_generate_ = so_tracker_types::wh_kalman;
    return true;
  }
  else if (type == "EKALMAN_HEADING")
  {
    tracker_to_generate_ = so_tracker_types::ekalman_speed_heading;
    return true;
  }
  vcl_cout << "UNRECOGNIZED: " << type << vcl_endl;
  //error
  return false;
}

da_so_tracker_sptr
da_so_tracker_generator
::generate( track_sptr init_track )
{
  switch (tracker_to_generate_)
  {
    case so_tracker_types::kalman:
    {
      return generate_kalman( init_track );
    }
     case so_tracker_types::wh_kalman:
    {
      return generate_wh_kalman( init_track );
    }
    case so_tracker_types::ekalman_speed_heading:
    {
      return generate_speed_heading(init_track);
    }
    default:
      log_error( "UNRECONIZED Single object tracker\n" );
      return NULL;
  }
}

da_so_tracker_sptr
da_so_tracker_generator
::generate_speed_heading( track_sptr init_track )
{
  if(!ekalman_heading_parameters_)
  {
    log_error( "NO EXTENDED KALMAN PARAMETERS\n" );
    return NULL;
  }
  extended_kalman_functions::speed_heading_fun fun;
//   extended_kalman_functions::linear_fun fun;
  da_so_tracker_sptr r =
    new da_so_tracker_kalman_extended<
      extended_kalman_functions::speed_heading_fun >( fun, ekalman_heading_parameters_);
//     new da_so_tracker_kalman_extended<extended_kalman_functions::linear_fun>( fun, 
//                                                                               ekalman_heading_parameters_);
  r->initialize( *init_track );
  return r;
}

da_so_tracker_sptr
da_so_tracker_generator
::generate_kalman( track_sptr init_track )
{
  if(!kalman_parameters_)
  {
    log_error( "NO KALMAN PARAMETERS\n" );
    return NULL;
  }
  da_so_tracker_sptr r = new da_so_tracker_kalman(kalman_parameters_);
  r->initialize( *init_track );
  return r;
}

da_so_tracker_sptr
da_so_tracker_generator
::generate_wh_kalman( track_sptr init_track )
{
  if(!wh_kalman_parameters_)
  {
    log_error( "NO KALMAN PARAMETERS\n" );
    return NULL;
  }
  da_so_tracker_sptr r = new wh_tracker_kalman(wh_kalman_parameters_);
  r->initialize( *init_track );
  return r;
}


void
da_so_tracker_generator
::set_kalman_params( da_so_tracker_kalman::const_parameters_sptr kp )
{
  kalman_parameters_ = kp;
}

vcl_string
da_so_tracker_generator
::get_so_type() const
{
  switch (tracker_to_generate_)
  {
    case so_tracker_types::kalman:
    {
      return "KALMAN";
    }
     case so_tracker_types::wh_kalman:
    {
      return "WH_KALMAN";
    }
    case so_tracker_types::ekalman_speed_heading:
    {
      return "EKALMAN_HEADING";
    }
    default:
      log_error( "UNRECONIZED Single object tracker\n" );
      return "";
  }
}

vcl_string
da_so_tracker_generator
::get_options() const
{
  return "KALMAN,WH_KALMAN,EKALMAN_HEADING";
}

void
da_so_tracker_generator
::add_default_parameters(config_block & block)
{
  block.add_parameter( "single_object_tracker", "KALMAN", 
                       "What time of single object trackers one should use: options (KALMAN,EKALMAN_HEADING)" );
  config_block kalman_blk;
  config_block wh_kalman_blk;
  config_block ekalman_heading_blk;
  da_so_tracker_kalman::const_parameters::add_parameters_defaults(kalman_blk);
  da_so_tracker_kalman::const_parameters::add_parameters_defaults(wh_kalman_blk);
  da_so_tracker_kalman_extended<extended_kalman_functions::speed_heading_fun>::const_parameters::add_parameters_defaults(ekalman_heading_blk);
  block.add_subblock(kalman_blk, "KALMAN");
  block.add_subblock(wh_kalman_blk, "WH_KALMAN");
  block.add_subblock(ekalman_heading_blk, "EKALMAN_HEADING");
}

bool
da_so_tracker_generator
::update_params(config_block const & block)
{
  vcl_string type = block.get<vcl_string>("single_object_tracker");
  if( !(this->set_so_type(type)) )
  {
    log_error( "Unknown type " << type << "\n" );
    return false;
  }
  switch( tracker_to_generate_ )
  {
    case so_tracker_types::kalman:
    {
      if(!kalman_parameters_)
      {
        kalman_parameters_ = new da_so_tracker_kalman::const_parameters;
      }
      config_block kblk = block.subblock( "KALMAN" );
      kalman_parameters_->set_parameters(kblk);
      break;
    }
      case so_tracker_types::wh_kalman:
    {
      if(!wh_kalman_parameters_)
      {
        wh_kalman_parameters_ = new wh_tracker_kalman::const_parameters;
      }
      config_block kblk = block.subblock( "WH_KALMAN" );
      wh_kalman_parameters_->set_parameters(kblk);
      break;
    }
    case so_tracker_types::ekalman_speed_heading:
    {
      if( !ekalman_heading_parameters_ )
      {
        ekalman_heading_parameters_
          = new da_so_tracker_kalman_extended<extended_kalman_functions::speed_heading_fun>::const_parameters;
      }
      config_block kblk = block.subblock( "EKALMAN_HEADING" );
      ekalman_heading_parameters_->set_parameters(kblk);
      break;
    }
    default:
      log_error( "UNRECOGNIZED Single object tracker\n" );
      return false;
  }

  return true;
}

void
da_so_tracker_generator
::set_to_default()
{
  config_block blk;
  this->add_default_parameters(blk);
  this->update_params(blk);
}

} //namespace vidtk

#include <vbl/vbl_smart_ptr.hxx>

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::da_so_tracker_generator );
