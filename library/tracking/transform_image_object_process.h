/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_add_image_clip_to_image_object_process_h
#define vidtk_add_image_clip_to_image_object_process_h

#include <tracking/image_object.h>

#include <vil/vil_image_view.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/transform_image_object_functors.h>

namespace vidtk
{

template< class PIXEL_TYPE >
class transform_image_object_process
 : public process
{
public:
  typedef transform_image_object_process self_type;
  typedef vbl_smart_ptr< transform_image_object_functor< PIXEL_TYPE > > function_ptr;

  transform_image_object_process( vcl_string const & name,
                                  function_ptr fun );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Set the detected source image.
  void set_image( vil_image_view<PIXEL_TYPE> const& img );
  VIDTK_INPUT_PORT( set_image, vil_image_view<PIXEL_TYPE> const& );

  void set_objects( vcl_vector< image_object_sptr > const& objs );
  VIDTK_INPUT_PORT( set_objects, vcl_vector< image_object_sptr > const& );

  /// The modified image objects.
  vcl_vector< image_object_sptr > const& objects() const;
  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

protected:
  config_block config_;
  bool disabled_;
  function_ptr function_;
  vil_image_view<PIXEL_TYPE> const * img_;
  vcl_vector< image_object_sptr > const * in_objs_;
  vcl_vector< image_object_sptr > out_objs_;
};

}

#endif //vidtk_add_image_data_to_image_object_h
