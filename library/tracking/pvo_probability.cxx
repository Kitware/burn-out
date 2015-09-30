/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pvo_probability.h"
#include <vcl_cassert.h>
#include <vcl_iostream.h>

namespace vidtk
{

pvo_probability
::pvo_probability( )
  : probability_person_(0.33333),
    probability_vehicle_(0.33333),
    probability_other_(0.33333)
{
  this->normalize();
}

pvo_probability
::pvo_probability( double probability_person,
                   double probability_vehicle,
                   double probability_other )
  : probability_person_(probability_person),
    probability_vehicle_(probability_vehicle),
    probability_other_( probability_other )
{
  this->normalize();
}

//pvo_probability
//::pvo_probability( pvo_probability const & rhs )
//{
//  *this = rhs;
//}

double
pvo_probability
::get_probability_person() const
{
  return probability_person_;
}

void
pvo_probability
::set_probability_person( double p )
{
  probability_person_ = p;
}

double
pvo_probability
::get_probability_vehicle() const
{
  return probability_vehicle_;
}

void
pvo_probability
::set_probability_vehicle( double p )
{
  probability_vehicle_ = p;
}

double
pvo_probability
::get_probability_other() const
{
  return probability_other_;
}

void
pvo_probability
::set_probability_other( double p )
{
  probability_other_ = p;
}

void
pvo_probability
::adjust( double pper, double pveh, double poth)
{
  probability_person_ *= pper;
  probability_vehicle_ *= pveh;
  probability_other_ *= poth;
  this->normalize();
}

void
pvo_probability
::set_probabilities( double pperson, double pvehicle, double pother)
{
  probability_person_ = pperson;
  probability_vehicle_ = pvehicle;
  probability_other_ = pother;
  this->normalize();
}

void
pvo_probability
::normalize()
{
  const static double val = 1.0/1073741824.0; ///Very small number to prevent div by zero.
  if(this->probability_person_ < val)
  {
    this->probability_person_ = val;
  }
  if(this->probability_vehicle_ < val)
  {
    this->probability_vehicle_ = val;
  }
  if(this->probability_other_ < val)
  {
    this->probability_other_ = val;
  }
  double sum = probability_person_ + probability_vehicle_ + probability_other_;
  probability_person_ = probability_person_/sum;
  probability_vehicle_ = probability_vehicle_/sum;
  probability_other_ = probability_other_/sum;
}

//void
//pvo_probability
//::operator =(const pvo_probability & rhs)
//{
//  this->probability_person_ = rhs.get_probability_person(); 
//  this->probability_vehicle_ = rhs.get_probability_vehicle(); 
//  this->probability_other_ = rhs.get_probability_other(); 
//}

pvo_probability &
pvo_probability
::operator *=(const pvo_probability & b)
{
  assert(b.probability_person_ != 0);
  assert(b.probability_vehicle_ != 0);
  assert(b.probability_other_ != 0);
  this->probability_person_ *= b.probability_person_;
  this->probability_vehicle_ *= b.probability_vehicle_;
  this->probability_other_ *= b.probability_other_;
  this->normalize();
  return *this;
}

double
pvo_probability
::get_probability( pvo_probability::id i )
{
  switch(i)
  {
    case pvo_probability::person:
      return this->get_probability_person();
    case pvo_probability::vehicle:
      return this->get_probability_vehicle();
    case pvo_probability::other:
      return this->get_probability_other();
  };
  vcl_cerr << "i is not of a known type: " << i << vcl_endl;
  assert( !"i is not of a known type");
  return -1.0; //This should never happen
}

vnl_vector<double>
pvo_probability
::get_probabilities()
{
  vnl_vector<double> result(3);
  result[pvo_probability::person] = this->get_probability_person();
  result[pvo_probability::vehicle] = this->get_probability_vehicle();
  result[pvo_probability::other] = this->get_probability_other();
  return result;
}

}

vidtk::pvo_probability operator*(const vidtk::pvo_probability & l, const vidtk::pvo_probability & r)
{
  assert(r.get_probability_person() != 0);
  assert(r.get_probability_vehicle() != 0);
  assert(r.get_probability_other() != 0);
  assert(l.get_probability_person() != 0);
  assert(l.get_probability_vehicle() != 0);
  assert(l.get_probability_other() != 0);
  vidtk::pvo_probability result( l.get_probability_person() * r.get_probability_person(), //probability_person,
                                 l.get_probability_vehicle() * r.get_probability_vehicle(), //probability_vehicle,
                                 l.get_probability_other() * r.get_probability_other() ); //probability_other
  return result;
}

vcl_ostream& operator<< (vcl_ostream& os, const vidtk::pvo_probability &p)
{

  os << p.get_probability_person() << " " << p.get_probability_vehicle() << " " << p.get_probability_other();
  return os;
}
