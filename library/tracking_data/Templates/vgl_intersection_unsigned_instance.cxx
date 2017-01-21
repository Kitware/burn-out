/*ckwg +5
 * Copyright 2011-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include <vgl/vgl_intersection.hxx>

// These are located here since tracks are using unsigned int bouding box.
// Later on, it might be prudent to convert all unsingned uses to be signed.
template vgl_box_2d<unsigned> vgl_intersection(vgl_box_2d<unsigned> const&, vgl_box_2d<unsigned> const&);
