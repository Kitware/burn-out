//ckwg

#include "track_writer_process.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <tracking/vsl/track_io.h>
#include <tracking/vsl/image_object_io.h>
#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>
#include <utilities/vsl/timestamp_io.h>



namespace vidtk
{
track_writer_process
::track_writer_process( vcl_string const& name )
  : process( name, "track_writer_process" ),
    disabled_( true ),
    format_( 0 ),
    allow_overwrite_( false ),
    suppress_header_( true ),
    src_objs_( NULL ),
    src_trks_( NULL ),
    fstr_(),
    bfstr_( NULL ),
    write_lat_lon_for_world_( false )
{
  config_.add_parameter( "disabled", "true", "Disables the writing process" );
  config_.add_parameter( "format", "columns_tracks", "The format of the output."
                         "  Options include: mit, kw18, kw20, and vsl ");
  config_.add_parameter( "filename", "", "The output file" );
  config_.add_parameter( "overwrite_existing", "false", 
                         "Weither or not a file can be overwriten" );
  config_.add_parameter( "suppress_header", "false", 
                         "Whether or not to write the header" );
  config_.add_parameter( "path_to_images","", 
                         "Path to the images (used only for writing mit files)");
  config_.add_parameter( "frequency","0", 
                         "The number of frames per second");
  config_.add( "x_offset", "0");
  config_.add( "y_offset", "0");
  config_.add_parameter( "write_lat_lon_for_world", "false", 
                         "Write lat/lon values in the world coordinate fields");
}


track_writer_process
::~track_writer_process()
{
  fstr_.close();

  delete bfstr_;
}


config_block
track_writer_process
::params() const
{
  return config_;
}


bool
track_writer_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    vcl_string fmt_str;
    blk.get( "format", fmt_str );
    blk.get( "filename", filename_ );
    blk.get( "overwrite_existing", allow_overwrite_ );
    blk.get( "suppress_header", suppress_header_ );
    blk.get( "path_to_images", path_to_images_ );
    blk.get( "x_offset", x_offset_);
    blk.get( "y_offset", y_offset_);
	  blk.get( "frequency", frequency_ );

    format_ = unsigned(-1);
    if( fmt_str == "columns_tracks" || fmt_str == "kw18" ||
                   fmt_str == "trk" || fmt_str == "dat")//KW18
    {
      format_ = 0;
    }
    else if( fmt_str == "vsl" )//Binary
    {
      format_ = 1;
    }
    else if( fmt_str == "columns_tracks_and_objects" ||
             fmt_str == "kw20" )//KW20
    {
      format_ = 2;
    }
    else if( fmt_str == "xml" || fmt_str == "mit")//MIT
    {
      format_ = 3;
    }
    //If the format is not defined use the extension of the file
    else
    {
      vcl_string file_ext = filename_.substr(filename_.rfind(".")+1);
      if( file_ext == "kw18" ||  file_ext == "trk" || file_ext == "dat")//KW20
      {
        format_ = 0;
      }
      else if( file_ext == "vsl" )
      {
        format_ = 1;
      }
      else if( file_ext == "kw20" )
      {
        format_ = 2;
      }
      else if( file_ext == "xml" )
      {
        format_ = 3;
      }
      else
      {
        log_error( name() << ": unknown format " << fmt_str << "\n" );
      }
    }

    if(format_ == 3)
    {
        if(path_to_images_ == "")
        {
          log_error( name() << ": Path to images must be set for mit files" );
        }
    }

    // Validate the format
    if( format_ > 3 )
    {
      throw unchecked_return_value("");
    }

