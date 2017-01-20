/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_list_frame_metadata_process_h_
#define vidtk_image_list_frame_metadata_process_h_

#include <video_io/frame_process.h>
#include <utilities/video_metadata.h>
#include <string>
#include <vector>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Read frame from supplied file name.
 *
 * This process reads an image from the file name supplied on the
 * set_input_image_file port. The frame number is encoded in the input
 * file name string. Frame level metadata is read from a separate file
 * name constructed from the frame number extracted from the image
 * file. This per-frame metadata file contains the GEO location
 * metadata and possibly a better frame number, depending on the
 * metadata format specified in the config.
 *
 * The image can be cropped using a configuration specification.

 */
template<class PixType>
class frame_metadata_decoder_process
  : public frame_process<PixType>
{
public:
  typedef vidtk::frame_metadata_decoder_process<PixType> self_type;

  frame_metadata_decoder_process( std::string const& name );
  virtual ~frame_metadata_decoder_process() { }

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // This only reads frames that we are given over input ports. This cannot go
  // back in time.
  virtual bool seek( unsigned /*frame_number*/ ) { return false; };

  // Nor does it know how many frames there are.
  virtual unsigned nframes() const { return 0; }

  // Or the frame rate.
  virtual double frame_rate() const { return 0.0; }

  /// \brief Input image file.
  ///
  /// Non-absolute paths that are passed through will be interpreted as relative
  /// to the current working directory.
  void set_input_image_file( std::string const& img_file_path );
  VIDTK_INPUT_PORT( set_input_image_file, std::string const& );

  virtual vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  virtual vil_image_view<PixType> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

  virtual vidtk::video_metadata metadata() const;
  VIDTK_OUTPUT_PORT( vidtk::video_metadata, metadata );

  virtual unsigned ni() const;
  virtual unsigned nj() const;

private:
  config_block config_;

  enum metadata_format { KLV, RED_RIVER };
  metadata_format metadata_format_;
  std::string img_fn_re_string_;
  std::string meta_location_;
  std::string meta_file_format_string_;
  double red_river_hfov_;
  double red_river_vfov_;

  std::string image_filename_;
  std::string meta_filename_;
  vidtk::timestamp timestamp_;
  vidtk::video_metadata metadata_;

  bool first_frame_;
  vil_image_view<PixType> img_;
  unsigned ni_;
  unsigned nj_;
  // Cache if someone uses the ni() or nj() functions, so we can
  // verify that the frame size doesn't change.
  mutable bool frame_size_accessed_;

  bool generic_load_image( std::string & fname );

  std::string roi_string_;

};


} // end namespace vidtk

#endif // vidtk_image_list_frame_metadata_process_h_
