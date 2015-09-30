/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_generic_frame_process
#define vidtk_generic_frame_process

#include <utilities/video_metadata.h>
#include <video/frame_process.h>
#include <vil/vil_image_view.h>
#include <vcl_memory.h>
#include <vcl_vector.h>
#include <vcl_utility.h>

#include <process_framework/pipeline_aid.h>

namespace vidtk
{


template<class PixType>
class generic_frame_process
  : public frame_process<PixType>
{
public:
  typedef generic_frame_process self_type;

  generic_frame_process( vcl_string const& );

  ~generic_frame_process();

  config_block params() const;

  bool set_params( config_block const& );

  bool is_initialized() const { return (impl_ != NULL); }

  bool initialize();

  bool reinitialize();

  bool step();

  bool seek( unsigned frame_number );

  vidtk::timestamp timestamp() const;

  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  vil_image_view<PixType> const& image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, image );

  vil_image_view<PixType> const& copied_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, copied_image );

  vidtk::video_metadata const& metadata() const;

  VIDTK_OUTPUT_PORT( vidtk::video_metadata, metadata );

  unsigned ni() const;
  unsigned nj() const;

  bool set_roi(vcl_string roi);
  bool set_roi( unsigned int x, unsigned int y,
                unsigned int w, unsigned int h );
  void reset_roi();

private:
  void populate_possible_implementations();

  typedef typename vcl_vector< vcl_pair< vcl_string, frame_process<PixType>* > > impl_list_type;
  typedef typename impl_list_type::iterator impl_iter_type;
  typedef typename impl_list_type::const_iterator impl_const_iter_type;

  // Possible concrete frame sources.
  impl_list_type impls_;

  // The selected implementation.
  frame_process<PixType>* impl_;
  vcl_string impl_name_;

  mutable vil_image_view<PixType> copy_img_;

  config_block conf_;

  bool read_only_first_frame_;
  unsigned frame_number_;

  // If the process is retained across two calls on intialize(), then
  // we only want to "initialize" only the first time.
  bool initialized_;
};

template<>
void generic_frame_process<vxl_byte>::populate_possible_implementations();
template<>
void generic_frame_process<vxl_uint_16>::populate_possible_implementations();


} // end namespace vidtk


#endif // vidtk_generic_frame_process
