/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shape_file_writer.h"

#include <tracking_data/image_object_util.h>
#include <tracking_data/track_state_util.h>

#include <utilities/video_metadata.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <shapefil.h>

#include <vul/vul_sprintf.h>

using namespace boost::posix_time;

namespace vidtk
{

struct shape_file_writer::internal
{
  SHPHandle file_handle_;
  DBFHandle dbf_handle_;
  int StartStamp, StartTimeMS, StartString,
      EndStamp, EndTimeMS, EndString, NumPoints;
  //APIX WAY:
  int latField, lonField, timeField, msField, timeStrField, MGRSField, frameNumField,
      interplatField;

};

// ------------------------------------------------------------------
shape_file_writer
::shape_file_writer( bool apix_way )
 : internal_(NULL), shape_type_(SHPT_ARC), is_open_(false), is_utm_(false), count_(0), apix_way_(apix_way)
{
  if(apix_way_)
  {
    set_shape_type(SHPT_POINT);
  }
  internal_ = new internal;
}


// ------------------------------------------------------------------
shape_file_writer
::~shape_file_writer()
{
  this->close();
  delete internal_;
}


// ------------------------------------------------------------------
bool
shape_file_writer
::write( vidtk::track_sptr track )
{
  return write(*track);
}

// ------------------------------------------------------------------
bool
shape_file_writer
::write( vidtk::track const & track )
{
  if(apix_way_)
  {
    return write_apix( track );
  }
  else
  {
    return write_old(track);
  }
  return false;
}


void
shape_file_writer
::reformat_time( std::string const& time_format_string,
                 double t_in_s,
                 long& utc_time,
                 int& utc_time_ms,
                 std::string& time_string )
{
  long hours = static_cast<long>( std::floor( t_in_s / 120.0 ) );
  t_in_s -= hours * 120.0;
  long minutes = static_cast<long>( std::floor( t_in_s / 60.0 ) );
  t_in_s -= minutes * 60.0;
  long seconds = static_cast<long>(t_in_s);
  t_in_s -= seconds;
  long ms = static_cast<long>(std::floor( t_in_s * 1000.0 ));
  time_duration duration( hours, minutes, seconds, 0);
  duration += milliseconds( ms );

  ptime frame_time( boost::gregorian::date(1970, 1, 1), duration );
  int time_of_day_ms = static_cast<int>( frame_time.time_of_day().total_milliseconds() - (frame_time.time_of_day().total_seconds() * 1000));

  time_string = vul_sprintf( time_format_string.c_str(),
                             static_cast<short>(frame_time.date().year()),
                             static_cast<short>(frame_time.date().month()),
                             static_cast<short>(frame_time.date().day()),
                             frame_time.time_of_day().hours(),
                             frame_time.time_of_day().minutes(),
                             frame_time.time_of_day().seconds(),
                             time_of_day_ms );

  utc_time = duration.total_seconds();
  utc_time_ms = ms;
}


// ------------------------------------------------------------------
bool
shape_file_writer
::write_apix( vidtk::track const & track )
{
  if(!is_open_)
  {
    return false;
  }
  std::vector< track_state_sptr > const & hist = track.history();
  for (unsigned i=0; i<hist.size(); ++i)
  {
    geo_coord::geo_coordinate_sptr coord = get_lat_lon(hist[i]);
    double lat = coord->get_latitude();
    double lon = coord->get_longitude();

    //DBF Stuff
    long utc_time_secs;
    int utc_time_ms;
    std::string time_string;
    std::string time_format_string = "%d-%02d-%02d T %02d:%02d:%02d.%03dZ";
    reformat_time( time_format_string,
                   hist[i]->time_.time_in_secs(),
                   utc_time_secs,
                   utc_time_ms,
                   time_string );


    DBFWriteDoubleAttribute( internal_->dbf_handle_, i, internal_->latField, lat );
    DBFWriteDoubleAttribute( internal_->dbf_handle_, i, internal_->lonField, lon );
    DBFWriteIntegerAttribute( internal_->dbf_handle_, i, internal_->timeField, utc_time_secs );
    DBFWriteIntegerAttribute( internal_->dbf_handle_, i, internal_->msField,  utc_time_ms );
    DBFWriteStringAttribute( internal_->dbf_handle_, i, internal_->timeStrField, time_string.c_str() );
    DBFWriteStringAttribute( internal_->dbf_handle_, i, internal_->MGRSField, coord->get_mgrs_coord().c_str() );
    DBFWriteIntegerAttribute( internal_->dbf_handle_, i, internal_->frameNumField, hist[i]->time_.frame_number() );
    DBFWriteIntegerAttribute( internal_->dbf_handle_, i, internal_->interplatField, 0 );
    //SHP
    SHPObject* obj = SHPCreateSimpleObject( shape_type_, 1, &lon, &lat, NULL );
    SHPWriteObject( internal_->file_handle_, /* new shape = */ -1, obj );
    SHPDestroyObject( obj );
  }
  return true;
}


// ------------------------------------------------------------------
bool
shape_file_writer
::write_old( vidtk::track const & track )
{
  if(!is_open_)
  {
    return false;
  }
  double * xs, * ys;
  track_state_sptr const& end    = track.last_state();
  track_state_sptr const& begin  = track.first_state();
  std::vector< track_state_sptr > const & hist = track.history();
  int number_of_frame = end->time_.frame_number() - begin->time_.frame_number() + 1;
  xs = new double[number_of_frame];
  ys = new double[number_of_frame];

  long begin_utc_time, end_utc_time;
  int begin_utc_time_ms, end_utc_time_ms;
  std::string begin_time_string, end_time_string;

  reformat_time( "%d-%02d-%02d T %02d:%02d:%02d.%03dZ", begin->time_.time_in_secs(),
                 begin_utc_time,
                 begin_utc_time_ms,
                 begin_time_string );
  reformat_time( "%d-%02d-%02d T %02d:%02d:%02d.%03dZ", end->time_.time_in_secs(),
                 end_utc_time,
                 end_utc_time_ms,
                 end_time_string );


  DBFWriteDoubleAttribute(  internal_->dbf_handle_, count_, internal_->StartStamp, static_cast<double>( begin_utc_time ));
  DBFWriteIntegerAttribute( internal_->dbf_handle_, count_, internal_->StartTimeMS, begin_utc_time_ms );
  DBFWriteStringAttribute(  internal_->dbf_handle_, count_, internal_->StartString, begin_time_string.c_str() );
  DBFWriteDoubleAttribute(  internal_->dbf_handle_, count_, internal_->EndStamp, static_cast<double>( end_utc_time ) );
  DBFWriteIntegerAttribute( internal_->dbf_handle_, count_, internal_->EndTimeMS, end_utc_time_ms );
  DBFWriteStringAttribute(  internal_->dbf_handle_, count_, internal_->EndString, end_time_string.c_str() );
  DBFWriteIntegerAttribute( internal_->dbf_handle_, count_, internal_->NumPoints, hist.size() );

  int prev_frame = begin->time_.frame_number()-1;
  double prev_locx = 0;
  double prev_locy = 0;

  int shp_at = 0;
  for( unsigned int at = 0; at < hist.size(); at++ )
  {
    int current_frame = hist[at]->time_.frame_number();
    geo_coord::geo_coordinate_sptr coord = get_lat_lon(hist[at]);
    double cloc_x = coord->get_latitude();
    double cloc_y = coord->get_longitude();

    double diff = current_frame - prev_frame;
    double diff_c = 0;
    while(current_frame - prev_frame > 1)
    {
      diff_c++;
      prev_frame++;
      double d = diff_c/diff;
      xs[shp_at] = (cloc_x*(1-d) + prev_locx*(d));
      ys[shp_at] = (cloc_y*(1-d) + prev_locy*(d));
      shp_at++;
    }
    xs[shp_at] = (cloc_x);
    ys[shp_at] = (cloc_y);
    shp_at++;
    prev_locx = cloc_x;
    prev_locy = cloc_y;
    prev_frame = current_frame;
  }

  SHPObject * spobj = SHPCreateSimpleObject( shape_type_, hist.size(), ys, xs, NULL );
  SHPWriteObject( internal_->file_handle_, -1, spobj );
  SHPDestroyObject( spobj );

  return true;
}


// ------------------------------------------------------------------
geo_coord::geo_coordinate_sptr
shape_file_writer
::get_lat_lon( vidtk::track_state_sptr ts )
{
  return vidtk::get_lat_lon( ts, is_utm_, zone_, is_north_ );
}


// ------------------------------------------------------------------
void
shape_file_writer
::open( std::string & fname )
{
  std::string f = fname;

  internal_->file_handle_ = SHPCreate( f.c_str(), shape_type_ );
  internal_->dbf_handle_  = DBFCreate( f.c_str() );

  if(apix_way_)
  {
    internal_->latField       = DBFAddField( internal_->dbf_handle_, "Latitude",   FTDouble,  12, 7 );
    internal_->lonField       = DBFAddField( internal_->dbf_handle_, "Longitude",  FTDouble,  12, 7 );
    internal_->timeField      = DBFAddField( internal_->dbf_handle_, "DataUTCTim", FTInteger, 10, 0 );
    internal_->msField        = DBFAddField( internal_->dbf_handle_, "DataTimeMS", FTInteger,  4, 0 );
    internal_->timeStrField   = DBFAddField( internal_->dbf_handle_, "TimeString", FTString,  30, 0 );
    internal_->MGRSField      = DBFAddField( internal_->dbf_handle_, "MGRS",       FTString,  24, 0 );
    internal_->frameNumField  = DBFAddField( internal_->dbf_handle_, "FrameNum",   FTInteger,  7, 0 );
    internal_->interplatField = DBFAddField( internal_->dbf_handle_, "Intrplat",   FTInteger,  2, 0 );
  }
  else
  {
    internal_->StartStamp  = DBFAddField( internal_->dbf_handle_, "StartStamp",  FTDouble,  10, 0 );
    internal_->StartTimeMS = DBFAddField( internal_->dbf_handle_, "StartTimeMS", FTInteger, 4, 0 );
    internal_->StartString = DBFAddField( internal_->dbf_handle_, "StartString", FTString,  30, 0 );
    internal_->EndStamp    = DBFAddField( internal_->dbf_handle_, "EndStamp",    FTDouble,  10, 0 );
    internal_->EndTimeMS   = DBFAddField( internal_->dbf_handle_, "EndTimeMS",   FTInteger,  4, 0 );
    internal_->EndString   = DBFAddField( internal_->dbf_handle_, "EndString",   FTString,  30, 0 );
    internal_->NumPoints   = DBFAddField( internal_->dbf_handle_, "NumPoints",   FTInteger,  6, 0 );
  }

  is_open_ = true;
  count_ = 0;
}


// ------------------------------------------------------------------
void
shape_file_writer
::close()
{
  if(is_open_)
  {
    is_open_ = false;
    SHPClose(internal_->file_handle_);
    DBFClose(internal_->dbf_handle_);
  }
}


// ------------------------------------------------------------------
void
shape_file_writer
::set_shape_type( int shape_type )
{
  shape_type_ = shape_type;
}


// ------------------------------------------------------------------
void
shape_file_writer
::set_is_utm(bool v)
{
  is_utm_ = v;
}


// ------------------------------------------------------------------
void
shape_file_writer
::set_utm_zone(int zone, bool is_north)
{
  zone_ = zone;
  is_north_ = is_north;
}

} // end name space


vidtk::shape_file_writer &
operator<<( vidtk::shape_file_writer & writer, vidtk::track_sptr track )
{
  writer.write(track);
  return writer;
}
