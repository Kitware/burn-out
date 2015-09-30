/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_list_frame_metadata_process
#define vidtk_image_list_frame_metadata_process

#include <video/frame_process.h>
#include <utilities/video_metadata.h>
#include <vcl_string.h>
#include <vcl_vector.h>

namespace vidtk
{

template<class PixType>
class image_list_frame_metadata_process
  : public frame_process<PixType>
{
public:
  image_list_frame_metadata_process( vcl_string const& name );

  config_block params() const;

  bool set_params( config_block const& );

  bool initialize();

  bool step();

  bool seek( unsigned frame_number );

  vidtk::timestamp timestamp() const;

  vil_image_view<PixType> const& image() const;
  vidtk::video_metadata const& metadata() const;

  unsigned ni() const;
  unsigned nj() const;

private:
  vcl_string file_;

  vcl_string parse_frame_number_;
  vcl_string parse_timestamp_;

  vcl_vector<vcl_string> image_filenames_;
  vcl_vector<vcl_string> meta_filenames_;
  vcl_vector<vidtk::timestamp> timestamps_;
  vcl_vector<vidtk::video_metadata> metadata_;
  unsigned cur_idx_;
  vil_image_view<PixType> img_;
  unsigned ni_;
  unsigned nj_;
  bool loop_forever_;
  bool loop_count_;
  // Cache if someone uses the ni() or nj() functions, so we can
  // verify that the frame size doesn't change.
  mutable bool frame_size_accessed_;

  bool load_image_roi(vcl_string & fname);
  bool load_image(vcl_string & fname);

  vcl_string roi_string_;

};


} // end namespace vidtk


#endif // vidtk_image_list_frame_metadata_process
