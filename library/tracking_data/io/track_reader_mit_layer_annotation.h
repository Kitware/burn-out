/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _TRACK_READER_MIT_LAYER_ANNOTATION_H_
#define _TRACK_READER_MIT_LAYER_ANNOTATION_H_

#include <tracking_data/io/track_reader_interface.h>
#include <boost/iostreams/filtering_stream.hpp>

class TiXmlNode;

struct occluder;


namespace vidtk {
namespace ns_track_reader {

// ----------------------------------------------------------------
/**
 *
 *
 */
class track_reader_mit_layer_annotation
  : public track_reader_interface
{
public:
  track_reader_mit_layer_annotation();
  ~track_reader_mit_layer_annotation();

  /** Try to open file.  The open method tries to open the file.
   * Returns false if unrecognized file type.
   * @retval TRUE - file is opend and ready to read.
   * @retval FALSE - file format not recognised.
   */
  virtual bool open( std::string const& filename );

  /** Read next terminated track set from the file. The next object is
   * read from the file. False is returned if there are no more
   * obejcts in the file. The returned timestamp indicates when the
   * tracks were terminated. It is possible that the timestamp from
   * sequential calls returns timestamps that are more than one frame
   * apart.
   * @param[out] datum Pointer to track vector
   * @param[out] frame Frame number on which tracks were terminated.
   * @return False is returned if there are no more objects in the
   * file. The values in \c datum and \c frame are undefined if false is
   * returned.  True is returned if a set of terminated tracks is
   * returned from the file.
   */
  virtual bool read_next_terminated( vidtk::track::vector_t& datum, unsigned& frame );

  /** Read all tracks from the file. All tracks from the file are
   * returned in the supplied vector.  Note that zero tracks may be
   * returned. Subsequent calls will always return zero tracks, since
   * all tracks have already been returned.
   * @param[out] datum Vector of tracks from file.
   * @return The number of objects read is returned.
   */
  virtual size_t read_all( vidtk::track::vector_t& datum );


/*
  void set_frame_indexing( int zero_or_one )
  {
    frame_indexing = zero_or_one;
  }
*/

private:
  void read_xml( TiXmlNode*                       pParent,
                 std::vector< vidtk::track_sptr >& trks,
                 std::vector< occluder >*          occluders );
  void read_occulder_xml( TiXmlNode*              pParent,
                          std::vector< occluder >& occluders );

  bool been_read_;
  double frequency_of_frames;
  unsigned frame_indexing;

  // Temporary variables used to keep track of previous values
  // while reading the xml file
  std::string prev_element;

  double global_min_x;
  double global_min_y;
  double global_max_x;
  double global_max_y;

  int global_frame_index;

};

} // end namespace
} // end namespace

#endif // mit_layer_annotation_reader_h_
