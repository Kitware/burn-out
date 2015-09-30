/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "utm_coordinate.h"

#include <vnl/vnl_math.h>

#include <vcl_sstream.h>

#include <vul/vul_string.h>
#include <vul/vul_sprintf.h>

namespace vidtk
{

void
utm_coordinate
::convert_to_lat_long( double & lat, double & lon )
{
  // http://www.uwgb.edu/dutchs/UsefulData/UTMFormulas.htm
  // and http://www.uwgb.edu/dutchs/UsefulData/UTMConversions1.xls
  double a = 6378137.0;
  double b = 6356752.3142;
  //double f = 0.00335281066474748;
  //double oof = 298.257223563;
  double e = vcl_sqrt(1.0 - vnl_math::sqr(b/a));
  double e1sq = e*e/(1-e*e);
  double k0 = 0.9996;
  double arc = northing_/k0;
  double ecsq  = e*e;
  double ecfth = ecsq*ecsq;
  double ecsth = ecfth*ecsq;
  double mu = arc/(a*(1.-ecsq/4.-3.*ecfth/64.-5.*ecsth/256.));
  double e1 = (1.-vcl_sqrt(1-ecsq))/(1.+vcl_sqrt(1.-ecsq));
  double C1 = 3.*e1/2.-27.*e1*e1*e1/32.;
  double C2 = 21.*e1*e1/16.-55.*e1*e1*e1*e1/32.;
  double C3 = 151.*e1*e1*e1/96.;
  double C4 = 1097.*e1*e1*e1*e1/512.;
  double flat = mu+C1*vcl_sin(2.*mu)+C2*vcl_sin(4.*mu)+C3*vcl_sin(6.*mu)+C4*vcl_sin(8.*mu);
  C1 = e1sq*vnl_math::sqr(vcl_cos(flat));
  double T1 = vnl_math::sqr(vcl_tan(flat));
  double N1 = a/vcl_sqrt(1.-vnl_math::sqr(e*vcl_sin(flat)));
  double R1 = a*(1.-ecsq)/vcl_sqrt(vcl_pow(1.-vnl_math::sqr(e*vcl_sin(flat)), 3));
  double D  = (500000.-easting_)/(N1*k0);
  double Q1 = N1*vcl_tan(flat)/R1;
  double Q2 = vnl_math::sqr(D)*0.5;
  double Q3 = (5.0 + 3.*T1+10.*C1-4.*C1*C1 - 9 *e1sq)*D*D*D*D/24.;
  double Q4 = (61.+90.*T1+298.*C1+45.*T1*T1-252.*e1sq-3.*C1*C1)*D*D*D*D*D*D/720.;
  lat= 180 * (flat - Q1 * (Q2 + Q3 + Q4))/vnl_math::pi;
  double Q5 = D;
  double Q6 = (1. + 2. * T1 + C1) * D * D * D / 6.0;
  double Q7 = (5. - 2. * C1 + 28.*T1 - 3. * C1 * C1 + 8. * e1sq + 24.*T1*T1) * D*D*D*D*D/120.;
  vcl_string tmp = zone_.substr(0,zone_.size()-1);
  double long0 = vul_string_atof(tmp);
  if(long0 > 0)
  {
    long0 = 6.*long0-183.;
  }
  else
  {
    long0 = 3.;
  }
  double h20 = (Q5 - Q6 + Q7)/vcl_cos(flat);
  double e22 = h20*180/vnl_math::pi;
  lon = (long0 - e22);
//  if(lon < 0)
//  {
//    lon = -lon;
//  }
}

std::string
utm_coordinate
::to_string() const
{
  vcl_string b = vul_sprintf( "Northing: %d Easting: %d Zone: %s",  
                              vnl_math::rnd_halfintup(northing_),
                              vnl_math::rnd_halfintup(easting_),
                              zone_.c_str());
  return b;
}

}; //namespace vidtk
