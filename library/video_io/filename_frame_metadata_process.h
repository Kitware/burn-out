/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_filename_frame_metadata_process
#define vidtk_filename_frame_metadata_process

#include <string>
#include <vil/vil_smart_ptr.h>
#include <vil/vil_stream.h>
#include <vil/vil_image_resource.h>
#include <vgl/vgl_box_2d.h>

#include <process_framework/pipeline_aid.h>
#include <video_io/frame_process.h>
#include <utilities/video_metadata.h>

namespace vidtk
{
typedef vil_smart_ptr<vil_stream> vil_stream_sptr;
struct filename_frame_metadata_process_impl;

// Given a metadata file, decode its associated cropped roi and metadata
// ----------------------------------------------------------------
/*! \brief Read image from supplied file name.
 *
 * This process reads an image from the file name suplied to the
 * set_filename port. The frame number is embedded in the file
 * name. The image may contain other metadata such as GEO location if
 * the image is in NTIF format.
 *
 * The pixel ROI (crop area) may be supplied on the set_pixel_roi
 * port. If supplied, it is in the form <width>x<height>+<x-origin-offset>+<y-origin-offset>
 * or as a regular expression: "^([0-9]+)x([0-9]+)([+-][0-9]+)([+-][0-9]+)$".
 *
 * Images may be cached on disk or in memory. This is useful in cases
 * where the images are large and are cropped.
 *
 * Use this process in cases where the image file name may be
 * available before all the image data. This process will retry
 * reading and skip bad images (up to a point).
 */
template<class PixType>
class filename_frame_metadata_process
: public frame_process<PixType>
{
public:
  typedef filename_frame_metadata_process<PixType> self_type;

  filename_frame_metadata_process( std::string const& name );
  virtual ~filename_frame_metadata_process(void);

  virtual config_block params() const { return this->config_; }
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  process::step_status step2();

  bool step()
  {
    return step2() == process::SUCCESS;
  }

  // Only designed to read a frame on demand
  bool seek( unsigned /*frame_number*/ ) { return false; }
  unsigned nframes() const { return 0; }
  double frame_rate() const { return 0.0; }

  void set_filename(std::string const& f) { this->filename_ = f; }
  VIDTK_INPUT_PORT(set_filename, std::string const&);

  void set_pixel_roi(const std::string &roi) { this->pixel_roi_ = roi; }
  VIDTK_OPTIONAL_INPUT_PORT(set_pixel_roi, const std::string&);

  // Strict accessors
  virtual vidtk::timestamp timestamp() const { return this->timestamp_; }
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp);

  virtual vil_image_view<PixType> image() const { return this->img_; }
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image);

  virtual video_metadata metadata() const { return this->metadata_; }
  VIDTK_OUTPUT_PORT( video_metadata, metadata);

  // Since ni and nj can change and are not known a priori, they must be 0
  unsigned ni() const { return 0; }
  unsigned nj() const { return 0; }


private:
  config_block config_;

  std::string filename_;
  std::string pixel_roi_;

  video_metadata metadata_;
  vidtk::timestamp timestamp_;
  vil_image_view<PixType> img_;

  filename_frame_metadata_process_impl *impl_;

  bool parse_framenum(void);
  bool parse_pixel_roi(void);

  vil_stream_sptr cache_file(void);
  vil_stream_sptr cache_file_0_NONE(void);
  vil_stream_sptr cache_file_1_RAM(void);
  vil_stream_sptr cache_file_2_DISK(void);
  static void copy_stream(vil_stream_sptr s_in, vil_stream_sptr s_out,
                          vil_streampos buf_size, vil_streampos total_size);

  bool parse_metadata(vil_image_resource_sptr img);
  bool parse_metadata_0_NONE(vil_image_resource_sptr img);
  bool parse_metadata_1_NITF(vil_image_resource_sptr img);

  process::step_status check_skip(bool condition, const std::string &message);

  void build_transforms(void);
  void build_geo_roi(void);
  void compute_pixel_roi(void);
  bool decode_roi(vil_image_resource_sptr img);
  bool parse_roi_geo(const std::string &s);
};

}

#endif
