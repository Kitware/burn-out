/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_METADATA_F2F_HOMOGRAPHY_PROCESS_H_
#define _VIDTK_METADATA_F2F_HOMOGRAPHY_PROCESS_H_

#include <string>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/video_metadata.h>

#include <vil/vil_image_view_base.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

/// Estimate a frame to frame homography from available metadata
class metadata_f2f_homography_process : public process
{
public:
  typedef metadata_f2f_homography_process self_type;

  metadata_f2f_homography_process(const std::string &name);

  ~metadata_f2f_homography_process();

  virtual config_block params() const;

  virtual bool set_params(const config_block &blk);

  virtual bool initialize();

  virtual process::step_status step2();

  virtual bool step()
  {
    return this->step2() == process::SUCCESS;
  }

  /// The input image is only used to get dimensiona
  void set_image(const vil_image_view_base &img);
  VIDTK_INPUT_PORT(set_image, const vil_image_view_base&);

  void set_metadata(const video_metadata &m);
  VIDTK_INPUT_PORT(set_metadata, const video_metadata&);

  /// Some processes use a vector for metadata.  This this case, we just
  /// take the first one
  void set_metadata_v(const std::vector<video_metadata> &m);
  VIDTK_INPUT_PORT(set_metadata_v, const std::vector<video_metadata>&);

  /// Homography estimation from previous frame to current frame
  vgl_h_matrix_2d<double> h_prev_2_cur(void) const;
  VIDTK_OUTPUT_PORT(vgl_h_matrix_2d<double>, h_prev_2_cur);

private:
  config_block config_;
  bool disabled_;

  struct impl;
  impl *p_;
};

}  // namespace vidtk

#endif
