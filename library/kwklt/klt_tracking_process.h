/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwklt_tracking_process_h_
#define vidtk_kwklt_tracking_process_h_

#include "klt_track.h"
#include "klt_tracking_process_impl.h"

#include <klt/klt.h>

#include <vil/vil_pyramid_image_view.h>

#include <process_framework/pipeline_aid.h>
#include <process_framework/process.h>
#include <utilities/config_block.h>

#include <boost/optional.hpp>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

class timestamp;

class klt_tracking_process
  : public process
{
public:
  typedef klt_tracking_process self_type;

  klt_tracking_process(std::string const& name);

  virtual ~klt_tracking_process();

  virtual config_block params() const;

  virtual bool set_params(config_block const&);

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();


  // for the moment, we only allow a 8-bit image. If we want to handle
  // 16-bit image, we would need to extend the underlying KLT to do
  // that.  Then, we can handle 16-bit images by overloading set_image
  // with a 16-bit image version, instead of making the whole class
  // templated.

  /// The image pyramid of the input image.
  void set_image_pyramid(vil_pyramid_image_view<float> const& img);
  VIDTK_INPUT_PORT(set_image_pyramid, vil_pyramid_image_view<float> const&);

  /// The image pyramid of the x gradient.
  void set_image_pyramid_gradx(vil_pyramid_image_view<float> const& img);
  VIDTK_INPUT_PORT(set_image_pyramid_gradx, vil_pyramid_image_view<float> const&);

  /// The image pyramid of the y gradient.
  void set_image_pyramid_grady(vil_pyramid_image_view<float> const& img);
  VIDTK_INPUT_PORT(set_image_pyramid_grady, vil_pyramid_image_view<float> const&);

  /// The timestamp for the current frame.
  void set_timestamp(vidtk::timestamp const& ts);
  VIDTK_INPUT_PORT(set_timestamp, vidtk::timestamp const&);

  /// The homography prediciton for where to search in the new frame
  void set_homog_predict(vgl_h_matrix_2d<double> const &h);
  VIDTK_OPTIONAL_INPUT_PORT(set_homog_predict, vgl_h_matrix_2d<double> const&);

  /// Set of tracks that are actively being tracked.
  std::vector<klt_track_ptr> active_tracks() const;
  VIDTK_OUTPUT_PORT(std::vector<klt_track_ptr>, active_tracks);

  /// Set of tracks that were created this round.
  std::vector<klt_track_ptr> terminated_tracks() const;
  VIDTK_OUTPUT_PORT(std::vector<klt_track_ptr>, terminated_tracks);

  /// Set of new tracks that were created at the last step.
  std::vector<klt_track_ptr> created_tracks() const;
  VIDTK_OUTPUT_PORT(std::vector<klt_track_ptr>, created_tracks);

protected:
  boost::scoped_ptr<klt_tracking_process_impl> impl_;
  bool has_first_frame_;
};


} // end namespace vidtk


#endif // vidtk_kwklt_tracking_process_h_
