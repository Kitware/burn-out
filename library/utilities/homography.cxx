/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography.h"

#include <iomanip>

namespace vidtk
{

// ----------------------------------------------------------------
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
std::ostream& homography::
to_stream(std::ostream& str) const
{
  str << "  -- transform --  ["
      << (is_valid() ? "V" : " ")  << (is_new_reference() ? "N" : " ") << "]\n"
      << std::setprecision(20)
      << "   [ " << m_homographyTransform.get(0,0) << ", " << m_homographyTransform.get(0,1) << ", " << m_homographyTransform.get(0,2) << "]\n"
      << "   [ " << m_homographyTransform.get(1,0) << ", " << m_homographyTransform.get(1,1) << ", " << m_homographyTransform.get(1,2) << "]\n"
      << "   [ " << m_homographyTransform.get(2,0) << ", " << m_homographyTransform.get(2,1) << ", " << m_homographyTransform.get(2,2) << "]"
      << std::endl;

  return str;
}


// ----------------------------------------------------------------
const homography::transform_t& homography::
get_transform() const
{
  return m_homographyTransform;
}


// ----------------------------------------------------------------
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

  return *this;
}


// ----------------------------------------------------------------
/** Set identity.
 *
 * This method sets the transformation to identity.
 */
homography& homography::
set_identity(bool _is_valid)
{
  m_homographyTransform.set_identity();
  this->m_valid = _is_valid;
  return *this;
}


// ----------------------------------------------------------------
homography&
homography::
normalize()
{
  vnl_matrix_fixed< double, 3, 3 > matrix = m_homographyTransform.get_matrix();
  double const& factor = matrix[2][2];

  // If normalizing factor is zero, then already normalized.
  if ( 0 == factor )
  {
    return *this;
  }

  matrix /= factor;

  m_homographyTransform = transform_t( matrix );

  return *this;
}


// ----------------------------------------------------------------
homography& homography::
set_valid(bool v)
{
  this->m_valid = v;

  return *this;
}


// ----------------------------------------------------------------
bool homography::
is_valid() const
{
  return m_valid;
}


// ----------------------------------------------------------------
homography& homography::
set_new_reference(bool v)
{
  this->m_newReference = v;

  return *this;
}


// ----------------------------------------------------------------
bool homography::
is_new_reference() const
{
  return m_newReference;
}

// ----------------------------------------------------------------
/** Equality operator
 *
 *
 */
bool homography::
operator == (homography const& rhs) const
{
  return this->is_equal(rhs, 0.0);
}

// ----------------------------------------------------------------
bool homography::is_equal(homography const& rhs, double epsilon) const
{
  if (!(this->m_valid == rhs.m_valid)) return (false);
  if (!(this->m_newReference == rhs.m_newReference)) return (false);

  vnl_matrix<double> hmTransform = this->m_homographyTransform.get_matrix();
  vnl_matrix<double> rhsHmTransform = rhs.m_homographyTransform.get_matrix();
  if (!hmTransform.is_equal(rhsHmTransform, epsilon))
    return false;

  return true;
}



//
// Output operators for homography reference classes.
//
std::ostream& operator<< (std::ostream& str, const plane_ref_t & /*obj*/)
{ str << "[plane-ref]"; return (str); }

std::ostream& operator<< (std::ostream& str, const utm_zone_t & obj)
{
  str << "[UTM - Z: " << obj.zone() << " " << (obj.is_north() ? "N" : "S") << "]";
  return str;
}

// instantiate the usual suspects.
template class vidtk::homography_T < vidtk::timestamp,   vidtk::timestamp >;
template class vidtk::homography_T < vidtk::timestamp,   vidtk::plane_ref_t >;
template class vidtk::homography_T < vidtk::plane_ref_t, vidtk::timestamp >;
template class vidtk::homography_T < vidtk::timestamp,   vidtk::utm_zone_t >;
template class vidtk::homography_T < vidtk::utm_zone_t,  vidtk::timestamp >;
template class vidtk::homography_T < vidtk::plane_ref_t, vidtk::utm_zone_t >;
template class vidtk::homography_T < vidtk::utm_zone_t,  vidtk::plane_ref_t >;

} // end namespace
