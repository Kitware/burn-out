/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_GEO_UTM_H_
#define _VIDTK_GEO_UTM_H_

#include <ostream>

/**
 * \page UTM Convert between geographic coordinates and UTM/UPS
 *
 * UTM and UPS are defined
 * - J. W. Hager, J. F. Behensky, and B. W. Drew,
 *   <a href="http://earth-info.nga.mil/GandG/publications/tm8358.2/TM8358_2.pdf">
 *   The Universal Grids: Universal Transverse Mercator (UTM) and Universal
 *   Polar Stereographic (UPS)</a>, Defense Mapping Agency, Technical Manual
 *   TM8358.2 (1989).
 * .
 * Section 2-3 defines UTM and section 3-2.4 defines UPS.  This document also
 * includes approximate algorithms for the computation of the underlying
 * transverse Mercator and polar stereographic projections.  Here we
 * substitute much more accurate algorithms given by
 * GeographicLib:TransverseMercator and GeographicLib:PolarStereographic.
 *
 * In this implementation, the conversions are closed, i.e., output from
 * Forward is legal input for Reverse and vice versa.  The error is about 5nm
 * in each direction.  However, the conversion from legal UTM/UPS coordinates
 * to geographic coordinates and back might throw an error if the initial
 * point is within 5nm of the edge of the allowed range for the UTM/UPS
 * coordinates.
 *
 * The simplest way to guarantee the closed property is to define allowed
 * ranges for the eastings and northings for UTM and UPS coordinates.  The
 * UTM boundaries are the same for all zones.  (The only place the
 * exceptional nature of the zone boundaries is evident is when converting to
 * UTM/UPS coordinates requesting the standard zone.)  The MGRS lettering
 * scheme imposes natural limits on UTM/UPS coordinates which may be
 * converted into MGRS coordinates.  For the conversion to/from geographic
 * coordinates these ranges have been extended by 100km in order to provide a
 * generous overlap between UTM and UPS and between UTM zones.
 *
 * The <a href="http://www.nga.mil">NGA</a> software package
 * <a href="http://earth-info.nga.mil/GandG/geotrans/index.html">geotrans</a>
 * also provides conversions to and from UTM and UPS.  Version 2.4.2 (and
 * earlier) suffers from some drawbacks:
 * - Inconsistent rules are used to determine the whether a particular UTM or
 *   UPS coordinate is legal.  A more systematic approach is taken here.
 * - The underlying projections are not very accurately implemented.
 *
 *********************************************************************/

namespace vidtk {
namespace  geo_coord {

// ----------------------------------------------------------------
/** Geographic point in UTM coordinates.
 *
 *
 */
class geo_UTM
{
public:
  geo_UTM();
  geo_UTM( int zone, bool is_north, double easting, double northing );

  /// default constructed coordinate
  bool is_empty() const;
  bool is_valid() const;

  geo_UTM& set_zone( int z );
  geo_UTM& set_is_north( bool v );
  geo_UTM& set_easting( double v );
  geo_UTM& set_northing( double v );

  int get_zone() const;
  bool is_north() const;
  double get_easting() const;
  double get_northing() const;

  bool operator==( const geo_UTM& rhs ) const;
  bool operator!=( const geo_UTM& rhs ) const;
  geo_UTM operator=( const geo_UTM& u );

private:
  int zone_;
  bool is_north_;
  double easting_;
  double northing_;

};   // end class geo_UTM

std::ostream & operator<< (std::ostream & str, const ::vidtk::geo_coord::geo_UTM & obj);

} // end namespace
} // end namespace

#endif /* _VIDTK_GEO_UTM_H_ */
