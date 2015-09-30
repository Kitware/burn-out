/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shape_file_writer.h"

#include <tracking/tracking_keys.h>

#include <shapefil.h>

#include <utilities/utm_coordinate.h>
#include <utilities/convert_latitude_longitude_utm.h>

#include <boost/date_time/posix_time/posix_time.hpp>

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
};

shape_file_writer
::shape_file_writer()
  :shape_type_(SHPT_ARC), is_open_(false), is_utm_(false), count_(0)
{
  internal_ = new internal;
}


shape_file_writer
::~shape_file_writer()
{
  this->close();
}


bool
shape_file_writer
::write( vidtk::track_sptr track )
{
  return write(*track);
}


bool
shape_file_writer
::write( vidtk::track const & track )
{
  if(!is_open_)
  {
    return false;
  }
  double * xs, * ys;
  track_state_sptr const& end    = track.last_state();
  track_state_sptr const& begin  = track.first_state();
  vcl_vector< track_state_sptr > const & hist = track.history();
  int number_of_frame = end->time_.frame_number() - begin->time_.frame_number() + 1;
  xs = new double[number_of_frame];
  ys = new double[number_of_frame];
//  //temp hack to test the dbp file 1190921184

//  time_begin.set_time(1190921184 + ( time_begin.frame_number() - 2759 + 1)*0.5);
//  time_end.set_time(1190921184 + ( time_end.frame_number() - 2759 + 1)*0.5);

  // This is ugly, but my naive attempts to construct a time duration in
  // boost from a fractional number of seconds since the epoch didn't work
  // out since the duration constructors take longs, which overflow if
  // you express ~37 years as microseconds, so it seems you have to
  // do the clipping to unit boundaries yourself?

  long begin_hours = static_cast<long>(vcl_floor( begin->time_.time() / ( 1e6 * 60 * 60 )));
  long begin_minutes =  static_cast<long>(vcl_floor( (begin->time_.time() - (begin_hours * 60 * 60 * 1e6)) / (1e6 * 60 ) ));
  long begin_seconds =  static_cast<long>(vcl_floor( (begin->time_.time() - (((begin_hours * 60) + begin_minutes) * 60 * 1e6)) / (1e6) ));
  long begin_ms =  static_cast<long>(vcl_floor( (begin->time_.time() - (((((begin_hours*60)+begin_minutes)*60)+begin_seconds) * 1e6 ))) / 1e3);
  time_duration begin_duration( begin_hours, begin_minutes, begin_seconds, 0);
  begin_duration += milliseconds( begin_ms );

  long end_hours =  static_cast<long>(vcl_floor( end->time_.time() / ( 1e6 * 60 * 60 )));
  long end_minutes =  static_cast<long>(vcl_floor( (end->time_.time() - (end_hours * 60 * 60 * 1e6)) / (1e6 * 60 ) ));
  long end_seconds =  static_cast<long>(vcl_floor( (end->time_.time() - (((end_hours * 60) + end_minutes) * 60 * 1e6)) / (1e6) ));
  long end_ms =  static_cast<long>(vcl_floor( (end->time_.time() - (((((end_hours*60)+end_minutes)*60)+end_seconds) * 1e6 ))) / 1e3);
  time_duration end_duration( end_hours, end_minutes, end_seconds, 0);
  end_duration += milliseconds( end_ms );

  ptime begin_time( boost::gregorian::date(1970, 1, 1), begin_duration );
  ptime end_time( boost::gregorian::date(1970, 1, 1), end_duration );

  int startMS = static_cast<int>( (begin_time.time_of_day().total_milliseconds() - begin_time.time_of_day().total_seconds()) * 1000);
  int endMS = static_cast<int>( (end_time.time_of_day().total_milliseconds() - end_time.time_of_day().total_seconds()) * 1000);

  vcl_string b = vul_sprintf("%d-%02d-%02d T %02d:%02d:%02d.%03dZ",
                             static_cast<short>(begin_time.date().year()),
                             static_cast<short>(begin_time.date().month()),
                             static_cast<short>(begin_time.date().day()),
                             begin_time.time_of_day().hours(),
                             begin_time.time_of_day().minutes(),
                             begin_time.time_of_day().seconds(),
                             startMS
    );

  vcl_string e = vul_sprintf("%d-%02d-%02d T %02d:%02d:%02d.%03dZ",
                             static_cast<short>(end_time.date().year()),
                             static_cast<short>(end_time.date().month()),
                             static_cast<short>(end_time.date().day()),
                             end_time.time_of_day().hours(),
                             end_time.time_of_day().minutes(),
                             end_time.time_of_day().seconds(),
                             endMS
    );

  DBFWriteDoubleAttribute(  internal_->dbf_handle_, count_, internal_->StartStamp, static_cast<double>( begin_duration.total_seconds() ));
  DBFWriteIntegerAttribute( internal_->dbf_handle_, count_, internal_->StartTimeMS, startMS );
  DBFWriteStringAttribute(  internal_->dbf_handle_, count_, internal_->StartString, b.c_str() );
  DBFWriteDoubleAttribute(  internal_->dbf_handle_, count_, internal_->EndStamp, static_cast<double>( end_duration.total_seconds()) );
  DBFWriteIntegerAttribute( internal_->dbf_handle_, count_, internal_->EndTimeMS, endMS );

  DBFWriteStringAttribute(  internal_->dbf_handle_, count_, internal_->EndString, e.c_str() );
  DBFWriteIntegerAttribute( internal_->dbf_handle_, count_, internal_->NumPoints, hist.size() );

  int prev_frame = begin->time_.frame_number()-1;
  double prev_locx = 0;
  double prev_locy = 0;

  int shp_at = 0;
  for( unsigned int at = 0; at < hist.size(); at++ )
  {
    vcl_vector<vidtk::image_object_sptr> objs;
    int current_frame = hist[at]->time_.frame_number();
    double cloc_x, cloc_y;


    if ( hist[at]->data_.get( vidtk::tracking_keys::img_objs, objs ) )
    {
      if(objs.size())
      {
        if(is_utm_)
        {
          //utm_coordinate utm(objs[0]->world_loc_[1], objs[0]->world_loc_[0], zone_ );
          double x = vcl_floor(objs[0]->world_loc_[1]+0.5);
          double y = vcl_floor(objs[0]->world_loc_[0]+0.5);
          utm_coordinate utm( x,y, zone_ );
          utm.convert_to_lat_long(cloc_x, cloc_y);
          /*
          utm_coordinate t = convert_latitude_longitude_utm::convert_latitude_longitude_to_utm(cloc_x, cloc_y);
          utm_coordinate t2 = convert_latitude_longitude_utm::convert_latitude_longitude_to_utm(cloc_x + 0.00005, cloc_y);

          cloc_x += 0.00005; //TODO: This is a hack.  It seem the latitude is off by
                             //a bit in APIX.  What is annoying is the results are
                             //the same as the excel document referened in the class.
          */
        }
        else
        {
          cloc_x = objs[0]->world_loc_[0];
          cloc_y = objs[0]->world_loc_[1];
        }
      }
    }
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


void
shape_file_writer
::open( vcl_string & fname )
{
  vcl_string f = fname;

  internal_->file_handle_ = SHPCreate( f.c_str(), shape_type_ );
  internal_->dbf_handle_  = DBFCreate( f.c_str() );

  internal_->StartStamp = DBFAddField( internal_->dbf_handle_, "StartStamp", FTDouble,  10, 0 );
  internal_->StartTimeMS = DBFAddField( internal_->dbf_handle_, "StartTimeMS", FTInteger, 4, 0 );
  internal_->StartString = DBFAddField( internal_->dbf_handle_, "StartString", FTString,  30, 0 );
  internal_->EndStamp   = DBFAddField( internal_->dbf_handle_, "EndStamp",   FTDouble,  10, 0 );
  internal_->EndTimeMS  = DBFAddField( internal_->dbf_handle_, "EndTimeMS",  FTInteger,  4, 0 );
  internal_->EndString  = DBFAddField( internal_->dbf_handle_, "EndString",  FTString,  30, 0 );
  internal_->NumPoints  = DBFAddField( internal_->dbf_handle_, "NumPoints",  FTInteger,  6, 0 );

  is_open_ = true;
  count_ = 0;
}


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


void
shape_file_writer
::set_shape_type( int shape_type )
{
  shape_type_ = shape_type;
}


void
shape_file_writer
::set_is_utm(bool v)
{
  is_utm_ = v;
}


void
shape_file_writer
::set_utm_zone(vcl_string const & zone)
{
  zone_ = zone;
}

};


vidtk::shape_file_writer &
operator<<( vidtk::shape_file_writer & writer, vidtk::track_sptr track )
{
  writer.write(track);
  return writer;
}
