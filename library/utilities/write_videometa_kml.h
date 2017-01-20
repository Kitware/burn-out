/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#ifndef _VIDTK_WRITE_VIDEOMETA_KML_H_
#define _VIDTK_WRITE_VIDEOMETA_KML_H_

#include <utilities/video_metadata.h>
#include <utilities/timestamp.h>

#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <vector>

#include <utilities/geo_lat_lon.h>

namespace vidtk
{

class write_videometa_kml
{
public:
  write_videometa_kml()
  {}

  ~write_videometa_kml();

  bool open(std::string const& fname);
  bool is_open() const;
  void close();
  ///write the conrner points in vm, if they exist
  bool write_corner_point( timestamp const& ts, video_metadata const& vm );
  ///write the corner points compute_transformation transform uses
  bool write_comp_transform_points(timestamp const& ts, unsigned int ni, unsigned int nj, video_metadata const& md );
protected:
  std::ofstream out_;
  bool write( double time, unsigned int frame, std::vector<geo_coord::geo_lat_lon> const& box );

}; //class write_videometa_kml

} //namespace vidtk

#endif //#ifndef _VIDTK_WRITE_VIDEOMETA_KML_H_
