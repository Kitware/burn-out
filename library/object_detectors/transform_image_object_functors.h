/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_add_image_clip_to_image_object_h
#define vidtk_add_image_clip_to_image_object_h

#include <tracking_data/image_object.h>

#include <vil/vil_image_view.h>

#include <utilities/config_block.h>

#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>

namespace vidtk
{

template < class PIXEL_TYPE >
class transform_image_object_functor
  : public vbl_ref_count
{
public:
  virtual ~transform_image_object_functor() { }

  virtual config_block params() const
  { config_block blk; return blk; }

  virtual bool set_params( config_block const& )
  { return true; }

  ///If child classes need different types of input, one should add
  ///new function that does nothing here.  Then implement the input
  ///functions you need in your dirived class.  Reason for new do
  ///nothing functions is to limit the number of classes that need
  ///to be changed.  There could be children that exists outside of
  ///this file.
  virtual bool set_input( vil_image_view< PIXEL_TYPE > const* )
  { return true; }

  ///If the input does not exist, you should return the input unchanged.
  virtual image_object_sptr operator()( image_object_sptr ) const = 0;
};


template < class PIXEL_TYPE >
class add_image_clip_to_image_object :
  public transform_image_object_functor< PIXEL_TYPE >
{
public:
  add_image_clip_to_image_object();
  virtual config_block params() const;
  virtual bool set_params( config_block const& blk );
  virtual bool set_input( vil_image_view< PIXEL_TYPE > const* );
  virtual image_object_sptr operator()( image_object_sptr in ) const;

protected:
  unsigned int buffer_;
  vil_image_view< PIXEL_TYPE > const* img_;
};

}

#endif //vidtk_add_image_data_to_image_object_h
