/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_list_frame_metadata_process_h_
#define vidtk_image_list_frame_metadata_process_h_

#include <video_io/frame_process.h>
#include <utilities/video_metadata.h>
#include <string>
#include <vector>

namespace vidtk
{

template<class PixType>
class image_list_frame_metadata_process
  : public frame_process<PixType>
{
public:
  image_list_frame_metadata_process( std::string const& name );
  virtual ~image_list_frame_metadata_process() { }

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  virtual bool seek( unsigned frame_number );
  virtual unsigned nframes() const;
  virtual double frame_rate() const;

  // output methods
  virtual vidtk::timestamp timestamp() const;
  virtual vil_image_view<PixType> image() const;
  virtual vidtk::video_metadata metadata() const;

  virtual unsigned ni() const;
  virtual unsigned nj() const;

private:
  std::string file_;

  std::string parse_frame_number_;
  std::string parse_timestamp_;
  enum metadata_format { KLV, RED_RIVER };
  metadata_format metadata_format_;
  double red_river_hfov_;
  double red_river_vfov_;

  std::vector<std::string> image_filenames_;
  std::vector<std::string> meta_filenames_;
  std::vector<vidtk::timestamp> timestamps_;
  std::vector<vidtk::video_metadata> metadata_;
  unsigned cur_idx_;
  vil_image_view<PixType> img_;
  unsigned ni_;
  unsigned nj_;
  bool loop_forever_;
  unsigned loop_count_;
  // Cache if someone uses the ni() or nj() functions, so we can
  // verify that the frame size doesn't change.
  mutable bool frame_size_accessed_;

  bool load_image_roi(std::string & fname);
  bool load_image(std::string & fname);

  std::string roi_string_;
};


} // end namespace vidtk


#endif // vidtk_image_list_frame_metadata_process_h_