    blk.get( "write_lat_lon_for_world", write_lat_lon_for_world_ );
  }
  catch( unchecked_return_value& )
  {
    // Reset to old values
    this->set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
track_writer_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  if( fail_if_exists( filename_ ) )
  {
    log_error( name() << "File: "
                 << filename_ << " already exists. To overwrite set allow_overwrite flag\n" );
    return false;
  }

  if( format_ == 0 || format_ == 2 || format_ == 3 )
  {
    fstr_.open( filename_.c_str() );
    if( ! fstr_ )
    {
      log_error( name() << ": Couldn't open "
                 << filename_ << " for writing.\n" );
      return false;
    }

    if( !suppress_header_ )
    {
      if( format_ == 0 )
      {
        fstr_ <<"# 1:Track-id  2:Track-length  "
          <<"3:Frame-number  4-5:Tracking-plane-loc(x,y)  6-7:velocity(x,y)  "
          <<"8-9:Image-loc(x,y)  10-13:Img-bbox(TL_x,TL_y,BR_x,BR_y)  "
          <<"14:Area  15-17:World-loc";
        if( write_lat_lon_for_world_ )
        {
          fstr_ <<"(longitude,latitude,0-when-available,444-otherwise) ";
        }
        else
        {
          fstr_ <<"(x,y,z) ";
        }
        fstr_ <<" 18:timesetamp  19:object-type-id  20:activity-type-id\n";
      }
      else if( format_ == 2 )
      {
        fstr_ <<"# 1:Track-id[-1_for_detections]  2:Track-length[-1_for_detections]  "
          <<"3:Frame-number  4-5:Tracking-plane-loc(x,y)[-1_for_detections]  "
          <<"6-7:velocity(x,y)[-1_for_detections]  "
          <<"8-9:Image-loc(x,y)  10-13:Img-bbox(TL_x,TL_y,BR_x,BR_y)  "
          <<"14:Area  15-17:World-loc(x,y,z) 18:timesetamp  "
          <<"19:object-type-id  20:activity-type-id\n";
      }
    }
  }
  else if( format_ == 1 )
  {
    bfstr_ = new vsl_b_ofstream( filename_ );
    if( ! *bfstr_ )
    {
      log_error( name() << ": Couldn't open "
                        << filename_ << " for writing.\n" );
      delete bfstr_;
      bfstr_ = NULL;
      return false;
    }

    vsl_b_write( *bfstr_, vcl_string("track_vsl_stream") );
    vsl_b_write( *bfstr_, 2 ); // version number
    bfstr_->os().flush();
  }

  return true;
}


bool
track_writer_process
::step()
{
  if( disabled_ )
  {
    return false;
  }

  if( src_trks_ == NULL )
  {
    vcl_cerr<< name() << ": source tracks not specified."<< vcl_endl;
    return false;
  }

  if( format_ == 2 && src_objs_ == NULL )
  {
    vcl_cerr<< name() << ": source objects not specified."<< vcl_endl;
    return false;
  }

  switch( format_ )
  {
  case 0:   write_format_0(); break;
  case 1:   write_format_vsl(); break;
  case 2:   write_format_2(); break;
  case 3:   write_format_mit(); break;
  default:  ;
  }

  src_trks_ = NULL;
  src_objs_ = NULL;
  ts_ = timestamp();

  return true;
}


void
track_writer_process
::set_image_objects( vcl_vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}


void
track_writer_process
::set_tracks( vcl_vector< track_sptr > const& trks )
{
  src_trks_ = &trks;
}


void
track_writer_process
::set_timestamp( timestamp const& ts )
{
  ts_ = ts;
}


bool
track_writer_process
::is_disabled() const
{
  return disabled_;
}


/// \internal
///
/// Returns \c true if the file \a filename exists and we are not
/// allowed to overwrite it (governed by allow_overwrite_).
bool
track_writer_process
::fail_if_exists( vcl_string const& filename )
{
  return ( ! allow_overwrite_ ) && vul_file::exists( filename );
}


/// Format_0: (columns_tracks) tracks only in KW20 format
///
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
/// \li Column(s) 19: Object type id #(-1 if not available)
/// \li Column(s) 20: Activity type id #(-1 if not available)

