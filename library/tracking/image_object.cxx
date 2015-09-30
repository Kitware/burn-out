/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/image_object.h>
#include <vbl/vbl_smart_ptr.txx>
#include <vgl/vgl_vector_2d.h>
#include <tracking/tracking_keys.h>

namespace vidtk
{


void
image_object
::image_shift( int di, int dj )
{
  vgl_vector_2d<float_type> delta_float( di, dj );

  for( unsigned s = 0; s < boundary_.num_sheets(); ++s )
  {
    vgl_polygon<float_type>::sheet_t& sh = boundary_[s];
    for( unsigned i = 0; i < sh.size(); ++i )
    {
      sh[i] += delta_float;
    }
  }

  // this is okay because unsigned int are modulo N arithmetric
  vgl_vector_2d<unsigned> delta_unsigned( di, dj );
  bbox_.set_centroid( bbox_.centroid() + delta_unsigned );

  img_loc_[0] += di;
  img_loc_[1] += dj;

  // world_loc_ is unchanged

  // area_ is unchanged

  mask_i0_ += di;
  mask_j0_ += dj;

  // mask_ is unchanged
}

image_object_sptr
image_object
::clone()
{
  //shallow copy
  image_object_sptr copy = new image_object( *this );

  //deep copy
  copy->mask_ = vil_image_view<bool>();
  copy->mask_.deep_copy( this->mask_ );

  tracking_keys::deep_copy_property_map( this->data_,
                                         copy->data_);
  return copy;
}

} // end namespace vidtk

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::image_object );
