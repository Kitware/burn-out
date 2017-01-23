/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_GEO_COORDINATE_H_
#define _VIDTK_GEO_COORDINATE_H_

#include <utilities/geo_lat_lon.h>
#include <utilities/geo_MGRS.h>
#include <utilities/geo_UTM.h>
#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

namespace vidtk {
namespace  geo_coord {

class geo_coordinate;
typedef vbl_smart_ptr<geo_coordinate> geo_coordinate_sptr;


// ----------------------------------------------------------------------------
/*! \brief A container class for the various geo_coord types in vidtk.
 *
 * This class is constructed either with an instance of one of the individual
 * geo_coord types or using their appropriate bits of data, e.g. lat/lon points
 * for lat_lon or zone, is_north, easting and northing for UTM. Currently there
 * is no default constructor. The intent behind that choice is to make the
 * convert functions simpler and to keep the data in this class immutable in
 * terms of the actual point it represents. Allowing a default constructor,'
 * (thus an invalid point), assumes one will be able to update the underlying
 * point. I don't think that's desirable behaviour.
 *
 * All type conversions done in this class are lazy. A (possibly arbitrary) decision
 * was made as to the conversion path perference. UTM prefers to convert from lat_lon
 * when possible, lat_lon similary prefers to convert from UTM and MGRS prefers converting
 * from UTM when availble.
 *
 * @todo: It might be useful to run some tests to ensure that the chose path of conversion
 *        is the most correct and efficient. In other words, is it faster and/or more accurate
 *        to obtain the MGRS from UTM directly? Does it ever make sense to convert to lat_lon
 *        first and get MGRS from that? The same questions apply to other point combinations.
 */

class geo_coordinate
  : public vbl_ref_count
{
public:

  // Constructors from underlying coord types. Our members are constructed anew
  // from the data contained in the provided coordinate.
  geo_coordinate();
  geo_coordinate( geo_coordinate_sptr g );
  explicit geo_coordinate( geo_MGRS const& );
  explicit geo_coordinate( geo_UTM const& );
  explicit geo_coordinate( geo_lat_lon const& );

  // Constructors from the raw data.
  // @todo: decide what a good initial MGRS parameter is. By the looks
  //        of the data, it should probably be a string. Not require currently though.
  geo_coordinate(double lat, double lon);
  geo_coordinate(int zone, bool is_north, double easting, double northing);

  // Destructor is not virtual. Class not intended for sub-classing
  ~geo_coordinate();

  // Get back the underlying coordinate type.
  // These functions automatically call the appropriate convert function if required.
  geo_UTM const& get_utm();
  geo_lat_lon const& get_lat_lon();
  geo_MGRS const& get_mgrs();

  // Functions to return salient bits of data from underlying coordinate.
  // Note: the appropriate conversion function is called if required.
  double get_latitude();
  double get_longitude();
  std::string const& get_mgrs_coord();

  void update_utm( geo_UTM const& utm );

  bool is_valid();

  geo_coordinate operator=( const geo_coordinate& gc );

private:

  // Members
  geo_lat_lon m_geo_lat_lon;
  geo_MGRS m_geo_MGRS;
  geo_UTM m_geo_UTM;

  // Implementation of light weight conversion wrappers for
  // our geo coordinate classes underlying members.
  void convert_to_lat_lon( geo_MGRS const& coord );
  void convert_to_lat_lon( geo_UTM const& coord );

  void convert_to_UTM( geo_lat_lon const& coord );
  void convert_to_UTM( geo_MGRS const& coord );

  void convert_to_MGRS( geo_lat_lon const& coord );
  void convert_to_MGRS( geo_UTM const& coord );

}; // class geo_coordinate

// --------------------------------------------------------------------------
/*! \brief A small utility class that represents a geographic bounding region.
 *
 * @todo: This class current uses a handrolled expand method to grow the bounds of the region.
 *        It would be much better to use an existing geometric package to handle this process
 *        as well as to provide more complete functionality. Vidtk currently uses geographiclib
 *        as its geo package of choice, which doesn't provide a suitable extension for this purpose.
 *        The best direction would be to use GEOS or something similar, but that would be a large
 *        change and would require a great to deal of performance testing ( likely worth the effort ).
 */
class geo_bounds
{

public:

  geo_bounds() :
    upper_left(),
    upper_right(),
    lower_left(),
    lower_right()
  { }


  /// Add a new point to the region
  bool add( geo_coord::geo_coordinate pt )
  {
    geo_UTM u = pt.get_utm();
    if (!u.is_valid())
    {
      return false;
    }

    // If any points are invalid, initialize them.
    if ( !upper_left.is_valid() ) upper_left = pt;
    if ( !upper_right.is_valid() ) upper_right = pt;
    if ( !lower_left.is_valid() ) lower_left = pt;
    if ( !lower_right.is_valid() ) lower_right = pt;

    geo_UTM ul = upper_left.get_utm();
    geo_UTM ur = upper_right.get_utm();
    geo_UTM ll = lower_left.get_utm();
    geo_UTM lr = lower_right.get_utm();

    //compare new upper_left
    if (u.get_easting() >= ul.get_easting() && u.get_northing() >= ul.get_northing())
    {
      upper_left = pt;
    }
    //compare new upper_right
    if (u.get_easting() <= ur.get_easting() && u.get_northing() >= ur.get_northing())
    {
      upper_right = pt;
    }
    //compare new lower_left
    if ( u.get_easting() >= ll.get_easting() && u.get_northing() <= ll.get_northing() )
    {
      lower_left = pt;
    }
    //compare new lower_right
    if ( u.get_easting() <= lr.get_easting() && u.get_northing() <= lr.get_northing() )
    {
      lower_right = pt;
    }

    return true;
  }

  // --------------------------------------------------------------------------
  /*! \brief Expand the current geo_bounds by a specified distance in meters.
   *         This function works directly on the UTM values of the geo_bounds
   */
  void expand( double meters )
  {
    geo_UTM ul = upper_left.get_utm();
    geo_UTM ur = upper_right.get_utm();
    geo_UTM ll = lower_left.get_utm();
    geo_UTM lr = lower_right.get_utm();

    ul.set_easting( ul.get_easting() - meters );
    ul.set_northing( ul.get_easting() + meters );
    upper_left.update_utm(ul);

    ur.set_easting(ur.get_easting() + meters);
    ur.set_northing(ur.get_northing() + meters);
    upper_right.update_utm(ur);

    ll.set_easting(ll.get_easting() - meters);
    ll.set_northing(ll.get_northing() - meters);
    lower_left.update_utm(ll);

    lr.set_easting(lr.get_easting() + meters);
    lr.set_northing(lr.get_northing() - meters);
    lower_right.update_utm(lr);
  }

  geo_coord::geo_coordinate const& get_upper_left() {return upper_left;}
  geo_coord::geo_coordinate const& get_upper_right() {return upper_right;}
  geo_coord::geo_coordinate const& get_lower_left() {return lower_left;}
  geo_coord::geo_coordinate const& get_lower_right() {return lower_right;}

private:
  geo_coord::geo_coordinate upper_left;
  geo_coord::geo_coordinate upper_right;
  geo_coord::geo_coordinate lower_left;
  geo_coord::geo_coordinate lower_right;

};

} // end namespace
} // end namespace

#endif // _VIDTK_GEO_COORDINATE_H_