void
track_writer_process
::write_format_0()
{
  // if you change this format, change the documentation above and change the header output in initialize();
  unsigned N = (*src_trks_).size();
  for( unsigned i = 0; i < N; ++i )
  {
    vcl_vector< vidtk::track_state_sptr > const& hist = (*src_trks_)[i]->history();
    unsigned M = hist.size();
    for( unsigned j = 0; j < M; ++j )
    {
      fstr_.precision( 20 ); //originally this was 3.  That resulted in time being truncated.  3 is evil.
                             //The floating-point precision determines the maximum number of digits to
                             //be written on insertion operations to express floating-point values.
      fstr_ << (*src_trks_)[i]->id() << " ";
      fstr_ << M << " ";
      if( hist[j]->time_.has_frame_number() )
      {
        fstr_ <<hist[j]->time_.frame_number() << " ";
      }
      else
      {
        fstr_ << "-1" << " ";
      }

      fstr_ << hist[j]->loc_(0) << " ";
      fstr_ << hist[j]->loc_(1) << " ";
      fstr_ << hist[j]->vel_(0) << " ";
      fstr_ << hist[j]->vel_(1) << " ";
      vcl_vector<vidtk::image_object_sptr> objs;
      if( ! hist[j]->data_.get( vidtk::tracking_keys::img_objs, objs ) || objs.empty() )
      {
        fstr_ << hist[j]->loc_(0) << " ";
        fstr_ << hist[j]->loc_(1) << " ";
        fstr_ << hist[j]->amhi_bbox_.min_x()+x_offset_ << " ";
        fstr_ << hist[j]->amhi_bbox_.min_y()+y_offset_ << " ";
        fstr_ << hist[j]->amhi_bbox_.max_x()+x_offset_ << " ";
        fstr_ << hist[j]->amhi_bbox_.max_y()+y_offset_ << " ";

        fstr_ << "0 0 0 0 ";
        log_warning( "no MOD for track " << (*src_trks_)[i]->id() << ", state " << j << "\n" );
      }
      else
      {
        vidtk::image_object const& o = *objs[0];
        fstr_ << o.img_loc_ << " ";
        fstr_ << o.bbox_.min_x()+x_offset_ << " ";
        fstr_ << o.bbox_.min_y()+y_offset_ << " ";
        fstr_ << o.bbox_.max_x()+x_offset_ << " ";
        fstr_ << o.bbox_.max_y()+y_offset_ << " ";
        fstr_ << o.area_ << " ";
        
        //
        // Write out longitude, latitude in world coordinates if:
        //  1. lat/lon are available
        //  2. class level write_lat_lon_for_world_ flag is set.
        //
        if (  write_lat_lon_for_world_ ) 
        {
          // Invalid default values to be written out to the file to 
          // to indicate missing values. 
          double lon = 444, lat = 444; 
          hist[j]->latitude_longitude( lat, lon );
          fstr_ << lon << " " << lat << " " << 0.0 << " ";
        } 
        else 
        {
          fstr_ << o.world_loc_ << " ";
        }
      }
      //if( hist[j]->time_.time_in_secs() )
      //  fstr_ << hist[j]->time_.time_in_secs() << " ";
      //else
      //  fstr_ << "-1" << " ";
      //timestamp is in seconds
      if( hist[j]->time_.has_time() )
      {
        fstr_ << hist[j]->time_.time_in_secs() << " ";
      }
      else if ( (frequency_ != 0) && hist[j]->time_.has_frame_number() )
      {
        fstr_ << static_cast<double>(hist[j]->time_.frame_number())/frequency_ << " ";
      }
      else
      {
        fstr_ << "-1" << ' ';
      }
      fstr_ << "-1" << " ";
      fstr_ << "-1" << "\n";
    }
  }
  fstr_.flush();
}

/// Format 1
///
/// This is a binary format using the vsl library.
///
void
track_writer_process
::write_format_vsl()
{
  // each step() should result in an independent set of objects.
  bfstr_->clear_serialisation_records();

  if( src_trks_ )
  {
    vsl_b_write( *bfstr_, true );
    vsl_b_write( *bfstr_, *src_trks_ );

  }
  else
  {
    vsl_b_write( *bfstr_, false );
  }

  if( src_objs_ )
  {
    vsl_b_write( *bfstr_, true );
    vsl_b_write( *bfstr_, ts_ );
    vsl_b_write( *bfstr_, *src_objs_ );
  }
  else
  {
    vsl_b_write( *bfstr_, false );
  }
  bfstr_->os().flush();
}


