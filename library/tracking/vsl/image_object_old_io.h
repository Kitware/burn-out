/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_old_io_h_
#define vidtk_image_object_old_io_h_

/// \file
///
/// \brief Binary IO functions for vidtk::image_object.

#include <vsl/vsl_fwd.h>
#include <tracking/image_object.h>

namespace vidtk
{
class image_object_old : public image_object
{
public:
  image_object_old(void) { new_ptr_ = this; }
  image_object_old(image_object *ptr) : new_ptr_(ptr) { }
  image_object * new_ptr(void) { return this->new_ptr_; }
private:
  image_object *new_ptr_;
};
typedef vbl_smart_ptr<image_object_old> image_object_old_sptr;
}

void vsl_b_write( vsl_b_ostream& s, vidtk::image_object_old const& o );

void vsl_b_read( vsl_b_istream& s, vidtk::image_object_old& o );

void vsl_b_write( vsl_b_ostream& os, vidtk::image_object_old const* p );

void vsl_b_read( vsl_b_istream& is, vidtk::image_object_old*& p );


#endif // vidtk_image_object_io_h_
