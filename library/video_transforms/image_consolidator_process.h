/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_consolidator_process_h_
#define vidtk_image_consolidator_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>

#include <vector>

template< typename PixType >
class vil_image_view;

namespace vidtk
{


/// A class which can consolidates multiple image views of type PixType
/// into a single vector of single plane vil_image_views. This is used
/// when we have a very large number of image views which we may want to
/// pipeline as future inputs or outputs.
template< typename PixType >
class image_consolidator_process
  : public process
{
public:

  typedef image_consolidator_process self_type;
  typedef vil_image_view< PixType > image_type;
  typedef std::vector< image_type > image_list_type;

  image_consolidator_process( std::string const& name );
  virtual ~image_consolidator_process() {}

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  static const unsigned MAX_INPUTS = 20;

#define ADD_INPUT_IMAGE( ID ) \
  void set_image_ ## ID ( image_type const& img ); \
  VIDTK_INPUT_PORT( set_image_ ## ID , image_type const& ); \

  ADD_INPUT_IMAGE( 1 );
  ADD_INPUT_IMAGE( 2 );
  ADD_INPUT_IMAGE( 3 );
  ADD_INPUT_IMAGE( 4 );
  ADD_INPUT_IMAGE( 5 );
  ADD_INPUT_IMAGE( 6 );
  ADD_INPUT_IMAGE( 7 );
  ADD_INPUT_IMAGE( 8 );
  ADD_INPUT_IMAGE( 9 );
  ADD_INPUT_IMAGE( 10 );
  ADD_INPUT_IMAGE( 11 );
  ADD_INPUT_IMAGE( 12 );
  ADD_INPUT_IMAGE( 13 );
  ADD_INPUT_IMAGE( 14 );
  ADD_INPUT_IMAGE( 15 );
  ADD_INPUT_IMAGE( 16 );
  ADD_INPUT_IMAGE( 17 );
  ADD_INPUT_IMAGE( 18 );
  ADD_INPUT_IMAGE( 19 );
  ADD_INPUT_IMAGE( 20 );

#undef ADD_INPUT_IMAGE

  void set_input_list( image_list_type const& list );
  VIDTK_INPUT_PORT( set_input_list, image_list_type const& );

  image_list_type output_list() const;
  VIDTK_OUTPUT_PORT( image_list_type, output_list );

private:

  // Possible inputs
  image_type input_images_[ MAX_INPUTS ];
  image_list_type input_list_;

  // Internal parameter block
  config_block config_;
  bool disabled_;

  // Algorithm outputs
  image_list_type output_list_;
};


} // end namespace vidtk


#endif // vidtk_image_consolidator_process_h_
