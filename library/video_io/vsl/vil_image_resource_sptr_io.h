/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vsl_vil_image_resource_sptr_io_h
#define vidtk_vsl_vil_image_resource_sptr_io_h

/// \file
///
/// \brief Binary IO functions for vil_image_resource_str.

#include <vsl/vsl_fwd.h>
#include <vil/vil_image_resource_sptr.h>

//I am putting these in vidtk because vil_image_resource is much more generic in
//vxl than we are currently using it (currently as a generic in memory image).
//Because of this, I am not completely comfortable including this binary reading
//and writing in vxl.  Also, we cannot use the normal names because those,
//will call the vsl_b_write for smart pointers, but we need to call
//vil_new_image_resource_of_view, which would call result in a dereference and
//delete when scope is left in read.  (Juda)
void write_img_resource_b( vsl_b_ostream& s, vil_image_resource_sptr const & v );
void read_img_resource_b( vsl_b_istream& s, vil_image_resource_sptr & v );

#endif //#ifndef vidtk_vsl_vil_image_resource_sptr_io_h
