/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_descriptor_generator_h_
#define vidtk_tot_descriptor_generator_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/tot_super_process.h>

namespace vidtk
{

class tot_descriptor_generator;

/// \brief Settings wrapper for the tot_descriptor_generator class.
class tot_descriptor_settings : public external_settings
{
public:

  tot_descriptor_settings();
  ~tot_descriptor_settings() {}

  /// Return a vidtk config block for all parameters.
  config_block config() const;

  /// Parse a vidtk style config block.
  void read_config( const config_block& blk );

private:

  friend class tot_descriptor_generator;

  config_block stored_config;
  bool output_intermediates;
};

// ------------------------------------------------------------------
/// \brief A generator for computing TOT classifications, only in descriptor form.
///
/// This class provides an alternative method for outputting TOT classifications
/// simply by wrapping the TOT super process. This class should have no children.
class tot_descriptor_generator : public online_descriptor_generator
{

public:

  tot_descriptor_generator();
  ~tot_descriptor_generator();

  /// Set any settings for this descriptor generator.
  bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  /// Called everytime we receive a new frame.
  bool frame_update_routine();

  /// Reset the generator.
  bool reset_generator();

private:

  // Settings which can be set externally by the caller
  tot_descriptor_settings settings_;

  // Internal call to the tot super process
  tot_super_process< vxl_byte > computer_;
};


} // end namespace vidtk

#endif // vidtk_tot_descriptor_generator_h_
