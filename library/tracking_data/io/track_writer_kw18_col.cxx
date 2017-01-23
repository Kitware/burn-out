/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer_kw18_col.h"

#include <tracking_data/tracking_keys.h>
#include <logger/logger.h>
#include <utilities/geo_lat_lon.h>

namespace vidtk
{

VIDTK_LOGGER( "kw18_col_writer" );

track_writer_kw18_col
::track_writer_kw18_col( colfmt in )
  : format_( in ),
  fstr_( NULL ),
  suppress_header_( false ),
  x_offset_( 0 ),
  y_offset_( 0 ),
  write_lat_lon_for_world_( false ),
  write_utm_( false ),
  frequency_( 0.0 )
{
}


track_writer_kw18_col
::~track_writer_kw18_col()
{
  this->close();
  delete fstr_;
}


void
track_writer_kw18_col
::set_options( track_writer_options const& options )
{
  suppress_header_ = options.suppress_header_;
  x_offset_ = options.x_offset_;
  y_offset_ = options.y_offset_;
  write_lat_lon_for_world_ = options.write_lat_lon_for_world_,
  write_utm_ = options.write_utm_;
  frequency_ = options.frequency_;
}


/// This format should only be used for the tracks.
///
/// \li Column(s) 1: Track-id
/// \li Column(s) 2: Track-length (# of detections)
/// \li Column(s) 3: Frame-number (-1 if not available)
/// \li Column(s) 4-5: Tracking-plane-loc(x,y) (Could be same as World-loc)
/// \li Column(s) 6-7: Velocity(x,y)
/// \li Column(s) 8-9: Image-loc(x,y)
/// \li Column(s) 10-13: Img-bbox(TL_x,TL_y,BR_x,BR_y) (location of top-left & bottom-right vertices)
/// \li Column(s) 14: Area
/// \li Column(s) 15-17: World-loc(x,y,z) (longitude, latitude, 0 - when available)
/// \li Column(s) 18: Timesetamp(-1 if not available)
/// \li Column(s) 19: Track-confidence(-1_when_not_available)

bool
track_writer_kw18_col
::write( vidtk::track const& track )
{
  if ( ! is_good() )
  {
    return false;
  }
  // if you change this format, change the documentation above and change the header output in initialize();
  std::vector< track_state_sptr > const& hist = track.history();
  unsigned M = hist.size();
  for ( std::vector< track_state_sptr >::const_iterator iter = hist.begin(); iter != hist.end(); ++iter )
  {
    track_state_sptr tss = *iter;
    fstr_->precision( 20 ); //originally this was 3.  That resulted in time being truncated.  3 is evil.
    //The floating-point precision determines the maximum number of digits to
    //be written on insertion operations to express floating-point values.
    ( *fstr_ ) << track.id() << " ";
    ( *fstr_ ) << M << " ";
    if ( tss->time_.has_frame_number() )
    {
      ( *fstr_ ) << tss->time_.frame_number() << " ";
    }
    else
    {
      ( *fstr_ ) << "-1" << " ";
    }

    if ( write_utm_ )
    {
      geo_coord::geo_UTM geo;
      tss->get_smoothed_loc_utm( geo );
      vnl_vector_fixed< double, 2 > vel;
      bool r = tss->get_utm_vel( vel );
      if ( r && geo.is_valid() )
      {
        ( *fstr_ ) << geo.get_easting() << " ";
        ( *fstr_ ) << geo.get_northing() << " ";
        ( *fstr_ ) << vel[0] << " ";
        ( *fstr_ ) << vel[1] << " ";
      }
      else
      {
        LOG_ERROR( "TRACK MISSING UTM DATA" );
        return false;
      }
    }
    else
    {
      ( *fstr_ ) << tss->loc_( 0 ) << " ";
      ( *fstr_ ) << tss->loc_( 1 ) << " ";
      ( *fstr_ ) << tss->vel_( 0 ) << " ";
      ( *fstr_ ) << tss->vel_( 1 ) << " ";
    }
    std::vector< image_object_sptr > objs;
    if ( ! tss->data_.get( tracking_keys::img_objs, objs ) || objs.empty() || ( objs[0] == NULL ) )
    {
      ( *fstr_ ) << tss->loc_( 0 ) << " ";
      ( *fstr_ ) << tss->loc_( 1 ) << " ";
      ( *fstr_ ) << tss->amhi_bbox_.min_x() + x_offset_ << " ";
      ( *fstr_ ) << tss->amhi_bbox_.min_y() + y_offset_ << " ";
      ( *fstr_ ) << tss->amhi_bbox_.max_x() + x_offset_ << " ";
      ( *fstr_ ) << tss->amhi_bbox_.max_y() + y_offset_ << " ";

      ( *fstr_ ) << "0 0 0 0 ";
      LOG_WARN( "no MOD for track " << track.id() );
    }
    else
    {
      this->write_io( *objs[0] );
      //
      // Write out longitude, latitude in world coordinates if:
      //  1. lat/lon are available
      //  2. class level write_lat_lon_for_world_ flag is set.
      //
      if (  write_lat_lon_for_world_ )
      {
        // Invalid default values to be written out to the file to
        // to indicate missing values.
        vidtk::geo_coord::geo_lat_lon ll;
        tss->latitude_longitude( ll );
        ( *fstr_ ) << ll.get_longitude() << " " << ll.get_latitude() << " " << 0.0 << " ";
      }
      else
      {
        ( *fstr_ ) << objs[0]->get_world_loc() << " ";
      }
    }
    //timestamp is in seconds
    if ( tss->time_.has_time() )
    {
      ( *fstr_ ) << tss->time_.time_in_secs() << " ";
    }
    else if ( ( frequency_ != 0 ) && tss->time_.has_frame_number() )
    {
      ( *fstr_ ) << static_cast< double > ( tss->time_.frame_number() ) / frequency_ << " ";
    }
    else
    {
      ( *fstr_ ) << "-1" << " ";
    }
    double conf = -1.0;
    tss->get_track_confidence(conf);
    (*fstr_) << conf <<"\n";
  }
  fstr_->flush();
  return this->is_good();
} // track_writer_kw18_col::write


void
track_writer_kw18_col
::write_io( image_object const& io )
{
  const vgl_box_2d< unsigned >& bbox = io.get_bbox();

  ( *fstr_ ) << io.get_image_loc() << " ";
  ( *fstr_ ) << bbox.min_x() + x_offset_ << " ";
  ( *fstr_ ) << bbox.min_y() + y_offset_ << " ";
  ( *fstr_ ) << bbox.max_x() + x_offset_ << " ";
  ( *fstr_ ) << bbox.max_y() + y_offset_ << " ";
  ( *fstr_ ) << io.get_area() << " ";
}


/// Format_2: (columns_tracks_and_objects) tracks and detections in KW20 format
///
/// This format adds object detections in the same file as tracks.
///
/// \li Column(s) 1: (-1 for detection entries)
/// \li Column(s) 2: (-1 for detection entries)
/// \li Column(s) 3: Frame-number (-1 if not available)
/// \li Column(s) 4-5: (-1 for detection entries)
/// \li Column(s) 6-7: (-1 for detection entries)
/// \li Column(s) 8-9: Image-loc(x,y)
/// \li Column(s) 10-13: Img-bbox(TL_x,TL_y,BR_x,BR_y) (location of top-left & bottom-right vertices)
/// \li Column(s) 14: Area
/// \li Column(s) 15-17: World-loc(x,y,z)
/// \li Column(s) 18: Timesetamp(-1 if not available)
/// \li Column(s) 19: Object type id #(-1 if not available)
/// \li Column(s) 20: Activity type id #(-1 if not available)
bool
track_writer_kw18_col
::write( image_object const& io, timestamp const& ts )
{
  if ( ! is_good() )
  {
    return false;
  }

  if ( format_ == KW20 )
  {
    return true;
  }

  ( *fstr_ ) << "-1" << " "; //track-id
  ( *fstr_ ) << "-1" << " "; //track-length
  if ( ts.has_frame_number() )
  {
    ( *fstr_ ) << ts.frame_number() << " ";
  }
  else
  {
    ( *fstr_ ) << "-1" << " ";
  }
  ( *fstr_ ) << "-1" << " "; //tracker-plane-loc-x
  ( *fstr_ ) << "-1" << " "; //tracker-plane-loc-y
  ( *fstr_ ) << "-1" << " "; //tracker-plane-vel-x
  ( *fstr_ ) << "-1" << " "; //tracker-plane-vel-y
  write_io( io );
  ( *fstr_ ) << io.get_world_loc() << " ";
  if ( ts.has_time() )
  {
    ( *fstr_ ) << ts.time_in_secs() << " ";
  }
  else
  {
    ( *fstr_ ) << "-1" << " ";
  }
  ( *fstr_ ) << "-1";
  ( *fstr_ ) << "-1";
  ( *fstr_ ) << "\n";
  fstr_->flush();

  return is_good();
} // track_writer_kw18_col::write


bool
track_writer_kw18_col
::open( std::string const& fname )
{
  if ( ! fstr_ )
  {
    delete fstr_;
  }
  fstr_ = new std::ofstream( fname.c_str() );
  if ( ! this->is_good() )
  {
    return false;
  }
  if ( ! suppress_header_ )
  {
    std::string detec_post = " ";
    if ( format_ == COL )
    {
      detec_post = " [-1_for_detections] ";
    }
    ( *fstr_ ) << "#1:Track-id" << detec_post << "2:Track-length" << detec_post << "3:Frame-number";
    if ( write_utm_ )
    {
      ( *fstr_ )  << "4:Tracking-plane-loc-UTM(x)" << detec_post
                  << "5:Tracking-plane-loc-UTM(y)" << detec_post
                  << "6:velocity_UTM(x)" << detec_post
                  << "7:velocity_UTM(y)" << detec_post;
    }
    else
    {
      ( *fstr_ )  << "4:Tracking-plane-loc(x)" << detec_post
                  << "5:Tracking-plane-loc(y)" << detec_post
                  << "6:velocity(x)" << detec_post
                  << "7:velocity(y)" << detec_post;
    }

    ( *fstr_ )  << "8:Image-loc(x)"
                << " 9:Image-loc(y)"
                << " 10:Img-bbox(TL_x)"
                << " 11:Img-bbox(TL_y)"
                << " 12:Img-bbox(BR_x)"
                << " 13:Img-bbox(BR_y)"
                << " 14:Area";

    std::string x="x",y="y",z="z";
    if ( write_lat_lon_for_world_ )
    {
      x = "longitude";
      y = "latitude";
      z = "altitude";
    }
    ( *fstr_ ) << " 15:World-loc("<<x<<")"
               << " 16:World-loc("<<y<<")"
               << " 17:World-loc("<<z<<")"
               << " 18:timesetamp"
               << " 19:track-confidence\n";
  }

  if ( ! this->is_good() )
  {
    LOG_ERROR( "Could not open " << fname );
    return false;
  }
  return true;
} // track_writer_kw18_col::open


bool
track_writer_kw18_col
::is_open() const
{
  return this->is_good();
}


void
track_writer_kw18_col
::close()
{
  if ( fstr_ )
  {
    fstr_->close();
  }
}


bool
track_writer_kw18_col
::is_good() const
{
  return fstr_ && fstr_->good();
}

}//namespace vidtk
