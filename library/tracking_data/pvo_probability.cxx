/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pvo_probability.h"

#include <vbl/vbl_smart_ptr.txx>
#include <cassert>
#include <iostream>
#include <algorithm>

#include <logger/logger.h>

VIDTK_LOGGER("pvo_probability");

#define DEFAULT_PROBABILITY (1.0/3.0)

namespace vidtk
{

const double pvo_probability::min_probability_ ( 1.0 / 1073741824.0 );

pvo_probability
::pvo_probability( )
  : probability_person_(DEFAULT_PROBABILITY),
    probability_vehicle_(DEFAULT_PROBABILITY),
    probability_other_(DEFAULT_PROBABILITY),
    is_set_(false)
{
  this->normalize();
}


pvo_probability
::pvo_probability( double probability_person,
                   double probability_vehicle,
                   double probability_other )
  : probability_person_(probability_person),
    probability_vehicle_(probability_vehicle),
    probability_other_( probability_other ),
    is_set_(true)
{
  this->normalize();
}


double
pvo_probability
::get_minimum_value()
{
  return min_probability_;
}


void
pvo_probability
::scale_pvo( double pper, double pveh, double poth)
{
  assert( pper > 0 );
  assert( pveh > 0 );
  assert( poth > 0 );

  // Don't need to validate factor greater than zero.
  // The normalize() operation will fix the result.
  probability_person_  *= pper;
  probability_vehicle_ *= pveh;
  probability_other_   *= poth;

  this->normalize();
}


void
pvo_probability
  ::normalize()
{
  this->probability_person_  = std::max( this->probability_person_,  min_probability_ );
  this->probability_vehicle_ = std::max( this->probability_vehicle_, min_probability_ );
  this->probability_other_   = std::max( this->probability_other_,   min_probability_ );

  const double sum = this->probability_person_ + this->probability_vehicle_ + this->probability_other_;
  probability_person_ = this->probability_person_ / sum;
  probability_vehicle_ = this->probability_vehicle_ / sum;
  probability_other_ = this->probability_other_ / sum;

  this->is_set_ = true;
}


pvo_probability&
pvo_probability
::operator *= ( pvo_probability const& b )
{
  assert(b.probability_person_ > 0);
  assert(b.probability_vehicle_ > 0);
  assert(b.probability_other_ > 0);

  // Don't need to validate factor greater than zero.
  // The normalize() operation will fix the result.
  this->probability_person_  *= b.probability_person_;
  this->probability_vehicle_ *= b.probability_vehicle_;
  this->probability_other_   *= b.probability_other_;

  this->normalize();

  return *this;
}


double
pvo_probability
::get_probability( pvo_probability::id i ) const
{
  switch(i)
  {
    case pvo_probability::person:
      return this->probability_person_;

    case pvo_probability::vehicle:
      return this->probability_vehicle_;

    case pvo_probability::other:
      return this->probability_other_;

  default:
    LOG_ERROR( "PVO probability ID code is not supported: " << i );
  }

  return -1.0; //This should never happen
}


void
pvo_probability
::set_probability( pvo_probability::id i, double val )
{
  switch(i)
  {
    case pvo_probability::person:
      this->probability_person_ = val;

    case pvo_probability::vehicle:
      this->probability_vehicle_ = val;

    case pvo_probability::other:
      this->probability_other_ = val;

  default:
    LOG_ERROR( "PVO probability ID code is not supported: " << i );
  }
}


bool
pvo_probability
::is_set() const
{
  return is_set_;
}


vidtk::pvo_probability operator*(const vidtk::pvo_probability & l, const vidtk::pvo_probability & r)
{
  assert(r.get_probability_person() > 0);
  assert(r.get_probability_vehicle() > 0);
  assert(r.get_probability_other() > 0);

  assert(l.get_probability_person() > 0);
  assert(l.get_probability_vehicle() > 0);
  assert(l.get_probability_other() > 0);

  // Don't need to validate factor is greater than zero at run time.
  // The normalize() operation in the CTOR will fix the result.
  vidtk::pvo_probability result( l.get_probability_person() * r.get_probability_person(), //probability_person,
                                 l.get_probability_vehicle() * r.get_probability_vehicle(), //probability_vehicle,
                                 l.get_probability_other() * r.get_probability_other() ); //probability_other

  return result;
}


std::ostream& operator<< (std::ostream& os, const vidtk::pvo_probability &p)
{
  os << p.get_probability_person() << " " << p.get_probability_vehicle() << " " << p.get_probability_other();
  return os;
}

}  // namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::pvo_probability );

#undef DEFAULT_PROBABILITY
