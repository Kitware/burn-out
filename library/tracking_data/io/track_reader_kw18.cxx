/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "track_reader_kw18.h"

#include <fstream>
#include <cstdio>
#include <tracking_data/tracking_keys.h>
#include <vil/vil_load.h>
#include <vil/vil_new.h>
#include <vil/vil_image_resource.h>
#include <vil/vil_crop.h>
#include <vil/vil_fill.h>
#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>

#include <utilities/shell_comments_filter.h>
#include <utilities/blank_line_filter.h>
#include <logger/logger.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>


namespace vidtk {
namespace ns_track_reader {


namespace {

VIDTK_LOGGER( "track_reader_kw18" );

  typedef std::map< int, int > id_map_t;

  // field numbers for KW18 file format
  enum{
    // 0: Object ID
    // 1: Track length (always 1 for detections)
    COL_FRAME = 2,
    COL_LOC_X,    // 3
    COL_LOC_Y,    // 4
    COL_VEL_X,    // 5
    COL_VEL_Y,    // 6
    COL_IMG_LOC_X,// 7
    COL_IMG_LOC_Y,// 8
    COL_MIN_X,    // 9
    COL_MIN_Y,    // 10
    COL_MAX_X,    // 11
    COL_MAX_Y,    // 12
    COL_AREA,     // 13
    COL_WORLD_X,  // 14
    COL_WORLD_Y,  // 15
    COL_WORLD_Z,  // 16
    COL_TIME,     // 17
    COL_CONFIDENCE// 18
  };

} // end namepsace


// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
track_reader_kw18
::track_reader_kw18()
: in_stream_(0),
  read_pixel_data_(false),
  been_read_(false)
{
}


track_reader_kw18
::~track_reader_kw18()
{
}


bool track_reader_kw18
::set_path_to_images( std::string const& path )
{
  if ( path.empty() )
  {
    this->read_pixel_data_ = false;
    return false;
  }

  std::string glob_str = path;
  std::string path_str = path;
  char lastc = *path.rbegin();
  if ( ( lastc != '\\' ) && ( lastc != '/' ) )
  {
    glob_str += "/";
    path_str += "/";
  }

  glob_str += "*";

  // Collect a list of file names. There will be one file per frame.
  for ( vul_file_iterator iter = glob_str; iter; ++iter )
  {
    // Check file "format" to make sure it is readable by vil_load()
    if ( ( vul_file::extension( iter.filename() ) == ".jpg" ) ||
         ( vul_file::extension( iter.filename() ) == ".png" ) ||
         ( vul_file::extension( iter.filename() ) == ".jpeg" ) )
      image_names_.push_back( path_str + iter.filename() );
  }

  this->read_pixel_data_ = ( ! image_names_.empty() );
  return this->read_pixel_data_;
}


// ----------------------------------------------------------------
/** Open the file or die trying.
 *
 *
 */
bool track_reader_kw18
::open( std::string const& filename )
{
  bool status( false );

  if ( 0 != in_stream_ ) // meaning we are already open
  {
    return filename_ == filename;
  }

  filename_ = filename;

  if ( ! validate_file() )
  {
    return false;
  }

  std::ifstream fstr( this->filename_.c_str() );
  if ( ! fstr )
  {
    LOG_ERROR( "kw18 reader could not open " << this->filename_ << " for reading." );
    return false;
  }

  // build boost filter to remove comments and blank lines
  // order is important.
  in_stream_ = new boost::iostreams::filtering_istream();

  in_stream_->push( vidtk::blank_line_filter() );
  in_stream_->push( vidtk::shell_comments_filter() );
  in_stream_->push( fstr );

  // If a path to images is specified, then get images
  if ( ! this->reader_options_.get_path_to_images().empty() )
  {
    status = set_path_to_images( this->reader_options_.get_path_to_images() );
    if ( ! status )
    {
      LOG_ERROR( "kw18 reader could not load images from "
                 << this->reader_options_.get_path_to_images() );
      return false;
    }
  }

  // Read all tracks from file
  if ( read_all_tracks() )
  {
    sort_terminated( all_tracks_ );
    return true;
  }

  return false;
} // open



bool
track_reader_kw18
::read_next_terminated( vidtk::track::vector_t& datum, unsigned& frame )
{
  if (this->current_terminated_ == this->terminated_at_.end() )
  {
    return false;  // end of tracks
  }

  datum = this->current_terminated_->second;
  frame = this->current_terminated_->first; // frame number

  ++this->current_terminated_;  // point to next set of tracks

  return true;
}


size_t
track_reader_kw18
::read_all( vidtk::track::vector_t& datum )
{
  if (been_read_)
  {
    return 0;
  }

  been_read_ = true;

  // Tracks have already been read in by open()
  datum = this->all_tracks_;
  return datum.size();
}


// ----------------------------------------------------------------
/** Read all tracks from stream.
 *
 * Return false if error; true if tracks are read and vectorized.
 */
bool
track_reader_kw18
::read_all_tracks()
{
  std::string line;
  id_map_t id_to_index;

  std::vector< std::string > col;
  std::vector< image_object_sptr > objs( 1 );

  while (std::getline(*in_stream_, line))
  {
    col.clear();
    boost::char_separator < char > sep(" ");
    typedef boost::tokenizer < boost::char_separator<char> > tok_t;
    tok_t tok( line, sep );
    for(tok_t::iterator it = tok.begin(); it != tok.end(); ++it)
    {
      col.push_back( *it );
    }

    // skip lines suppressed by filtered stream
    if ( col.empty() )
    {
      continue;
    }

    if ( ( col.size() < 18 ) || ( col.size() > 20 ) )
    {
      LOG_ERROR( "This is not a kw18 kw19 or kw20 file; found " << col.size() <<
                 " columns in\n\"" << line << "\"" );
      return false;
    }

    /*
     * Check to see if we have seen this track before. If we have,
     * then retrieve the track's index into our output vector. If not
     * seen before, add track id -> track vector index to our map and
     * press on.
     *
     * This allows for track states to be written in a non-contiguous
     * manner as may be done by streaming writers.
     */
    int index;
    int id = atoi( col[0].c_str() );
    id_map_t::iterator itr = id_to_index.find( id );
    if ( itr == id_to_index.end() )
    {
      // create a new track
      vidtk::track_sptr to_push = new vidtk::track;
      to_push->set_id( id );
      this->all_tracks_.push_back( to_push );

      index = this->all_tracks_.size() - 1;
      id_to_index[id] = index;
    }
    else
    {
      index = itr->second;
    }

    vidtk::track_state_sptr new_state = new vidtk::track_state;

    new_state->time_.set_frame_number( atoi( col[COL_FRAME].c_str() ) );
    new_state->loc_[0] = atof( col[COL_LOC_X].c_str() );
    new_state->loc_[1] = atof( col[COL_LOC_Y].c_str() );
    new_state->loc_[2] = 0;

    new_state->vel_[0] = atof( col[COL_VEL_X].c_str() );
    new_state->vel_[1] = atof( col[COL_VEL_Y].c_str() );
    new_state->vel_[2] = 0;

    image_object_sptr obj = new image_object;
    obj->set_image_loc( atof( col[COL_IMG_LOC_X].c_str() ),
                        atof( col[COL_IMG_LOC_Y].c_str() ));

    int min_x = atoi( col[COL_MIN_X].c_str() );
    int max_x = atoi( col[COL_MAX_X].c_str() );
    int min_y = atoi( col[COL_MIN_Y].c_str() );
    int max_y = atoi( col[COL_MAX_Y].c_str() );

    obj->set_bbox(
      ( min_x < 0 ? 0 : min_x ),
      ( max_x < 0 ? 0 : max_x ),
      ( min_y < 0 ? 0 : min_y ),
      ( max_y < 0 ? 0 : max_y ) );

    obj->set_area( atof( col[COL_AREA].c_str() ) );

    // If specified, we need to interpret the world location as a
    // lat/lon coordinate
    if ( this->reader_options_.get_read_lat_lon_for_world() )
    {
      new_state->set_latitude_longitude( atof( col[COL_WORLD_Y].c_str() ), // latitude
                                         atof( col[COL_WORLD_X].c_str() ) ); // longitude
    }
    else
    {
      obj->set_world_loc(atof( col[COL_WORLD_X].c_str() ), // x
                         atof( col[COL_WORLD_Y].c_str() ), // y
                         atof( col[COL_WORLD_Z].c_str() )); // z
    }

    if ( this->read_pixel_data_ )
    {
      if (new_state->time_.frame_number() >= this->image_names_.size())
      {
        LOG_ERROR( "kw18 reader does not have image file name for this frame" );
      }

      //Read the image from file
      vil_image_view< vxl_byte > src_img = vil_load( this->image_names_[new_state->time_.frame_number()].c_str() );
      if ( ! src_img )
      {
        LOG_ERROR( "kw18 reader could not load image from file \""
                   <<  this->image_names_[new_state->time_.frame_number()]
                   << "\"" );
      }

      const vgl_box_2d< unsigned >& obj_bbox = obj->get_bbox();
      vil_image_view< vxl_byte > img_crop = vil_crop( src_img,
                obj_bbox.min_x(), obj_bbox.max_x() - obj_bbox.min_x(),
                obj_bbox.min_y(), obj_bbox.max_y() - obj_bbox.min_y() );

      vil_image_view< vxl_byte > bbox_pxls;
      bbox_pxls.deep_copy( img_crop );

      vil_image_resource_sptr data = vil_new_image_resource_of_view( bbox_pxls );
      obj->set_image_chip( data, 0u );
    } // end read pixel data

    objs[0] = obj;
    new_state->data_.set( tracking_keys::img_objs, objs );

    new_state->amhi_bbox_.set_min_x( atoi( col[COL_MIN_X].c_str() ) );
    new_state->amhi_bbox_.set_min_y( atoi( col[COL_MIN_Y].c_str() ) );
    new_state->amhi_bbox_.set_max_x( atoi( col[COL_MAX_X].c_str() ) );
    new_state->amhi_bbox_.set_max_y( atoi( col[COL_MAX_Y].c_str() ) );

    // time_ is a timestamp which keeps time in microseconds.  All
    // kw18 files so far have time written in seconds so, convert from
    // seconds to microseconds.
    new_state->time_.set_time( atof( col[COL_TIME].c_str() ) * 1e6 );

    if( col.size() == 19 )
    {
      new_state->set_track_confidence( atof( col[COL_CONFIDENCE].c_str() ) );
    }

    this->all_tracks_[index]->add_state( new_state );

  } // ...while !eof

  return true;
} // read_all_tracks


// ----------------------------------------------------------------
/** Validate file type.
 *
 * This method inspects the file to see if it is of the expected
 * type. Currently, if the filename ends in '.kw18' or the first data
 * line has 18 fields, the file is considered o.k.
 *
 * @return \c true is returned if the file format is recognized, \c
 * false otherwise.
 */

bool
track_reader_kw18
::validate_file ()
{

  //
  std::string const file_ext = vul_file::extension( filename_ );
  if ( boost::iequals( file_ext, ".kw18") )
  {
    return true;
  }

  std::ifstream fstr( this->filename_.c_str() );
  if( ! fstr )
  {
    LOG_WARN( "kw18 reader could not open file \"" << this->filename_ << "\"");
    return false;               // not valid file
  }

  // build boost filter to remove comments and blank lines
  // order is important.
  boost::iostreams::filtering_istream local_stream;

  local_stream.push (vidtk::blank_line_filter());
  local_stream.push (vidtk::shell_comments_filter());
  local_stream.push (fstr);

  double dbl_track_id, dbl_n_frames, dbl_frame_num; // fields 0, 1, 2
  double loc_x, loc_y, vel_x, vel_y; // fields 3, 4, 5, 6
  double obj_loc_x, obj_loc_y;   // fields 7,8
  double bb_c1_x, bb_c1_y, bb_c2_x, bb_c2_y; // fields 9, 10, 11, 12
  double area, world_x, world_y, world_z; // fields 13, 14, 15, 16
  double timestamp; // field 17
  std::string s;

  // skip a few empty lines
  while (std::getline(local_stream, s) && s.empty())
  {
  }

  // This is a little weak. There can be up to 20 numbers on a
  // record. At least we validate that the fields are numbers.
  int count = std::sscanf( s.c_str(),
                          "%lf %lf %lf "
                          "%lf %lf %lf %lf "
                          "%lf %lf %lf %lf %lf %lf "
                          "%lf %lf %lf %lf %lf",
                          &dbl_track_id,
                          &dbl_n_frames,
                          &dbl_frame_num,

                          &loc_x,
                          &loc_y,
                          &vel_x,
                          &vel_y,

                          &obj_loc_x,
                          &obj_loc_y,
                          &bb_c1_x,
                          &bb_c1_y,
                          &bb_c2_x,
                          &bb_c2_y,

                          &area,
                          &world_x,
                          &world_y,
                          &world_z,
                          &timestamp );
  if (count != 18 )
  {
    LOG_DEBUG( "Incorrect data format for kw18 file");
    return false;
  }

  return true;
}

} // end namespace
} // end namespace
