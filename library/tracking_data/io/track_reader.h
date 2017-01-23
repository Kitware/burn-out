/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_TRACK_READER_H_
#define _VIDTK_TRACK_READER_H_


#include <string>
#include <tracking_data/track.h>
#include <tracking_data/io/track_reader_interface.h>


namespace vidtk {


// ----------------------------------------------------------------
/** Universal track reader.
 *
 * This class converts track data from a disk file into the memory
 * resident structures.
 *
 * The intent of this class is that an object is instantiated with the
 * filename each time a file needs to be read. There is explicitly no
 * close() method. The object is not to be reused. When done reading
 * the file, delete the reader and make a new one.
 *
 * There are many file formats that are handled internally. If you
 * have a custom file format that you need supported, you will need to
 * write a new subclass of track_reader_interface.
 *
 * Add an instance of the new reader immediately after creating a
 * reader object so it will be available when needed to open the file.
 *
 * Implementing a read method that orders tracks by track termination
 * time is additional semantics that I would rather not implement in a
 * class in the physical access layer, but that is a major use case.
 * Pushing track ordering up to the next level requires that all
 * format readers ingest all tracks at once to impress desired
 * ordering. At least one format (VSL) does not like to always ingest
 * all tracks from a file because there may be bazillions of them,
 * stressing memory reserves. This approach is built upon the implicit
 * assupotion that all VSL format track files are written in track
 * terminated order.
 *
 * @sa ns_track_reader::track_reader_interface
 *
 * Example
 \code
track_reader the_reader(filename);
vidtk::track::vector_t tracks;

// Optionally add custom reader objects here.
// Allocate readers from heap since they will be transferred to
// the track_reader.
the_reader.add_reader( new track_reader_foo() );
// the_reader now owns the foo reader and will delete the object when done.

// Set reader options. Must be done before open() call.
vidtk::ns_track_reader::track_reader_options opt;
opt.set_ignore_occlusions(false)
   .set_ignore_partial_occlusions(flase);

the_reader.update_options( opt );

if (! the_reader.open())
{
    LOG_ERROR("Could not open file " << filename);
}

size_t count = the_reader.read_all(tracks);

 \endcode

\msc
App, track_reader, format_1_reader, format_2_reader, format_foo_reader;

App=>track_reader [ label="<<create>>" ];
track_reader=>format_1_reader [ label="<<create>>" ];
track_reader=>track_reader [ label="add_reader(reader 1)" ];
track_reader=>format_2_reader [ label="<<create>>" ];
track_reader=>track_reader [ label="add_reader(reader 2)" ];

--- [ label="set options" ];
App=>track_reader [ label="set_reader_options(opt)" ];

--- [ label="add custom reader" ];
App=>format_foo_reader [ label="<<create>>" ];
App=>track_reader [ label="add_reader(foo_reader)" ];

--- [ label="later" ];
App=>track_reader [ label="Open()" ];
track_reader=>format_1_reader [ label="open()" ];
track_reader<<format_1_reader [ label="return(false)" ];
track_reader=>format_2_reader [ label="open()" ];
track_reader<<format_2_reader [ label="return(true)" ];

App=>track_reader [ label="read_all()" ];
track_reader=>format_2_reader [ label="read_all()" ];
\endmsc

 */
class track_reader
{
public:
  track_reader( std::string const& filename );
  virtual ~track_reader();

  /** Try to open file.  The open method tries all registered readers
   * to see if the file format is recognised.
   * @retval TRUE - file is opend and ready to read.
   * @retval FALSE - file format not recognised.
   */
  bool open();

  /** Read all tracks from the file. All tracks from the file are
   * returned in the supplied vector.  Note that zero tracks may be
   * returned. Subsequent calls will always return zero tracks, since
   * all tracks have already been returned.
   * @param[out] datum Vector of tracks from file.
   * @return The number of objects read is returned.
   */
  size_t read_all( vidtk::track::vector_t& datum );

  /** Read next terminated track set from the file. The next object is
   * read from the file. False is returned if there are no more
   * obejcts in the file. The returned timestamp indicates when the
   * tracks were terminated. It is possible that the timestamp from
   * sequential calls returns timestamps that are more than one frame
   * apart. If this call returns true and a no tracks, the timestamp
   * may not be valid.
   * @param[out] datum Pointer to track vector
   * @param[out] frame Frame number on which tracks were terminated.
   * @return False is returned if there are no more objects in the
   * file. The values in \c datum and \c frame are undefined if false is
   * returned.  True is returned if a set of terminated tracks is
   * returned from the file.
   */
  bool read_next_terminated( vidtk::track::vector_t& datum, unsigned& frame );

  /** Add new reader to list of supported formats. New readers must be
   * allocated from the heap because the track_reader assumes
   * ownership of the object once it is added and the new reader will
   * be deleted when the track_reader is destroyed. The list of
   * readers is scanned from last added to first added to allow user
   * added readers to supercede a build-in reader.
   * See example code and see below.
   *
   * \code
   * the_reader.add_reader( new track_reader_foo() );
   * \endcode
   *
   * @param[in] reader New file format reader to add.
   */
  void add_reader( ns_track_reader::track_reader_interface* reader );

  /** Set track reader specific options. The updated options from the
   * specified parameter are applied to the reader. Options in the
   * object that are not specifically set are not modified in this
   * reader. This update procedure is used, rather than wholesale
   * copying of all options, to allow specific options to be
   * overridden without having to set all options in a reader specific
   * way.  For example, some readers have different default options.
   *
   * @param[in] opt Track reader options.
   */
  void update_options( vidtk::ns_track_reader::track_reader_options const& opt );


protected:
  typedef boost::shared_ptr< ns_track_reader::track_reader_interface > reader_handle_t;
  std::vector< reader_handle_t > reader_list_;

  std::string filename_;
  reader_handle_t current_reader_;

  vidtk::ns_track_reader::track_reader_options reader_options_;


private:
  int read_mode_;
};

} // end namespace

#endif
