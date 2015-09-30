/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_h_
#define vidtk_image_object_h_

/**
\file
\brief
 Method and field definitions for a single detected object.
*/
#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <vnl/vnl_vector_fixed.h>
#include <vgl/vgl_polygon.h>
#include <vgl/vgl_box_2d.h>
#include <vil/vil_image_view.h>
#include <utilities/property_map.h>
#include <utilities/image_histogram.h>

namespace vidtk
{

class image_object;
/// \brief Smart pointer to image_object.
typedef vbl_smart_ptr<image_object> image_object_sptr;

/// \brief Represents an object detected in a single image.
///
/// By design, it does not contain a pixel template of the object, to
/// avoid the class being templated over the pixel type. To obtain
/// the corresponding pixel template, use the templ() method.
///
/// These objects are designed to be used via smart pointers.
class image_object
  : public vbl_ref_count
{
public:
  ///Constructor
  image_object()
    : area_( -1 )
  {
  }
  ///Creates name for type double.
  typedef double float_type;

  /// A 2-d vnl_vector of float_type.
  typedef vnl_vector_fixed<float_type,2> float2d_type;

  /// A 3-d vnl_vector of float_type.
  typedef vnl_vector_fixed<float_type,3> float3d_type;

  /// A 2-d point image point.
  typedef vgl_point_2d<unsigned> image_point_type;

  /// \brief The boundary of the object.
  ///
  /// Note that these coordinates are in the orignal image
  /// coordinates, and *not* in the mask_ coordinates.  Subtract
  /// mask_i0_ and mask_j0_ to transform the boundary to the mask_
  /// coordinates.
  vgl_polygon<float_type> boundary_;

  /// \brief The bounding box of the object.
  ///
  /// These coordinates are in the original image coordinates.  The
  /// bounding box only captures the visible portion of the object,
  /// and hence will never have coordinates outside the image plane.
  ///
  /// If mask_i0_ and mask_j0_ are set, then bbox_.min_point() will
  /// have the same values.
  ///
  /// The bounding box is inclusive of min_point() but exclusive of
  /// max_point().  That is, a 2x2 object with top-left corner at
  /// coordinate (10,15) will have min_point() at (10,15) and
  /// max_point() at (12,17).
  vgl_box_2d<unsigned> bbox_;

  /// \brief A single point description of the location of the object in image coordinates.
  ///
  /// This could be the object centroid, the bottom-most point, or any
  /// other description.  The choice is left up to the algorithm
  /// detecting the objects.  Of course, related detections are
  /// expected to use a consistent definition of this location.
  float2d_type img_loc_;

  /// \brief A single point description of the location of the object in world coordinates.
  ///
  /// This is the the "world" location of the object, and is generally
  /// computed by transforming img_loc_ from image to world
  /// coordinates.  One possible transformation is an image plane to
  /// ground plane homography.
  ///
  /// It could be set to be the same as img_loc_ (with z=0) for image
  /// plane tracking.
  float3d_type world_loc_;

  /// \brief An estimate of the area of the object (optional).
  ///
  /// The semantics of area_ are left to the algorithm emitted the
  /// objects.  Examples are: the pixel count, the bounding box area,
  /// the convex hull area, the camera-adjusted area.
  ///
  /// A value of -1 indicates an unknown area.
  float_type area_;

  /// \brief The location of the top left corner of mask_, if present,
  /// in the original image.
  ///
  /// These values are valid iff mask_ is a valid image.
  unsigned mask_i0_, mask_j0_;

  /// The pixels that correspond to the object (optional).
  vil_image_view<bool> mask_;

  /*
  // Color histogram used for correspondence with tracks.
  image_color_histogram2<vxl_byte, bool> color_histogram_;

  // Grayscale histogram used for correspondence with tracks.
  image_grayscale_histogram<vxl_byte, bool> gray_histogram_;
  */

  /// Arbitrary properties that can be attached to the image objects.
  property_map data_;

  /// \brief The pixel template of the object.
  ///
  /// Make a shallow copy of the current bounding box from the given image.
  template< typename T >
  vil_image_view<T> templ( const vil_image_view<T>& image ) const;

  /// \brief The pixel template of the object.
  ///
  /// Make a shallow copy of the current bounding box from the given image and buffer.
  template< typename T >
  vil_image_view<T> templ( const vil_image_view<T>& image, unsigned buffer ) const;


  /// \brief Shift the object in pixel coordinates.
  ///
  /// This will shift all the image related attributes (bounding box,
  /// polygon, mask, image location, etc) by the given shift.  The
  /// resulting coordinates are still assumed to be valid image
  /// coordinates (for a different image).
  void image_shift( int di, int dj );

  /// \brief Create a deep copy clone.
  ///
  /// This function calls the default copy constructor following by
  /// specialized deep copy operations for the individual fields.
  image_object_sptr clone();
};


} // end namespace vidtk


#endif // vidtk_image_object_h_
