/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "multiple_features_process.h"

#include <utilities/unchecked_return_value.h>

namespace vidtk
{

multiple_features_process
::multiple_features_process( vcl_string const & name )
: process( name, "mulitple_features_process" ),
  config_dir_()
{
  config_.add_parameter( "w_color", "0.3", "The weight on the color simularity" );
  config_.add_parameter( "w_kinematics", "0.5", "The weight on the kinematic simularity" );
  config_.add_parameter( "w_area", "0.2", "The weight on the area simularity" );
  config_.add( "pdf_delta", "0.005" );
  config_.add_optional( 
    "test_online_filename", 
    "This file is mandatory for use multiple features (color,"       
    "kinematics and area)for correspondence. It contains "
    "'learned model' in the form of CDF look up tables." );
  config_.add_optional( 
    "test_init_filename", 
    "This file is mandatory for use multiple features (color,"       
    "kinematics and area)for correspondence. It contains "
    "'learned model' in the form of CDF look up tables." );
  config_.add_optional( 
    "train_filename", 
    "Used for storing data to train.");

  mf_online_params_ = new multiple_features;
  mf_init_params_ = new multiple_features;
}

multiple_features_process
::~multiple_features_process()
{
  delete mf_online_params_;
  delete mf_init_params_;
}

config_block
multiple_features_process
::params() const
{
  return config_;
}

bool
multiple_features_process
::set_params( config_block const & blk )
{ 
  try
  {
    double v;
    double w_color, w_kinematics, w_area, w_sum;
    blk.get<double>( "w_color", w_color);
    blk.get<double>( "w_kinematics", w_kinematics);
    blk.get<double>( "w_area", w_area);

    // case of weights less than zero has been handled
    // when they are read in
    w_sum = w_color + w_kinematics + w_area;

    if(w_sum != 0)
    {
      mf_init_params_->w_color = w_color/w_sum;
      mf_online_params_->w_color = mf_init_params_->w_color;
      mf_init_params_->w_kinematics = w_kinematics/w_sum;
      mf_online_params_->w_kinematics = mf_init_params_->w_kinematics;
      mf_init_params_->w_area = w_area/w_sum;
      mf_online_params_->w_area = mf_init_params_->w_area;
    }
    else
    {
      mf_init_params_->w_color = 0.3;
      mf_online_params_->w_color = 0.3;
      mf_init_params_->w_kinematics = 0.5;
      mf_online_params_->w_kinematics = 0.5;
      mf_init_params_->w_area = 0.2;
      mf_online_params_->w_area = 0.2;
    }

    blk.get<double>( "pdf_delta", v);
    mf_init_params_->pdf_delta = v;
    mf_online_params_->pdf_delta = v;

    mf_online_params_->test_enabled = blk.has( "test_online_filename" );
    if( mf_online_params_->test_enabled  ) 
    {
      blk.get( "test_online_filename", test_online_filename_ );
    }
    mf_init_params_->test_enabled = blk.has( "test_init_filename" );
    if( mf_init_params_->test_enabled  ) 
    {
      blk.get( "test_init_filename", test_init_filename_ );
    }
    mf_online_params_->train_enabled = blk.has( "train_filename" );
    if( mf_online_params_->train_enabled ) 
    {
      blk.get( "train_filename", mf_online_params_->train_filename );
    }
  }
  catch( unchecked_return_value& )
  {
    // restore old values
    set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}

bool
multiple_features_process
::initialize()
{
  bool v = true;

  if(   mf_online_params_->test_enabled )
  {
    vcl_string filename;
    if ( this->config_dir_.size() > 0 )
    {
#ifdef WIN32
      filename = this->config_dir_ + "\\" + this->test_online_filename_;
#else
      filename = this->config_dir_ + "/" + this->test_online_filename_;
#endif
    }
    else
    {
      filename = this->test_online_filename_;
    }
    vcl_cout << "multiple_features_process, loading file : " << filename << vcl_endl;
    v = mf_online_params_->load_cdfs_params( filename );
  }
  if(   mf_init_params_->test_enabled )
  {
    vcl_string filename;
    if ( this->config_dir_.size() > 0 )
    {
#ifdef WIN32
      filename = this->config_dir_ + "\\" + this->test_init_filename_;
#else
      filename = this->config_dir_ + "/" + this->test_init_filename_;
#endif
    }
    else
    {
      filename = this->test_init_filename_;
    }
    vcl_cout << "multiple_features_process, loading file : " << filename << vcl_endl;
    v = mf_init_params_->load_cdfs_params( filename ) && v;
  }

  return v;
}

bool
multiple_features_process
::reset()
{
  return this->initialize();  
}

bool
multiple_features_process
::step()
{
  //do nothing. I am just supposed to send  'mf_*_params_' to
  //whoever wants it.
  return true;
}

multiple_features const& 
multiple_features_process
::mf_online_params() const
{
  return *mf_online_params_;
}

multiple_features const& 
multiple_features_process
::mf_init_params() const
{
  return *mf_init_params_;
}

void
multiple_features_process
::set_configuration_directory( vcl_string const & dir )
{
  // Makes a copy of config_dir
  this->config_dir_ = dir;
}


}// namespace vidtk
