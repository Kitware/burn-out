/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_h_
#define vidtk_homography_h_


#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/fuzzy_compare.h>
#include <utilities/timestamp.h>

#include <ostream>

namespace vidtk
{

// ----------------------------------------------------------------
/** Base class for homographies.
 *
 * This class contains all common functionality that is not
 * templatable. Consider this like a mixin and not the base of a
 * polymorphic class hierarchy.
 *
 * Homographies should be based on the standard set of type names:
 *
 * @sa image_to_image_homography image_to_plane_homography
 * plane_to_image_homography plane_to_plane_homography
 * image_to_utm_homography utm_to_image_homography
 * plane_to_utm_homography utm_to_plane_homography
 */
class homography
{
public:
  // -- TYPES --
  typedef vgl_h_matrix_2d< double > transform_t;

  // -- CONSTRUCTORS --
  virtual ~homography();

  homography( homography const& h )
    : m_valid( h.m_valid ),
      m_newReference( h.m_newReference ),
      m_homographyTransform( h.m_homographyTransform )
  { }

  /**
   * @brief Return the transform
   *
   * This method returns the transform matrix contained within this
   * homography.
   */
  const transform_t& get_transform() const;

  /**
   * @brief Format object to stream.
   *
   * This method formats the homography object on the specified stream.
   * Derived classes should override this method in order to produce
   * accurate output of those classes.
   */
  virtual std::ostream& to_stream( std::ostream& str ) const;

  /**
   * @brief Is homography valid.
   *
   * This method determines if the homography is valid and returns that
   * state.  If the needs of an application require a different approach
   * to determining if a homography object is valid, then reimplement
   * this method in a derived class.
   */
  virtual bool is_valid() const;

  /**
   * @brief Is homography based on a new reference.
   *
   * This method returns the status indicating if this homography is
   * based on a new reference image.  This only has meaning in a
   * temporal sense where there is a series of homographies. At some
   * point the reference for the homographies will change. The
   * new-reference attribute is only set in the first homography that
   * refers to the new reference.
   */
  virtual bool is_new_reference() const;

  /**
   * @brief Set transform matrix.
   *
   * This method sets a new transform matrix for this homography. The
   * matrix is copied into this homography.
   *
   * @param trans Reference to transform matrix.
   */
  homography& set_transform( const transform_t& trans );

  /**
   * @brief Set homography transform to identity.
   *
   * This method sets the homography transform to identity and sets
   * the transform valid attribute as specified.
   *
   * @param is_valid New value for transform valid attribute.
   */
  homography& set_identity( bool is_valid );

  /**
   * @brief Normalize homography transform.
   *
   * This method normalizes the homomgraphy transform.
   */
  homography& normalize();

  /**
   * @brief Set valid state attribute.
   *
   * This method sets the \e valid attribute of this homography.
   *
   * @param v New state of valid attribute.
   */
  homography& set_valid( bool v );

  /**
   * @brief Set new reference attribute.
   *
   * This method sets the \e new-reference attribute for this
   * homography.  This attribute is set to indicate that there has been
   * a break in the stream of homographies and this is the first of a
   * series that is using a new reference. By convention, this bit is
   * only set in the first homography that uses the new reference.
   *
   * @param v New value for attribute
   */
  homography& set_new_reference( bool v );

  bool operator==(homography const& rhs) const;
  bool operator!=( homography const& rhs )  const { return ! ( this->operator==( rhs ) );  }

  /**
   * @brief Equality comparison method
   *
   * Unlike the == operator, an epsilon parameter can be specified when
   * comparing members that have floating point data type.
   *
   * @param epsilon Optional difference allowed between floating point values
  **/
  bool is_equal(homography const& rhs,
    double epsilon = fuzzy_compare<double>::epsilon()) const;

protected:
  /**
   * @brief Constructor.
   *
   * A new homography base class is created. It is set as not valid, not
   * a new reference and transform is set to identity.
   */
  homography(); // CTOR


private:
  /// Set if this homography is valid. The actual semantics for \e
  /// valid are determined by the derived classes.
  bool m_valid;

  /// Set if new reference frame.  This indicator is set after a
  /// homography break indicating the start of a new set of
  /// homographies using a new reference.
  bool m_newReference;

  /// Homography transform.  This matrix represents the homography
  /// transform used to convert between two coordinate frames.
  transform_t m_homographyTransform;

}; // end class homography


// ----------------------------------------------------------------
/// \brief A placeholder for reference data for an arbitrary plane.
///
/// Currently we are not using reference data for an arbitrary plane
/// which is currently the "tracking world" plane.
class plane_ref_t
{
public:
  bool operator==( plane_ref_t const& /*rhs*/ ) const { return true; }

};


// ----------------------------------------------------------------
/// \brief UTM zone (without easting & northing) saved as reference of UTM plane.
class utm_zone_t
{
public:
  utm_zone_t() : m_zone( 0 ), m_is_north( true ) { }

  utm_zone_t( int z, bool in )
    : m_zone( z ),
      m_is_north( in )
  { }

  int zone() const { return m_zone; }

  bool is_north() const { return m_is_north; }

  void set_zone( int z ) { m_zone = z; }

  void set_is_north( bool v ) { m_is_north = v; }

