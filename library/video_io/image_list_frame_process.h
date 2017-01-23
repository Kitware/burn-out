/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_list_frame_process_h_
#define vidtk_image_list_frame_process_h_

#include <video_io/frame_process.h>

#include <string>
#include <vector>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Read images from list of names or directory.
 *
 * This process implements the frame process interface and reads
 * frames from a list of files. This list is either supplied in a file
 * or derived from the contents of a directory.
 *
 * The list of frames can be optionally looped.
 *
 * Frame numbers can be either derived from the file name string or
 * assigned monotonically. Frame time can be optionally decoded from
 * the file name string also. If frame time is not specified, it is
 * left as undefined in the generated timestamp.
 *
 * No metadata is produced by this process.
 */
template<class PixType>
class image_list_frame_process
  : public frame_process<PixType>
{
public:
  image_list_frame_process( std::string const& name );
  virtual ~image_list_frame_process() { }

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();

  virtual bool step();

  virtual bool seek( unsigned frame_number );

  virtual unsigned nframes() const;
  virtual double frame_rate() const;

  virtual vidtk::timestamp timestamp() const;
  virtual vil_image_view<PixType> image() const;

  virtual unsigned ni() const;
  virtual unsigned nj() const;

private:

  std::string file_;
  std::string parse_frame_number_;
  std::string parse_timestamp_;
  std::string ignore_substring_;

  std::vector< std::string > filenames_;
  std::vector< vidtk::timestamp > timestamps_;

  unsigned cur_idx_;
  unsigned base_frame_number_;
  vil_image_view< PixType > img_;
  unsigned ni_;
  unsigned nj_;
  bool loop_forever_;
  unsigned loop_count_;

  // Cache if someone uses the ni() or nj() functions, so we can
  // verify that the frame size doesn't change.
  mutable bool frame_size_accessed_;

  bool load_image_roi( std::string& fname );
  bool load_image( std::string& fname );

  std::string roi_string_;
};


} // end namespace vidtk


#endif // vidtk_image_list_frame_process_h_
