/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _TRACK_READER_VIPER_H_
#define _TRACK_READER_VIPER_H_


#include <tracking_data/io/track_reader_interface.h>


class TiXmlNode;


namespace vidtk {
namespace ns_track_reader {

//enum that indicates the parent attribute of the element if applicable
enum XML_STATE
{
  NONE,
  LOCATION,
  OCCLUSION,
  DESCRIPTOR
};


class track_reader_viper
  : public track_reader_interface
{
public:
  track_reader_viper();
  virtual ~track_reader_viper();


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

  std::vector<std::string> descriptor_object_names() const
  {
    return descriptor_object_names_;
  }


private:
  std::vector<std::string> descriptor_object_names_;

  void read_xml( TiXmlNode* pParent,
                 vidtk::track::vector_t& trks,
                 XML_STATE xml_state = NONE);

  //maintains current track's state and allows them to be mutated before adding to the track
  std::vector<track_state_sptr> temp_states;
  std::vector<std::pair<unsigned,unsigned> > occluded_frame_ranges;

  bool been_read_;


};

} // end namespace
} // end namespace
#endif  //viper_reader_h_
