/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_floating_point_image_hash_process_h_
#define vidtk_floating_point_image_hash_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>

template< typename FloatType>
class vil_image_view;

namespace vidtk
{

/// Reduces some floating point image to some integral type via a user
/// defined algorithm. Current offerings:
///
///   SCALE_POSITIVE: Will scale the floating point image values (assumed
///   to contain positive values only) by a fixed value, and then round
///   it to the nearest integral type.
///
template< typename FloatType, typename HashType >
class floating_point_image_hash_process
  : public process
{
public:

  typedef floating_point_image_hash_process self_type;
  typedef vil_image_view< FloatType > input_type;
  typedef vil_image_view< HashType > output_type;

  floating_point_image_hash_process( std::string const& _name );
  virtual ~floating_point_image_hash_process() {}

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// Set input image
  void set_input_image( input_type const& src );
  VIDTK_INPUT_PORT( set_input_image, input_type const& );

  /// Get output image
  vil_image_view<HashType> hashed_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<HashType>, hashed_image );

private:

  // Operating modes of this filter
  enum{ SCALE_POSITIVE } algorithm_;

  // Internal parameter block
  config_block config_;
  bool round_;
  FloatType param1_;
  FloatType param2_;

  // Algorithm inputs and outputs
  input_type input_img_;
  output_type hashed_img_;
};


} // end namespace vidtk


#endif // vidtk_floating_point_image_hash_process_h_