  bool operator==( utm_zone_t const& rhs ) const
  { return ( this->m_zone == rhs.m_zone ) && ( this->m_is_north == rhs.m_is_north );  }


private:
  int m_zone;
  bool m_is_north;
};


// ----------------------------------------------------------------
/** Homography template.
 *
 * This class represents a homography transform from one coordinate
 * system, the source to another, the destination.
 *
 * A homography is considered valid after a transform matrix has been
 * assigned.
 *
 * @sa image_to_image_homography image_to_plane_homography
 * plane_to_image_homography plane_to_plane_homography
 * image_to_utm_homography utm_to_image_homography
 * plane_to_utm_homography utm_to_plane_homography
 */
template < class ST, class DT >
class homography_T
  : public homography
{
public:
  typedef homography_T< ST, DT >  self_t;

  /**
   * @brief Default constructor.
   *
   * @return Default constructed homography.
   */
  homography_T() { }

  /**
   * @brief Copy constructor.
   *
   * @param h Source homography.
   *
   * @return New homography.
   */
  homography_T( homography_T const& h )
    : homography( h ),
      m_sourceReference( h.m_sourceReference ),
      m_destReference( h.m_destReference )
  { }

  virtual ~homography_T() { }

  /**
   * @brief Get source reference for homography.
   *
   * This method returns the reference object for the source
   * coordinate system.
   *
   * @return The source reference object.
   */
  ST get_source_reference() const { return this->m_sourceReference; }

  /**
   * @brief Getdestination reference for homography.
   *
   * This method returns the reference object for the destination
   * coordinate system.
   *
   * @return The destination reference object.
   */
  DT get_dest_reference() const { return this->m_destReference; }

  /**
   * @brief Format homography to stream.
   *
   * This method formats this homography to the specified stream.
   *
   * @param str Stream to format on.
   *
   * @return stream reference.
   */
  virtual std::ostream& to_stream( std::ostream& str ) const
  {
    str << "Homography - source: " << m_sourceReference
        << "  dest: " << m_destReference << "\n";
    return vidtk::homography::to_stream( str );
  }

  /**
   * @brief Set source reference object.
   *
   * @param r Source reference object.
   */
  self_t& set_source_reference( const ST& r )
  {
    this->m_sourceReference = r;
    return *this;
  }

  /**
   * @brief Set destination reference object.
   *
   * @param r Destination reference object.
   */
  self_t& set_dest_reference( const DT& r )
  {
    this->m_destReference = r;
    return *this;
  }

  /**
   * @brief Get inverse of homography.
   *
   * This method returns inverse of this homography. The source and
   * destination objects are reversed too.
   *
   * @return Inverse homography
   */
  homography_T< DT, ST > get_inverse()
  {
    homography_T< DT, ST > result;
    result.set_transform( this->get_transform().get_inverse() );
    result.set_source_reference( this->get_dest_reference() );
    result.set_dest_reference( this->get_source_reference() );
    result.set_valid( this->is_valid() );
    return result;
  }

  /**
   * @brief Equality operator.
   *
   * This operator determines if two homographies are equivalent. The
   * transform and reference objects must be the same.
   *
   * @param rhs Homography on right side of equality operator.
   *
   * @return \b true if homographies are equal, \b false if not.
   */
  bool operator==( self_t const& rhs ) const
  {
    return this->is_equal(rhs, 0.0);
  }

  /**
  * @brief Equality comparison.
  *
  * This determines if two homographies are equivalent within an epsilon tolerance.
  * The transform and reference objects must be the same however.
  *
  * @param rhs Homography on right side of equality operator.
  * @param epsilon Tolerance between the two homographies
  *
  * @return \b true if homographies are equals, \b false if not.
  */
  bool is_equal(self_t const& rhs,
    double epsilon = fuzzy_compare<double>::epsilon()) const
  {
    if (!(homography::is_equal(rhs, epsilon))) { return false; }
    if (!(this->m_sourceReference == rhs.m_sourceReference)) { return false; }
    if (!(this->m_destReference == rhs.m_destReference)) { return false; }
    return true;
  }

private:
  ST m_sourceReference;
  DT m_destReference;
};

// Create all allowable variants through typedefs
//@{
/** Specific homography type. This homography type represents a
 * coordinate conversion from one set of coordinates to another.
 */
typedef homography_T< timestamp, timestamp >     image_to_image_homography;
typedef homography_T< timestamp, plane_ref_t >   image_to_plane_homography;
typedef homography_T< plane_ref_t, timestamp >   plane_to_image_homography;
typedef homography_T< plane_ref_t, plane_ref_t > plane_to_plane_homography;
typedef homography_T< timestamp, utm_zone_t >    image_to_utm_homography;
typedef homography_T< utm_zone_t, timestamp >    utm_to_image_homography;
typedef homography_T< plane_ref_t, utm_zone_t >  plane_to_utm_homography;
typedef homography_T< utm_zone_t, plane_ref_t >  utm_to_plane_homography;
//@}

std::ostream& operator<<( std::ostream& str, const plane_ref_t& obj );
std::ostream& operator<<( std::ostream& str, const utm_zone_t& obj );

inline std::ostream&
operator<<( std::ostream& str, const homography& obj )
{ obj.to_stream( str ); return str; }


#define VIDTK_HOMOGRAPHY_TYPES(CALL)            \
  CALL(image_to_image_homography)               \
  CALL(image_to_plane_homography)               \
  CALL(plane_to_image_homography)               \
  CALL(plane_to_plane_homography)               \
  CALL(image_to_utm_homography)                 \
  CALL(utm_to_image_homography)                 \
  CALL(plane_to_utm_homography)                 \
  CALL(utm_to_plane_homography)

} // end namespace

#endif /* _VIDTK_HOMOGRAPHY_H_ */
