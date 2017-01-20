/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reader_vsl.h"

#include <string>
#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vul/vul_file.h>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <tracking_data/vsl/track_io.h>
#include <tracking_data/vsl/image_object_io.h>
#include <utilities/vsl/timestamp_io.h>
#include <logger/logger.h>
#include <memory>
#include <boost/algorithm/string.hpp>


namespace vidtk {
namespace ns_track_reader {

VIDTK_LOGGER( "track_reader_vsl" );

track_reader_vsl
::track_reader_vsl()
  : bin_( NULL ),
    been_read_(false)
{
}


track_reader_vsl
::~track_reader_vsl()
{
  delete this->bin_;
}


// ----------------------------------------------------------------
/** Open and verify file type.
 *
 *
 */
bool
track_reader_vsl
::open( std::string const& filename )
{
  if ( 0 != this->bin_ )
  {
    return filename_ == filename;
  }

  filename_ = filename;

  // Currently the only semantic test for our file type is the file
  // extension
  std::string const file_ext = vul_file::extension( filename_ );
  if ( ! boost::iequals( file_ext, ".vsl" ) )
  {
    return false;
  }

  this->bin_ = NULL;
  std::auto_ptr< vsl_b_ifstream > local_stream( new vsl_b_ifstream( this->filename_ ) );
  if ( ! local_stream->is() )
  {
    LOG_ERROR( "vsl track reader couldn't open " << this->filename_ << " for reading." );
    return false;
  }

  // read in header and version
  std::string header;
  int version;

  vsl_b_read( *local_stream, header );
  vsl_b_read( *local_stream, version );

  if ( ( "track_vsl_stream" != header ) || ( 2 != version ) )
  {
    LOG_ERROR( "Unexpected version from VSL file. Found " << version
               << " expected 2." );
    return false;
  }

  // check for residual errors
  if ( ! *local_stream )
  {
    return false;
  }

  // Save stream now that we know it is a good file
  this->bin_ = local_stream.release();
  return true;
} // open


// ----------------------------------------------------------------
/** \brief Read tracks for the next frame.
 *
 * The set of tracks for the current frame is returned via the
 * parameter.
 *
 * This behaviour of this reader is different from the other readers
 * in that the data file was written out on a frame by frame basis, so
 * there is an entry for each frame that contains zero or more
 * terminated tracks and associated data.
 *
 * It is desirable to leave this reader operating on a frame by frame
 * basis (rather than reading all tracks) since some track files are
 * very large and would stress memory resources if read into memory
 * all at once.
 *
 * Also note that this captures the whole track and states data
 * structures, which can be rather large.
 *
 * @param[out] tracks Tracks for this frame.
 * @param[out] frame Frame number on which these tracks were
 * terminated. This is done on a best-effort bases, since there may be
 * cases where the frame/timestamp is not available.
 */

bool track_reader_vsl
  ::read_next_terminated( vidtk::track::vector_t& tracks, unsigned& frame )
{
  tracks.clear();
  frame = 0;

  if ( ! this->bin_->is() ) { return false; }

  this->bin_->clear_serialisation_records();

  bool hasTracks;
  vsl_b_read( *( this->bin_ ), hasTracks );
  if ( ! this->bin_->is() ) { return false; }

  if ( hasTracks )
  {
    vsl_b_read( *( this->bin_ ), tracks );
    if ( ! this->bin_->is() ) { return false; }

    // Just because it has tracks, there is a vector in the file.
    // Doesn't mean there are any tracks really in there.
    if ( ! tracks.empty() )
    {
      // get frame from first track
      frame = tracks[0]->last_state()->get_timestamp().frame_number() +1;
    }
  }

  bool hasObjs;
  vsl_b_read( *( this->bin_ ), hasObjs );
  if ( ! this->bin_->is() ) { return false; }

  if ( hasObjs )
  {
    // Not really sure what to do with these yet
    vidtk::timestamp ts;
    std::vector< vidtk::image_object_sptr > objs;

    vsl_b_read( *( this->bin_ ), ts );
    vsl_b_read( *( this->bin_ ), objs );
    if ( ! this->bin_->is() ) { return false; }
  }

  return true;
}


// ----------------------------------------------------------------
size_t
track_reader_vsl
::read_all( vidtk::track::vector_t& datum )
{
  bool result(false);
  unsigned frame;

  datum.clear();
  if ( been_read_ )
  {
    return 0;
  }

  do
  {
    vidtk::track::vector_t step_tracks;
    result = this->read_next_terminated( step_tracks, frame );
    datum.insert( datum.end(), step_tracks.begin(), step_tracks.end() );
  } while(result);

  been_read_ = true;

  return datum.size();
}

} // end namespace
} // end namespace
