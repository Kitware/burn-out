/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "write_videometa_kml.h"
#include <utilities/compute_transformations.h>
#include <iomanip>

namespace vidtk
{

write_videometa_kml
::~write_videometa_kml()
{
  close();
}

bool
write_videometa_kml
::open(std::string const& fname)
{
  close();
  out_.open(fname.c_str());
  if(!out_)
  {
    //no log, messaging should be handled by caller
    return false;
  }
  out_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
  out_ << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << std::endl;
  out_ << "  <Document>" << std::endl;
  out_ << "    <name>MetaBoxes</name>" << std::endl;
  out_ << "    <open>0</open>" << std::endl;
  out_ << "    <description>corner points</description>" << std::endl;

  return true;
}

bool
write_videometa_kml
::is_open() const
{
  return out_.is_open();
}

void
write_videometa_kml
::close()
{
  if(is_open())
  {
    out_ << "  </Document>" << std::endl;
    out_ << "</kml>" << std::endl;
    out_.close();
  }
  out_.clear();
}

bool
write_videometa_kml
::write_corner_point( timestamp const& ts, video_metadata const& md )
{
  std::vector<  geo_coord::geo_lat_lon > pts;
  if( md.has_valid_corners() )
  {
    pts.resize(4);
    pts[0] =  md.corner_ul();
    pts[1]=  md.corner_ur();
    pts[2]=  md.corner_lr();
    pts[3]=  md.corner_ll();
    return write(ts.time_in_secs(), ts.frame_number(), pts);
  }
  return false;
}

bool
write_videometa_kml
::write_comp_transform_points(timestamp const& ts, unsigned int ni, unsigned int nj, video_metadata const& md )
{
  std::vector< geo_coord::geo_lat_lon > pts;
  if(!extract_latlon_corner_points( md, ni, nj, pts ))
  {
    //no log, messaging should be handled by caller
    return false;
  }
  return write(ts.time_in_secs(), ts.frame_number(), pts);
}

bool
write_videometa_kml
::write( double time, unsigned int frame, std::vector<geo_coord::geo_lat_lon> const& box )
{
  out_ << "     <Placemark>" << std::endl;
  out_ << "        <TimeStamp>" << std::endl;
  out_ << "           <when> " << time << " </when>" << std::endl;
  out_ << "        </TimeStamp>" << std::endl;
  out_ << "        <name> " << frame << " </name>";
  out_ << "        <extrude>1</extrude>" << std::endl;
  out_ << "        <altitudeMode>relativeToGround</altitudeMode>" << std::endl;
  out_ << "        <LinearRing>" << std::endl;
  out_ << "            <coordinates>" << std::endl;

  for(unsigned int i = 0; i < box.size(); ++i)
  {
    out_ << "            " << std::setprecision(10) << box[i].get_longitude() << "," << box[i].get_latitude() << ",0" << std::endl;
  }

  if(box.size())
  {
    out_ << "            " << box[0].get_longitude() << "," << box[0].get_latitude() << ",0" << std::endl;
  }

  out_ << "            </coordinates>" << std::endl;
  out_ << "         </LinearRing>" << std::endl;
  out_ << "      </Placemark>" << std::endl;

  return out_.good();
}

}
