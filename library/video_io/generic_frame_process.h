/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_generic_frame_process_h_
#define vidtk_generic_frame_process_h_

#include <utilities/video_metadata.h>
#include <video_io/frame_process.h>
#include <vil/vil_image_view.h>
#include <memory>
#include <vector>
#include <utility>

#include <process_framework/pipeline_aid.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Video source process.
 *
 * This process sources images, timestamps and video metadata. The way
 * these are sources is configurable. The full featured attribute of
 * this process allows just about any type of video to be sources
 * using the config file rather than hand coding the video source.
 *
 * This process provides a collection of different ways of generating
 * tracker input data. The actual source is selected via the config
 * file and used to source the video.
 *
 * The available video sources depends on what pixel type is used to
 * instantiate this class.
 *
 * Currently the configuration is rather complex, so I am recommending
 * looking at the code until better documentation is available.
 *
 * \tparam PixType - Image pixel type.
 */
template<class PixType>
class generic_frame_process
  : public frame_process<PixType>
{
public:
  typedef generic_frame_process self_type;

  generic_frame_process( std::string const& );
  virtual ~generic_frame_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();

  bool is_initialized() const { return (impl_ != NULL); }
  bool reinitialize();

  virtual process::step_status step2();

  virtual bool seek( unsigned frame_number );

  virtual unsigned nframes() const;
  virtual double frame_rate() const;

  virtual vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  virtual vil_image_view<PixType> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

  vil_image_view<PixType> copied_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, copied_image );

  virtual vidtk::video_metadata metadata() const;
  VIDTK_OUTPUT_PORT( vidtk::video_metadata, metadata );

  virtual unsigned ni() const;
  virtual unsigned nj() const;

  virtual bool set_roi(std::string & roi);
  virtual bool set_roi( unsigned int x, unsigned int y,
                unsigned int w, unsigned int h );
  void reset_roi();

private:
  void populate_possible_implementations();

  typedef typename std::vector< std::pair< std::string, frame_process<PixType>* > > impl_list_type;
  typedef typename impl_list_type::iterator impl_iter_type;
  typedef typename impl_list_type::const_iterator impl_const_iter_type;

  // Possible concrete frame sources.
  impl_list_type impls_;

  // The selected implementation.
  frame_process<PixType>* impl_;
  std::string impl_name_;

  mutable vil_image_view<PixType> copy_img_;

  config_block conf_;

  bool read_only_first_frame_;
  unsigned frame_number_;

  // If the process is retained across two calls on intialize(), then
  // we only want to "initialize" only the first time.
  bool initialized_;
  bool disabled_;
};

// Declare the specializations that are defined in the .cxx file
#define DECL( T )                              \
  template<> void generic_frame_process<T>  \
    ::populate_possible_implementations()
DECL( bool );
DECL( vxl_byte );
DECL( vxl_uint_16 );
#undef DECL

} // end namespace vidtk


#endif // vidtk_generic_frame_process_h_
