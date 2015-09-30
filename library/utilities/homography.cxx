/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography.h"
#include "log.h"

namespace vidtk
{

// ----------------------------------------------------------------
/** Constructor.
 *
 * A new homography base class is created. It is set as not valid, not
 * a new reference and transform is set to identity.
 */
homography::
homography()
  :m_valid(false),
   m_newReference(false)
{
  // Set to identity for safety.
  this->set_identity(false);
}


homography::
 ~homography()
{
}


// ----------------------------------------------------------------
/** Format object to stream.
 *
 * This method formats the homography object on the specified stream.
 * Derived classes should override this method in order to produce
 * accurate output of those classes.
 */
vcl_ostream& homography::
to_stream(vcl_ostream& str) const
{
  str << "  -- transform --  ["
      << (is_valid() ? "V" : " ")  << (is_new_reference() ? "N" : " ") << "]\n"
      << "   [ " << m_homographyTransform.get(0,0) << ", " << m_homographyTransform.get(0,1) << ", " << m_homographyTransform.get(0,2) << "]\n"
      << "   [ " << m_homographyTransform.get(1,0) << ", " << m_homographyTransform.get(1,1) << ", " << m_homographyTransform.get(1,2) << "]\n"
      << "   [ " << m_homographyTransform.get(2,0) << ", " << m_homographyTransform.get(2,1) << ", " << m_homographyTransform.get(2,2) << "]"
      << vcl_endl;

  return ( str );
}


// ----------------------------------------------------------------
/** Return the transform
 *
 * This method returns the transform matrix contained within this
 * homography.
 */
const homography::transform_t& homography::
get_transform() const
{
  return ( m_homographyTransform );
}


// ----------------------------------------------------------------
/**
 *
 *
 */
homography& homography::
set_transform(const transform_t& trans)
{
  this->m_homographyTransform = trans;

  // Test if transformation is valid.
  //if( test( trans ) )
  {
    // TODO: Add a meaningful test here on *trans*.
    // Maybe singularity or others? Depends on who is using the is_valid flag.

    this->m_valid = true;
  }
  //else
  //{
  //  this->m_valid = false;
  //}

  return ( *this );
}


// ----------------------------------------------------------------
/** Set identity.
 *
 * This method sets the transformation to identity.
 */
homography& homography::
set_identity(bool is_valid)
{
  m_homographyTransform.set_identity();
  this->m_valid = is_valid;
  return ( *this );
}


// ----------------------------------------------------------------
/** Set valid state attribute.
 *
 * This method sets the \e valid attribute of this homography.
 */
homography& homography::
set_valid(bool v)
{
  this->m_valid = v;

  return ( *this );
}

// ----------------------------------------------------------------
/** Set valid state attribute.
 *
 * This method sets the \e valid attribute of this homography to false.
 *
 * @deprecated Use set_valid(false); (This is redundant.)
 */
homography& homography::
set_invalid()
{
  this->m_valid = false;

  return ( *this );
}


// ----------------------------------------------------------------
/** Is homography valid.
 *
 * This method determines if the homography is valid and returns that
 * state.  If the needs of an application require a different approach
 * to determining if a homography object is valid, then reimplement
 * this method in a derived class.
 */
bool homography::
is_valid() const
{
  return ( m_valid );
}


// ----------------------------------------------------------------
/** Set new reference attribute.
 *
 * This method sets the \e new-reference attribute for this
 * homography.  This attribute is set to indicate that there has been
 * a break in the stream of homographies and this is the first of a
 * series that is using a new reference. By convention, this bit is
 * only set in the first homography that uses the new reference.
 */
homography& homography::
set_new_reference(bool v)
{
  this->m_newReference = v;

  return ( *this );
}


// ----------------------------------------------------------------
/** Is homography based on a new reference.
 *
 * This method returns the status indicating if this homography is
 * based on a new reference image.  This only has meaning in a
 * temporal sense where there is a series of homographies. At some
 * point the reference for the homographies will change. The
 * new-reference attribute is only set in the first homography that
 * refers to the new reference.
 */
bool homography::
is_new_reference() const
{
  return ( m_newReference );
}


// ----------------------------------------------------------------
/** Equality operator
 *
 *
 */
bool homography::
operator == (homography const& rhs) const
{
  if ( ! (this->m_valid == rhs.m_valid)) return (false);
  if ( ! (this->m_newReference == rhs.m_newReference)) return (false);
  if ( ! (this->m_homographyTransform == rhs.m_homographyTransform)) return (false);

  return (true);
}


//
// Output operators for homography reference classes.
//
vcl_ostream& operator<< (vcl_ostream& str, const plane_ref_t & /*obj*/)
{ str << "[plane-ref]"; return (str); }

vcl_ostream& operator<< (vcl_ostream& str, const utm_zone_t & obj)
{
  str << "[UTM - Z: " << obj.zone() << " " << (obj.is_north() ? "N" : "S") << "]";
  return (str);
}


// ----------------------------------------------------------------
/** Multiply homographies.
 *
 * Define template to multiply two homographies with a shared inner type.
 *
 * @tparam SrcT - source reference type
 * @tparam SharedType - shared inner type
 * @tparam DestType - destination reference type
 */
template< class SrcT, class SharedType, class DestType >
vidtk::homography_T< SrcT, DestType > operator*(vidtk::homography_T< SharedType, DestType > const & l,
                                                vidtk::homography_T< SrcT, SharedType > const & r )
{
  vidtk::homography_T< SrcT, DestType > re;
  re.set_transform(l.get_transform() * r.get_transform());
  re.set_source_reference(r.get_source_reference());
  re.set_dest_reference(l.get_dest_reference());
  re.set_valid( l.is_valid() && r.is_valid() );
  return re;
}

//
// Define macros to instantiate all allowable variants of multiplying two homographies.
//

#define SET_UP_SINGLE_MULT(X) \
  template vidtk::homography_T < X, X > operator*< X, X, X >( vidtk::homography_T < X, X > const & l, vidtk::homography_T < X, X > const & r )

#define SET_UP_PAIR_MULT( X, Y ) \
  template vidtk::homography_T < X, Y > operator*< X, X, Y >( vidtk::homography_T < X, Y > const & l, vidtk::homography_T < X, X > const & r ); \
  template vidtk::homography_T < X, Y > operator*< X, Y, Y >( vidtk::homography_T < Y, Y > const & l, vidtk::homography_T < X, Y > const & r )

#define SET_UP_MID_MULT(X,Y,Z) \
  template vidtk::homography_T < X, Z > operator*< X, Y, Z >( vidtk::homography_T < Y, Z > const & l, vidtk::homography_T < X, Y > const & r )

#define SET_UP_MULT(X,Y,Z)                      \
  SET_UP_SINGLE_MULT(X);                        \
  SET_UP_SINGLE_MULT(Y);                        \
  SET_UP_SINGLE_MULT(Z);                        \
  SET_UP_PAIR_MULT( X, Y );                     \
  SET_UP_PAIR_MULT( Y, X );                     \
  SET_UP_PAIR_MULT( X, Z );                     \
  SET_UP_PAIR_MULT( Z, X );                     \
  SET_UP_PAIR_MULT( Y, Z );                     \
  SET_UP_PAIR_MULT( Z, Y );                     \
  SET_UP_MID_MULT(X, Y, Z);                     \
  SET_UP_MID_MULT(X, Z, Y);                     \
  SET_UP_MID_MULT(Y, X, Z);                     \
  SET_UP_MID_MULT(Y, Z, Y);                     \
  SET_UP_MID_MULT(Z, X, Y);                     \
  SET_UP_MID_MULT(Z, Y, X)

// Instantiate all allowable types
SET_UP_MULT(vidtk::timestamp, vidtk::plane_ref_t, vidtk::utm_zone_t);

#undef SET_UP_MULT
#undef SET_UP_MID_MULT
#undef SET_UP_PAIR_MULT
#undef SET_UP_SINGLE_MULT

// instantiate the usual suspects.
template class vidtk::homography_T < vidtk::timestamp, vidtk::timestamp >;
template class vidtk::homography_T < vidtk::timestamp, vidtk::plane_ref_t >;
template class vidtk::homography_T < vidtk::plane_ref_t, vidtk::timestamp >;
template class vidtk::homography_T < vidtk::timestamp, vidtk::utm_zone_t >;
template class vidtk::homography_T < vidtk::utm_zone_t, vidtk::timestamp >;
template class vidtk::homography_T < vidtk::plane_ref_t, vidtk::utm_zone_t >;
template class vidtk::homography_T < vidtk::utm_zone_t, vidtk::plane_ref_t >;


// This function computes the 2x2 jacobian from a 3x3 homogrpy at the given location,
// return by reference the jacobian
bool
jacobian_from_homo_wrt_loc( vnl_matrix_fixed<double, 2, 2> & jac,
              vnl_matrix_fixed<double, 3, 3> const& H,
              vnl_vector_fixed<double, 2>    const& from_loc )
{
  // The jacobian is a 2x2 matrix with entries
  // [d(f_0)/dx   d(f_0)/dy;
  //  d(f_1)/dx   d(f_1)/dy]
  //
  const double mapped_w = H(2,0)*from_loc[0] + H(2,1)*from_loc[1] + H(2,2);

  if(vcl_abs(mapped_w) < 1e-10)
  {
    log_error("Degenerate case: cannot divided by zero "
               <<"in jacobian_from_homo_wrt_loc().\n");
    return false;
  }

  // w/ respect to x
  jac(0,0) = H(0,0)*( H(2,1)*from_loc[1]+H(2,2) ) - H(2,0)*( H(0,1)*from_loc[1] + H(0,2) );
  jac(1,0) = H(1,0)*( H(2,1)*from_loc[1]+H(2,2) ) - H(2,0)*( H(1,1)*from_loc[1] + H(1,2) );
  // w/ respect to y
  jac(0,1) = H(0,1)*( H(2,0)*from_loc[0]+H(2,2) ) - H(2,1)*( H(0,0)*from_loc[0] + H(0,2) );
  jac(1,1) = H(1,1)*( H(2,0)*from_loc[0]+H(2,2) ) - H(2,1)*( H(1,0)*from_loc[0] + H(1,2) );

  jac *= (1/(mapped_w*mapped_w));

  return true;
}


} // end namespace
