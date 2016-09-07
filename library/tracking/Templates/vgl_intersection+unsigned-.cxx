/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include <vgl/vgl_intersection.hxx>

//These are located here since tracks are using unsigned int bouding box.
//Matt L and Juda think we should not be using unsigned int, instead just int.
//Main reason is some functions could cause the values to become negative.
//NOTE: The instatiation template function does not (at the time of
//      writing this) link when using unsigned int.
template vgl_box_2d<unsigned int > vgl_intersection(vgl_box_2d<unsigned int > const&,vgl_box_2d<unsigned int > const&);
