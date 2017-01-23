/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef __AOI_H__
#define __AOI_H__

/**
\file
\brief
Method and field definitions for an aoi.
*/


#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>
#include <vnl/vnl_vector_fixed.h>
#include <vul/vul_reg_exp.h> // crop string parsing
#include <utilities/timestamp.h>

#include <geographic/geo_coords.h>

#include <string>

namespace vidtk
{

class aoi;
typedef vbl_smart_ptr< aoi > aoi_sptr;

// ----------------------------------------------------------------
/**
 * @brief Area of interest.
 *
 */
class aoi :
  public vbl_ref_count
{
public:
  aoi();
  virtual ~aoi();

  vxl_int_32 get_aoi_id() const;
  vxl_int_32 get_mission_id() const;
  std::string get_aoi_name() const;
  std::string get_aoi_crop_string() const;
  vnl_vector_fixed< int, 2 > const& get_aoi_crop_wh() const;
  vnl_vector_fixed< int, 2 > const& get_aoi_crop_xy() const;
  double get_start_time() const;
  double get_end_time() const;
  timestamp const& get_start_timestamp() const;
  timestamp const& get_end_timestamp() const;
  unsigned get_start_frame() const;
  unsigned get_end_frame() const;
  std::string get_description() const;

  void set_aoi_id( vxl_int_32 );
  void set_mission_id( vxl_int_32 );
  void set_aoi_name( std::string );
  ///Crop string in the  format XxY+W+H
  bool set_aoi_crop_string( std::string );
  void set_aoi_crop_wh( vnl_vector_fixed< int, 2 > const& );
  void set_aoi_crop_xy( vnl_vector_fixed< int, 2 > const& );
  void set_start_time( timestamp& );
  void set_end_time( timestamp& );
  void set_description( std::string );

  enum aoi_point { UPPER_LEFT = 0, UPPER_RIGHT = 1, LOWER_RIGHT = 2, LOWER_LEFT = 3 };
  void set_geo_point( aoi_point, geographic::geo_coords );
  void set_geo_point_lat_lon( aoi_point, double lat, double lon );
  double get_latitude( aoi_point ) const;
  double get_longitude( aoi_point ) const;

  bool has_geo_points() const;
  enum intersection_mode { PIXEL, UTM, LAT_LONG };
  ///Tests to see if the point is inside the aoi.  PIXEL and LAT_LONG ignores the zone and is_north.  If they are NULL
  ///for UTM, the point is assumed to be in the same zone, but a warning message is issued for the first instance.
  bool point_inside( intersection_mode, double x, double y, int* zone = NULL, bool* is_north = NULL ) const;


private:
  ///Parse a crop string from the database into useable components
  bool get_crop_region_from_string( std::string&  crop_string,
                                    int&          crop_i0,
                                    int&          crop_j0,
                                    unsigned int& crop_ni,
                                    unsigned int& crop_nj );

  geographic::geo_coords world_corner_points_[4];

  vxl_int_32 aoi_id_;
  vxl_int_32 mission_id_;
  std::string aoi_name_;
  std::string aoi_crop_string_;
  vnl_vector_fixed< int, 2 > aoi_crop_wh_;
  vnl_vector_fixed< int, 2 > aoi_crop_xy_;
  double start_time_;
  double end_time_;
  unsigned start_frame_;
  unsigned end_frame_;
  timestamp start_timestamp_;
  timestamp end_timestamp_;
  std::string description_;
};

} // namespace vidtk

#endif //__AOI_H__
