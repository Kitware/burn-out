/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_HOMOGRAPHY_H_
#define _VIDTK_HOMOGRAPHY_H_


#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <vcl_ostream.h>

namespace vidtk
{

// ----------------------------------------------------------------
/** Base class for homographies.
 *
 * This class contains all common functionality that is not
 * templatable. Consider this like a mixin and not the base of a
 * polymorphic class hierarchy.
 */

class homography
{
public:
  // -- TYPES --
  typedef vgl_h_matrix_2d< double > transform_t;

  // -- CONSTRUCTORS --
  virtual ~homography();

  homography(homography const & h)
    : m_valid(h.m_valid), m_newReference(h.m_newReference),
      m_homographyTransform(h.m_homographyTransform)
  { }

  // -- ACCESSORS
  const transform_t& get_transform() const;
  virtual vcl_ostream& to_stream(vcl_ostream& str) const;
  virtual bool is_valid() const;
  virtual bool is_new_reference() const;

  // -- MANIPULATORS --
  homography& set_transform(const transform_t& trans);
  homography& set_identity(bool is_valid);

  homography& set_valid (bool v);
  homography& set_invalid (); // deprecated - redundant
  homography& set_new_reference (bool v);

  bool operator == ( homography const& rhs ) const;
  bool operator != ( homography const& rhs )  const { return ( !( this->operator== (rhs) ) ); }


protected:
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
class plane_ref_t {
public:
  bool operator== (plane_ref_t const& /*rhs*/) const { return true; }

 };


// ----------------------------------------------------------------
/// \brief UTM zone (without easting & northing) saved as reference of UTM plane.
class utm_zone_t
{
public:
  utm_zone_t() : m_zone(0), m_is_north(true) {};
  utm_zone_t( int z, bool in )
    : m_zone( z ),
      m_is_north( in )
  {}
  int zone() const { return m_zone; }
  bool is_north() const { return m_is_north; }
  void set_zone(int z) { m_zone = z; }
  void set_is_north(bool v) { m_is_north = v; }

  bool operator == (utm_zone_t const& rhs) const
  { return ((this->m_zone == rhs.m_zone) && (this->m_is_north == rhs.m_is_north)); }


private:
  int m_zone;
  bool m_is_north;
};


// ----------------------------------------------------------------
/** Homography template.
 *
 * This class represents a homography transform from one coordinate
 * system to another.
 *
 * A homography is considered valid after a transform matrix has been
 * assigned.
 */

template< class ST, class DT >
class homography_T
: public homography
{
public:
  typedef homography_T < ST, DT >  self_t;

  // -- CONSTRUCTORS --
  homography_T() { }
  virtual ~homography_T() { }
  homography_T( homography_T const & h)
    : homography(h),
      m_sourceReference(h.m_sourceReference),
      m_destReference(h.m_destReference)
  {}

  // -- ACCESSORS --
  ST get_source_reference() const { return this->m_sourceReference; }
  DT get_dest_reference() const { return this->m_destReference; }
  virtual vcl_ostream& to_stream(vcl_ostream& str) const
  {
    str << "Homography - source: " << m_sourceReference
        << "  dest: " << m_destReference << "\n";
    return vidtk::homography::to_stream(str);
  }

  // -- MANIPULATORS --
  self_t & set_source_reference(const ST& r) { this->m_sourceReference = r;
                                                  return *this; }
  self_t & set_dest_reference(const DT& r) { this->m_destReference = r;
                                                return *this; }
  homography_T< DT, ST > get_inverse()
  {
    homography_T< DT, ST > result;
    result.set_transform(this->get_transform().get_inverse());
    result.set_source_reference(this->get_dest_reference());
    result.set_dest_reference(this->get_source_reference());
    result.set_valid(this->is_valid());
    return result;
  }

  bool operator== ( self_t const& rhs ) const
  {
    if ( !( this->homography::operator== (rhs) ) ) return false;
    if ( !( this->m_sourceReference == rhs.m_sourceReference) ) return false;
    if ( !( this->m_destReference == rhs.m_destReference) ) return false;
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
typedef homography_T < timestamp, timestamp >     image_to_image_homography;
typedef homography_T < timestamp, plane_ref_t >   image_to_plane_homography;
typedef homography_T < plane_ref_t, timestamp >   plane_to_image_homography;
typedef homography_T < plane_ref_t, plane_ref_t > plane_to_plane_homography;
typedef homography_T < timestamp, utm_zone_t >    image_to_utm_homography;
typedef homography_T < utm_zone_t, timestamp >    utm_to_image_homography;
typedef homography_T < plane_ref_t, utm_zone_t >  plane_to_utm_homography;
typedef homography_T < utm_zone_t, plane_ref_t >  utm_to_plane_homography;
//@}

vcl_ostream& operator<< (vcl_ostream& str, const plane_ref_t & obj);
vcl_ostream& operator<< (vcl_ostream& str, const utm_zone_t & obj);

inline vcl_ostream& operator<< (vcl_ostream& str, const homography & obj)
{ obj.to_stream(str); return str; }


template< class SrcT, class SharedType, class DestType >
vidtk::homography_T< SrcT, DestType > operator*(vidtk::homography_T< SharedType, DestType > const & l, vidtk::homography_T< SrcT, SharedType > const & r );


bool
jacobian_from_homo_wrt_loc( vnl_matrix_fixed<double, 2, 2> & jac,
              vnl_matrix_fixed<double, 3, 3> const& H,
              vnl_vector_fixed<double, 2>    const& from_loc );

} // end namespace


#endif /* _VIDTK_HOMOGRAPHY_H_ */
