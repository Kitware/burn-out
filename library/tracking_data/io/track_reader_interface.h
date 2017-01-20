/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_TRACK_READER_INTERFACE_H_
#define _VIDTK_TRACK_READER_INTERFACE_H_

#include <tracking_data/track.h>
#include <string>
#include <ostream>

namespace vidtk {
namespace ns_track_reader {

  //
  // Define list of options
  //  ( name, type, default-value )
  //
#define OPTION_LIST(CALL)                                       \
  CALL(path_to_images, std::string, "")                         \
  CALL(ignore_occlusions, bool, true)                           \
  CALL(ignore_partial_occlusions, bool, true)                   \
  CALL(occluder_filename, std::string, "")                      \
  CALL(read_lat_lon_for_world, bool, false)                     \
  CALL(sec_per_frame, double, 0.5)                              \
  CALL(db_type, std::string, "")                                \
  CALL(db_name, std::string, "perseas")                         \
  CALL(db_user, std::string, "perseas")                         \
  CALL(db_pass, std::string, "")                                \
  CALL(db_host, std::string, "localhost")                       \
  CALL(db_port, std::string, "12345")                           \
  CALL(connection_args, std::string, "")                        \
  CALL(append_tracks, bool, true)                               \
  CALL(db_session_id, vxl_int_32, -1)                           \


// ----------------------------------------------------------------
/** Track reader options.
 *
 * This class contains all the tracker reader options that one could
 * imagine. Not all track file formats know what to do with all the
 * options, but since you don't really know what file format you may
 * encounter, you had better set all options the way you want.
 */

class track_reader_options
{
public:
  track_reader_options()
  {
#define INIT_OPTION(N,T,I) this->N ## _ = I; this->N ## _set_ = false;
    OPTION_LIST(INIT_OPTION)
#undef INIT_OPTION
  }


#define METHOD_OPTION(N,T,I)                            \
public:                                                 \
  T const& get_ ## N() const { return (N ## _); }       \
  track_reader_options & set_ ## N(T const& v)          \
  { N ##_ = v; N ## _set_ = true; return (*this); }     \
  bool is_ ## N ## _set() const { return N ## _set_; }
  OPTION_LIST(METHOD_OPTION)
#undef METHOD_OPTION

  private:

#define ALLOCATE_OPTION(N,T,I) T N ## _; bool N ## _set_;
  OPTION_LIST(ALLOCATE_OPTION)
#undef ALLOCATE_OPTION

public:
  void update_from (track_reader_options const& rhs)
  {
#define UPDATE_OPTION(N,T,I) if (rhs.is_ ## N ## _set()) { this->set_ ## N( rhs.N ## _ ); }
    OPTION_LIST(UPDATE_OPTION)
#undef UPDATE_OPTION
  }

  std::ostream& to_stream( std::ostream & str) const
  {
#define STREAM_OPTION(N,T,I)                            \
    str << # N " ("                                     \
        << ( is_ ## N ## _set() ? "set" : "not set" )   \
        << "): " << this->N ## _ << std::endl;
    OPTION_LIST(STREAM_OPTION)
#undef STREAM_OPTION
    return str;
  }

};

#undef OPTION_LIST

inline std::ostream& operator << ( std::ostream& str, track_reader_options const& obj )
{  obj.to_stream( str ); return str; }


// ----------------------------------------------------------------
/** Abstract base class for all track file formats.
 *
 * This class represents the abstract interface for all track
 * readers. Each track file format must implement a class derived from
 * this class.
 */
class track_reader_interface
{
public:
  virtual ~track_reader_interface();

  /** Try to open the file. True is returned if the file format is
   * readable; false otherwise.  If the file format is recognisable,
   * the file in a state ready to read.
   */
  virtual bool open( std::string const& filename ) = 0;

  /** Read next set of terminated track.
   * @param[out] datum Next set of terminated track from file.
   * @param[out] frame Frame number on which tracks were terminated.
   * @return true if data is returned. false if end of file.
   *
   * Note that the returned frame-of-termination is deduced by taking
   * the last frame_number of the track and adding one.  If you're
   * tracking at 10hz on 30hz data, that frame number probably doesn't
   * exist in the system.  Likewise, the synthesis of the frame_number
   * precludes us from deducing the corresponding usecs_timestamp, so
   * we can't return a complete timestamp object.  This is (probably?)
   * why track_reader_process doesn't set tracker_ts_ based on this
   * frame number.
   *
   */
  virtual bool read_next_terminated( vidtk::track::vector_t&  datum,
                                     unsigned&                frame ) = 0;

  /** Read all tracks from file. A vector of all tracks is returned
   * through the parameter. This can only be called once on a
   * source. Subsequnt calls will return 0.
   * @param[out] datum Vector of tracks from file.
   * @return The number of tracks read, so you can use a return of
   * zero to indicate empty file.
   */
  virtual size_t read_all( vidtk::track::vector_t& datum ) = 0;

  /** Set track reader specific options.
   * @param[in] opt Tracker reader options.
   */
  void update_options( vidtk::ns_track_reader::track_reader_options const& opt );


protected:
  track_reader_interface();

  bool file_contains( std::string const& filename,
                      std::string const& text,
                      size_t            nbytes = 1024 );

  // These items are used order tracks by terminated timestamp.  Not
  // all readers will need to do this, but it here because several
  // readers need this capability.
  void sort_terminated( vidtk::track::vector_t& tracks );
  typedef std::map< unsigned int, vidtk::track::vector_t > terminated_map_t;
  terminated_map_t terminated_at_;
  terminated_map_t::iterator current_terminated_;

  std::string filename_;

  track_reader_options reader_options_;

}; // end class track_reader_interface

} // end namespace
} // end namespace

#endif /* _VIDTK_TRACK_READER_INTERFACE_H_ */
