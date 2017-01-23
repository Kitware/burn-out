/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_h_
#define vidtk_image_object_h_

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <vnl/vnl_vector_fixed.h>
#include <vgl/vgl_polygon.h>
#include <vgl/vgl_box_2d.h>
#include <vil/vil_image_view.h>
#include <vil/vil_image_resource.h>

#include <utilities/property_map.h>
#include <utilities/image_histogram.h>
#include <utilities/string_friendly_enum.h>
#include <utilities/vidtk_coordinate.h>
#include <utilities/geo_coordinate.h>

#include <boost/shared_ptr.hpp>

#include <limits>

namespace vidtk
{

// This value is unsigned, but intentionally set to max_int
// Keeping constrained to int aids with storage and retrieval
// If this value is changed to unsigned, please update the database
// column sizes and ensure dashboard tests still pass.
// We can currently support 32-bit - 1 image borders with this value.
static const unsigned int INVALID_IMG_CHIP_BORDER = std::numeric_limits<int>::max();

class image_object;

/** @brief Smart pointer to image_object.
 */
typedef vbl_smart_ptr<image_object> image_object_sptr;


// ----------------------------------------------------------------
  /**
 * @brief Represents an object detected in a single image.
 *
 * This class represents the detection of something interesting in an
 * image.
 *
 * By design, it does not contain a pixel template of the object, to
 * avoid the class being templated over the pixel type. To obtain
 * the corresponding pixel template, use the templ() method.
 *
 * These objects are designed to be used via smart pointers.
 *
 * \note Future work and additions should not make any additional
 * public data items, but should supply accessor methods. For those
 * with the desire, existing public data fields should be made private
 * and accessors should be added.
 */

class image_object
  : public vbl_ref_count
{

public:

  ///Creates name for type double.
  typedef double float_type;

  /// A 2-d point image point.
  typedef vgl_point_2d<unsigned> image_point_type;

  typedef vil_image_view< float > heat_map_type;
  typedef boost::shared_ptr< heat_map_type > heat_map_sptr;

  typedef std::pair<float, float> intensity_distribution_type;

  image_object();

  /**
   * Getter and setter for boundary
   **/
  vgl_polygon< float_type > const& get_boundary() const;
  void set_boundary( vgl_polygon< float_type > const& b );

  /**
   * Getter and setter foor bounding_box
   **/
  vgl_box_2d< unsigned > const& get_bbox() const;
  void set_bbox( vgl_box_2d< unsigned > const& b );
  void set_bbox( unsigned min_x, unsigned max_x, unsigned min_y, unsigned max_y );

  /**
   * Getter and setter for area
   **/
  float_type get_area() const;
  void set_area( float_type a );

  /**
   * Getter and setter for image_area
   **/
  float_type get_image_area() const;
  void set_image_area( float_type a );

  /**
   * Convenience getter for mask_origin info
   **/
  unsigned get_mask_i0() const;
  unsigned get_mask_j0() const;

  /**
   * Getter and setter for used_in_track property
   **/
  bool is_used_in_track() const;
  void set_used_in_track( bool u );

  /**
   * Getter and setter for image_histogram property
   **/
  bool get_histogram( image_histogram& ) const;
  void set_histogram( const image_histogram& );

  /**
   * Getter and setter for intensity_distribution property
   **/
  bool get_intensity_distribution( intensity_distribution_type& dist ) const;
  void set_intensity_distribution( const intensity_distribution_type& dist );


  /**
   *  Getter and setter for the geo location of this detection.
   */
  geo_coord::geo_coordinate_sptr const get_geo_loc() const;
  void set_geo_loc(geo_coord::geo_coordinate_sptr const geo);


  /** Get image location.
   *
   * Get the location of the image object. Location is returned in
   * image coordinates.
   *
   * This could be the object centroid, the bottom-most point, or any
   * other description.  The choice is left up to the algorithm
   * detecting the objects.  Of course, related detections are
   * expected to use a consistent definition of this location.
   *
   * @param[out] x - x coordinate of location
   * @param[out] y - y coordinate of location
   */
  vidtk_pixel_coord_type const& get_image_loc() const;

  /**
   *  Sets the image location
   *  The options for this function are either setting from an existing img_loc
   *  or, populating out img_loc values from provided values.
   **/
  void set_image_loc(vidtk_pixel_coord_type const& loc);
  void set_image_loc(float_type x, float_type y);

  /**
   *  Getter and setter for the world location
   *  The options for this function are either setting from an existing world_loc
   *  or, populating out world_loc values from provided values.
   **/
  tracker_world_coord_type const& get_world_loc() const;
  void set_world_loc(tracker_world_coord_type const& loc);
  void set_world_loc(float_type x, float_type y, float_type z);

  /**
   * @brief Get object mask (optional).
   *
   * This method returns the binary object mask and origin, if there
   * is one present.
   *
   * @param[out] mask Object mask is returned here.
   * @param[out] origin Pixel coordinates of top left corner of mask
   *
   * @return \b true if there is a mask present, \b false if no mask.
   */
  bool get_object_mask( vil_image_view< bool >& mask, image_point_type& origin ) const;

  /**
   * @brief Set object mask.
   *
   * This method sets the specified binary object mask into this
   * object detection. The mask must be clipped to the object bounding
   * box (bbox_) by calling clip_image_for_detection(). The origin of
   * this mask is specified relative to the original image.
   *
   * @param mask Binary mask image
   * @param origin Origin
   */
  void set_object_mask( vil_image_view< bool >const& mask, image_point_type const& origin );

  /** @brief Source of image object.
   *
   * This enum specifies the detector type that created this image
   * object. If you modify this list of sources, don't forget to
   * update the convert_souce_code() method also.
   */
  STRING_FRIENDLY_ENUM( source_code, 0,
   (UNKNOWN)                    ///< unknown or unspecified source
   (MOTION)                     ///< created by motion detector
   (SALIENCY)                   ///< created by saliency detector
   (APPEARANCE)                 ///< created by appearance detector
  )

  /**
   * @brief Set origin of image object.
   *
   * This method sets the codes that indicate the creator of this
   * image object.
   *
   * @param s Detector type code
   * @param inst Process instance name
   */
  void set_source( source_code s, std::string const& inst );
  source_code get_source_type() const;
  std::string get_source_name() const;

  /**
   * @brief Shift the object in pixel coordinates.
   *
   * This will shift all the image related attributes (bounding box,
   * polygon, mask location, image location) by the given shift.
   * The resulting coordinates are still assumed to be valid image
   * coordinates (for a different image).
   *
   * @param di Number of pixels to shift in X.
   * @param dj Number of pixels to shift in Y.
   */
  void image_shift( int di, int dj );

  /**
   * @brief Create a deep copy clone of this object
   *
   * This function calls the default copy constructor following by
   * specialized deep copy operations for the individual fields.
   *
   * @return Smart pointer to a new object
   */
  image_object_sptr clone();

  ///@{
  /**
   * @brief Access confidence value for this detection.
   *
   */
  double get_confidence() const;
  void set_confidence( double val );
  ///@}

  //
  // As part of the strategy to migrate away from the property map,
  // these accessors provide a top level interface to the data. The
  // data will move someday, and if you use this API, you won't have
  // to fix your code when it does.
  //


  /**
   * @brief Get image chip for this detection.
   *
   * The portion of the image associated with the detection is
   * returned. The image returned may have been expanded from what was
   * the object bounding box by some number of pixels on all four
   * edges. The amount of this expansion is returned as the \e border
   * value. A value of zero indicates no expansion.
   *
   * \note The current implementation will truncate the image chip if
   * it is expanded beyond the bounds of the original image.
   *
   * @param[out] image Reference to image chip
   * @param[out] border Size of border added to chip, in pixels
   *
   * @return \b true is returned if there is an image chip in the
   * detection, \b false is returned if there is no chip.
   */
  bool get_image_chip( vil_image_resource_sptr& image, unsigned int& border ) const;


  /**
   * @brief Set image chip for this detection.
   *
   * The clipped portion of the image that is associated with this
   * detection is saved in the image object. If a border is needed
   * for this clipped image, it must be applied before being passed
   * in.
   *
   * Since this method takes a image resource, a small example is in
   * order.
   * \code
      vil_image_view< PixType > temp;
      temp.deep_copy( clip_image_for_detection( img, objs[k], add_image_info_buffer_amount_) );

      vil_image_resource_sptr data = vil_new_image_resource_of_view(temp);
      objs[k]->set_image_chip( data, add_image_info_buffer_amount_ );
   * \endcode
   * @param image Image to be saved for detection
   * @param border Number of pixels that have been added around the border
   */
  void set_image_chip(
    vil_image_resource_sptr image, unsigned int border = INVALID_IMG_CHIP_BORDER);

  /**
   * @brief Get detection heat map.
   *
   * This method returns a reference to the heat map for this
   * detection. This map is a 2D "image" of detection confidence (or
   * other metric, depending on detector) for the image pixels that
   * represent a detection. This heat map may be larger that the
   * associated image chip, but generally should never be smaller. Use
   * the size of the image and the origin, compared with the image
   * chip, to determine how the heat map overlaps the image chip.
   *
   * If the heat map is not available, the return parameters are
   * unmodified.
   *
   * @param[out] map Image of detection confidence values.
   * @param[out] origin Upper left origin of heat map relative to image.
   *
   * @return \b true if heat map is present, \b false if no heat map available.
   */
  bool get_heat_map( heat_map_sptr& map, image_point_type& origin ) const;

  /**
   * @brief Set heat map for this detection
   *
   * The specified heat map is set into this detection. Since the heat
   * map is managed by a shared_ptr, it must be allocated from the
   * heap.
   *
   * @param map Detection heat map.
   * @param origin Origin of map relative to original image.
   */
  void set_heat_map( heat_map_sptr map, image_point_type const& origin );


private:

  enum source_code source_type_;

  /** @brief The boundary of the object.
   *
   * Note that these coordinates are in the orignal image
   * coordinates, and *not* in the mask_ coordinates.  Subtract
   * mask_i0_ and mask_j0_ to transform the boundary to the mask_
   * coordinates.
   *
   * The current boundary computation is computed for the convex
   * representation of the object (using convex_hull). As such,
   * it will not be a fully accurate representation of the object.
   */
  vgl_polygon< float_type > boundary_;

  /** @brief The bounding box of the object.
   *
   * These coordinates are in the original image coordinates.  The
   * bounding box only captures the visible portion of the object,
   * and hence will never have coordinates outside the image plane.
   *
   * If mask_i0_ and mask_j0_ are set, then bbox_.min_point() will
   * have the same values.
   *
   * The bounding box is inclusive of min_point() but exclusive of
   * max_point().  That is, a 2x2 object with top-left corner at
   * coordinate (10,15) will have min_point() at (10,15) and
   * max_point() at (12,17).
   */
  vgl_box_2d< unsigned > bbox_;

  /** @brief An estimate of the area of the object in world coordinate
   * system (optional). Units would typically be in meters-squared.
   *
   * A value of -1 indicates an unknown area.
   */
  float_type area_;

  /** @brief An estimate of the image area of the object (optional),
   * i.e., pixel count.
   *
   * A value of -1 indicates an unknown area.
   */
  float_type img_area_;

  /// @brief The location of the top left corner of mask_, if present,
  /// in the original image.
  ///
  /// These values specify the upper left corner of the object
  /// mask. Note that if these are set , they are the same as the
  /// upper left corner of the bounding box.
  ///
  /// \note Not clear why mask needs separate origin information.
  ///
  /// These values are valid iff mask_ is a valid image.
  unsigned mask_i0_, mask_j0_;

  /**
   * @brief The pixels that correspond to the object (optional).
   *
   * This field contains a binary mask of the detected object. The one
   * bits indicate object while the zero bits are background. This
   * mask is sized the same as the bounding box.
   *
   * \todo Move to private section. Use accessors.
   */
  vil_image_view< bool > mask_;

  /**
   * @brief Arbitrary properties that can be attached to the image objects.
   */
  property_map data_;

  /** @brief A single point description of the location of the object in image coordinates.
   *
   * This could be the object centroid, the bottom-most point, or any
   * other description.  The choice is left up to the algorithm
   * detecting the objects.  Of course, related detections are
   * expected to use a consistent definition of this location.
   *
   * \deprecated Direct access to this field is discouraged.
   */
  vidtk_pixel_coord_type img_loc_;

  /** @brief A single point description of the location of the object in world coordinates.
   *
   * This is the the "world" location of the object, and is generally
   * computed by transforming img_loc_ from image to world
   * coordinates.  One possible transformation is an image plane to
   * ground plane homography.
   *
   * It could be set to be the same as img_loc_ (with z=0) for image
   * plane tracking.
   */
  tracker_world_coord_type world_loc_;

  std::string source_instance_name_;

  geo_coord::geo_coordinate_sptr geo_loc_;

  /**
   * Confidence or quality of object detection.  Higher confidence
   * values indicate better match to objects as defined in the
   * configuration. The range is guaranteed to be [0, 1]
   * (i.e. 0 - low confidence; 1.0 - high confidence).
   */
  double confidence_;

  heat_map_sptr heat_map_;
  image_point_type heat_map_origin_;
};

} // end namespace vidtk

#endif // vidtk_image_object_h_
