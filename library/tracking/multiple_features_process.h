/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_multiple_features_process_h_
#define vidtk_multiple_features_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/multiple_features.h>

namespace vidtk
{

  // Defining this process class because underlying 'multiple features
  //could be used by 'track_initializer_process' or 
  //'da_track_process'.

class multiple_features_process
  : public process
{
public:
  typedef multiple_features_process self_type;

  multiple_features_process( vcl_string const & name );

  multiple_features_process();

  ~multiple_features_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();
 
  virtual bool reset();

  virtual bool step();

  /// Set of parameters for MultiFeature tracking.
  multiple_features const& mf_online_params() const;

  multiple_features const& mf_init_params() const;

  void set_configuration_directory( vcl_string const & dir );

protected:
  config_block config_;

private:

  //Multi-feature (mf) parameters: Using color, area and kinemaatics 
  //in the cost function for correspondence between objects and tracks.
  multiple_features * mf_online_params_;
  multiple_features * mf_init_params_;

  vcl_string test_online_filename_;
  vcl_string test_init_filename_;

  vcl_string config_dir_;
}; //class multiple_features_process


} //namespace vidtk

#endif //vidtk_multiple_features_process_h_