/// Format_2: (columns_tracks_and_objects) tracks and detections in KW20 format
///
/// This format adds object detections in the same file as tracks.
///
/// \li Column(s) 1: Track-id (-1 for detection entries)
/// \li Column(s) 2: Track-length (# of detections, -1 for detection entries)
/// \li Column(s) 3: Frame-number (-1 if not available)
/// \li Column(s) 4-5: Tracking-plane-loc(x,y) (Could be same as World-loc, -1 for detection entries)
/// \li Column(s) 6-7: Velocity(x,y) (-1 for detection entries)
/// \li Column(s) 8-9: Image-loc(x,y)
/// \li Column(s) 10-13: Img-bbox(TL_x,TL_y,BR_x,BR_y) (location of top-left & bottom-right vertices)
/// \li Column(s) 14: Area
/// \li Column(s) 15-17: World-loc(x,y,z)
/// \li Column(s) 18: Timesetamp(-1 if not available)
/// \li Column(s) 19: Object type id #(-1 if not available)
/// \li Column(s) 20: Activity type id #(-1 if not available)

void
track_writer_process
::write_format_2()
{
  // if you change this format, change the documentation above and change the header output in initialize();

  unsigned N = (*src_trks_).size();
  for( unsigned i = 0; i < N; ++i )
  {
    vcl_vector< vidtk::track_state_sptr > const& hist = (*src_trks_)[i]->history();
    unsigned M = hist.size();
    for( unsigned j = 0; j < M; ++j )
    {
      fstr_ << (*src_trks_)[i]->id() << " ";
      fstr_ << M << " ";
      if( hist[j]->time_.has_frame_number() )
        fstr_ <<hist[j]->time_.frame_number() << " ";
      else
        fstr_ << "-1" << " ";
      fstr_ << hist[j]->loc_(0) << " ";
      fstr_ << hist[j]->loc_(1) << " ";
      fstr_ << hist[j]->vel_(0) << " ";
      fstr_ << hist[j]->vel_(1) << " ";
      vcl_vector<vidtk::image_object_sptr> objs;
      if( ! hist[j]->data_.get( vidtk::tracking_keys::img_objs, objs ) || objs.empty() )
      {
        log_error( "no MOD for track " << (*src_trks_)[i]->id() << ", state " << j << "\n" );
        continue;
      }
      vidtk::image_object const& o = *objs[0];
      fstr_ << o.img_loc_ << " ";
      fstr_ << o.bbox_.min_x()+x_offset_ << " ";
      fstr_ << o.bbox_.min_y()+y_offset_ << " ";
      fstr_ << o.bbox_.max_x()+x_offset_ << " ";
      fstr_ << o.bbox_.max_y()+y_offset_ << " ";
      fstr_ << o.area_ << " ";
      fstr_ << o.world_loc_ << " ";
      if( hist[j]->time_.has_time() )
        fstr_ << hist[j]->time_.time_in_secs() << " ";
      else
        fstr_ << "-1" << " ";
      fstr_ << "-1";
      fstr_ << "-1";
      fstr_ << "\n";
    }
  }

  N = src_objs_->size();
  for( unsigned i = 0; i < N; ++i )
  {
    image_object const& o = *(*src_objs_)[i];

    fstr_ << "-1" << " "; //track-id
    fstr_ << "-1" << " "; //track-length
    if( ts_.has_frame_number() )
      fstr_ << ts_.frame_number() << " ";
    else
      fstr_ << "-11" << " ";
    fstr_ << "-1" << " "; //tracker-plane-loc-x
    fstr_ << "-1" << " "; //tracker-plane-loc-y
    fstr_ << "-1" << " "; //tracker-plane-vel-x
    fstr_ << "-1" << " "; //tracker-plane-vel-y
    fstr_ << o.img_loc_ << " ";
    fstr_ << o.bbox_.min_x()+x_offset_ << " ";
    fstr_ << o.bbox_.min_y()+y_offset_ << " ";
    fstr_ << o.bbox_.max_x()+x_offset_ << " ";
    fstr_ << o.bbox_.max_y()+y_offset_ << " ";
    fstr_ << o.area_ << " ";
    fstr_ << o.world_loc_ << " ";
    if( ts_.has_time() )
      fstr_ << ts_.time_in_secs() << " ";
    else
      fstr_ << "-1" << " ";
    fstr_ << "-1";
    fstr_ << "-1";
    fstr_ << "\n";
  }
  fstr_.flush();
}

void
track_writer_process
::write_format_mit()
{

  vcl_string file_names = "";
  int num_images = 0;

  if(vul_file::exists( path_to_images_) )
  {
    vcl_string glob_str = path_to_images_;
    char lastc = path_to_images_.c_str()[path_to_images_.length()-1];
    if(lastc != '\\' || lastc != '/')
    {
      glob_str += "/";
    }
    glob_str += "*";
    for(vul_file_iterator iter = glob_str ; iter; ++iter)
    {
      if(vul_file::extension(iter.filename()) == ".jpg" || 
         vul_file::extension(iter.filename()) == ".png" || 
         vul_file::extension(iter.filename()) == ".jpeg")
      {
        file_names += "      <fileName>";
        file_names += iter.filename();
        file_names += "</fileName>\n";
        num_images++;
      }
    }
  }
  fstr_ << "\
<annotation>\n\
    <version>1.00</version>\n\
    <videoType>frames</videoType>\n\
    <folder>" << path_to_images_ << "</folder>\n\
    <NumFrames>" << num_images << "</NumFrames>\n\
    <currentFrame>0</currentFrame>\n\
    <fileList>\n" <<
      file_names << "\
    </fileList>\n\
    <source>\n\
        <sourceImage>videos taken by Antonio Torrabla</sourceImage>\n\
        <sourceAnnotation>Qt-based C++ video labeling tool (by Ce Liu, Aug 2007)</sourceAnnotation>\n\
    </source>\n";

  unsigned N = (*src_trks_).size();
  for( unsigned i = 0; i < N; ++i )
  {
    vcl_vector< vidtk::track_state_sptr > const& hist = (*src_trks_)[i]->history();
    unsigned M = hist.size();
    fstr_ << "\
            <object>\n\
              <name>"<<(*src_trks_)[i]->id()<<"</name>\n\
              <notes></notes>\n\
              <deleted>0</deleted>\n\
              <verified>0</verified>\n\
              <date>20-Aug-2008 10:27:25</date>\n\
              <id>"<<(*src_trks_)[i]->id()<<"</id>\n\
              <username>anonymous</username>\n\
              <numFrames>"<<num_images<<"</numFrames>\n\
              <startFrame>"<<hist[0]->time_.frame_number()<<"</startFrame>\n\
              <endFrame>"<<hist[M-1]->time_.frame_number()<<"</endFrame>\n\
              <createdFrame>"<<hist[0]->time_.frame_number()<<"</createdFrame>\n\
              <labels>\n";

    for( unsigned j = 0; j < M; ++j )
    {
      fstr_ << "\
                  <frame>\n\
                      <index>"<<hist[j]->time_.frame_number()<<"</index>\n\
                      <depth>200</depth>\n\
                      <globalLabel>1</globalLabel>\n\
                      <localLabel>1</localLabel>\n\
                      <tracked>1</tracked>\n\
                      <depthLabel>1</depthLabel>\n\
                      <isPolygon>1</isPolygon>\n\
                      <polygon>\n\
                          <pt>\n\
                               <x>"<<hist[j]->amhi_bbox_.min_x()+x_offset_<<"</x>\n\
                               <y>"<<hist[j]->amhi_bbox_.min_y()+y_offset_<<"</y>\n\
                               <labeled>1</labeled>\n\
                          </pt>\n\
                          <pt>\n\
                               <x>"<<hist[j]->amhi_bbox_.min_x()+x_offset_<<"</x>\n\
                               <y>"<<hist[j]->amhi_bbox_.max_y()+y_offset_<<"</y>\n\
                               <labeled>1</labeled>\n\
                          </pt>\n\
                          <pt>\n\
                               <x>"<<hist[j]->amhi_bbox_.max_x()+x_offset_<<"</x>\n\
                               <y>"<<hist[j]->amhi_bbox_.max_y()+y_offset_<<"</y>\n\
                               <labeled>1</labeled>\n\
                          </pt>\n\
                          <pt>\n\
                               <x>"<<hist[j]->amhi_bbox_.max_x()+x_offset_<<"</x>\n\
                               <y>"<<hist[j]->amhi_bbox_.min_y()+y_offset_<<"</y>\n\
                               <labeled>1</labeled>\n\
                          </pt>\n\
                       </polygon>\n\
                  </frame>\n";
    }
    fstr_ << "\
              </labels>\n\
          </object>\n";
  }
  fstr_ << "</annotation>\n";
  fstr_.flush();
}
} // end namespace vidtk
