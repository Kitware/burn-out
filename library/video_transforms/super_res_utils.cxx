/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/super_res_utils.h>
#include <video_transforms/adjoint_dbw.h>

#include <limits>
#include <vnl/vnl_math.h>
#include <vil/vil_crop.h>

namespace vidtk
{

void homogs_to_flows(const std::vector<vgl_h_matrix_2d<double> > &homogs,
                     int ref_frame,
                     int i0, int j0, int ni, int nj,
                     int scale_factor, std::vector<vil_image_view<double> > &flows)
{
  std::string flowname;

  const vgl_h_matrix_2d<double> &ref_H = homogs[ref_frame];

  i0 *= scale_factor;  j0 *= scale_factor;
  ni *= scale_factor;  nj *= scale_factor;

  vnl_double_3x3 S, Sinv;
  S.set_identity();
  S(0,0) = scale_factor;
  S(1,1) = scale_factor;
  Sinv.set_identity();
  Sinv(0,0) = 1.0/scale_factor;
  Sinv(1,1) = 1.0/scale_factor;

  const vnl_double_3x3 M =  ref_H.get_matrix() * Sinv;
  for (unsigned int f = 0; f < homogs.size(); f++)
  {
    vgl_h_matrix_2d<double> ref_to_f = S * homogs[f].get_inverse().get_matrix() * M;

    vil_image_view<double> flow;
    flow.set_size(ni, nj, 2);
    flow.fill(std::numeric_limits<double>::quiet_NaN());
    for (int i = 0; i < ni; i++)
    {
      for (int j = 0; j < nj; j++)
      {
        vgl_point_2d<double> p = ref_to_f * vgl_homg_point_2d<double>(i+i0, j+j0);
        flow(i, j, 0) = p.x() - i;
        flow(i, j, 1) = p.y() - j;
      }
    }

    flows.push_back(flow);
  }

  std::cout << "Converted " << flows.size() << " Homogs to flows.\n";
}

vgl_box_2d<double>
flow_destination_bounds(const vil_image_view<double> &flow)
{
  const unsigned int ni=flow.ni();
  const unsigned int nj=flow.nj();

  vgl_box_2d<double> bbox;
  for( unsigned j=0; j<nj; ++j)
  {
    for( unsigned i=0; i<ni; ++i)
    {
      const double& di = flow(i,j,0);
      const double& dj = flow(i,j,1);
      if( vnl_math::isfinite(di) && vnl_math::isfinite(dj) )
      {
        bbox.add(vgl_point_2d<double>(i + di, j + dj));
      }
    }
  }
  return bbox;
}

void translate_flow(vil_image_view<double> &flow, double dx, double dy)
{
  const unsigned int ni=flow.ni();
  const unsigned int nj=flow.nj();

  for( unsigned j=0; j<nj; ++j)
  {
    for( unsigned i=0; i<ni; ++i)
    {
      flow(i,j,0) += dx;
      flow(i,j,1) += dy;
    }
  }
}

/// Use the valid region of the flow destinations to crop the frames and translate the flow.
void crop_frames_and_flows(std::vector<vil_image_view<double> > &flows,
                           std::vector<vil_image_view<double> > &frames,
                           int scale_factor, int margin)
{
  assert(flows.size() == frames.size());
  std::vector<vgl_box_2d<int> > boxes;
  for (unsigned int i=0; i<frames.size(); ++i)
  {
    boxes.push_back(vgl_box_2d<int>(0, frames[i].ni()-1, 0, frames[i].nj()-1));
  }
  crop_boxes_and_flows(flows, boxes, scale_factor, margin);
  // crop the frames
  for (unsigned int i=0; i<flows.size(); ++i)
  {
    frames[i].deep_copy(vil_crop(frames[i],
                                 boxes[i].min_x(), boxes[i].width(),
                                 boxes[i].min_y(), boxes[i].height()));
  }
}


/// Crops the image boxes and flows to what is needed from the crop region
/// This function is the same as \a crop_frames_and_flows except that instead
/// of croping the frames it produces the cropped bounding boxes that can
/// be used to crop the frames later.
/// \param flows the optical flow for each frame
/// \param boxes the bounding boxes corresponding to image sizes
/// \param scale_factor the amount of scaling used in the flows
/// \param margin an expansion tolerance on the cropping region
void crop_boxes_and_flows(std::vector<vil_image_view<double> > &flows,
                          std::vector<vgl_box_2d<int> > &boxes,
                          const int scale_factor,
                          const int margin)
{
  assert(flows.size() == boxes.size());
  for (unsigned int i=0; i<flows.size(); ++i)
  {
    // get a bounding box around the flow and expand by a margin
    vgl_box_2d<double> bounds = flow_destination_bounds(flows[i]);
    vgl_box_2d<int> bbox(std::floor(bounds.min_x()), std::ceil(bounds.max_x()),
                         std::floor(bounds.min_y()), std::ceil(bounds.max_y()));
    bbox.expand_about_centroid(2*margin);

    // get a bounding box around the (up sampled) image and intersect
    vgl_box_2d<int> img_box = boxes[i];
    img_box.scale_about_origin(scale_factor);
    bbox = vgl_intersection(bbox, img_box);
    bbox.scale_about_origin(1.0 / scale_factor);

    // this is the box that can be used to crop the source frame
    boxes[i] = bbox;

    // scale the box back to the scaled space and translate the flow accordingly
    bbox.scale_about_origin(scale_factor);
    translate_flow(flows[i], -bbox.min_x(), -bbox.min_y());
  }
}


void create_warps_from_flows(const std::vector<vil_image_view<double> > &flows,
                             const std::vector<vil_image_view<double> > &frames,
                             std::vector<adjoint_image_ops_func<double> > &warps,
                             const int scale_factor,
                             const double sensor_sigma,
                             const bool down_scaling,
                             const bool bicubic_warping)
{
  assert(flows.size() == frames.size());

  warps.clear();
  warps.reserve(flows.size());
  for (unsigned int i = 0; i < flows.size(); ++i)
  {
    warps.push_back(create_dbw_from_flow(flows[i],
                                         frames[i].ni(), frames[i].nj(),
                                         frames[i].nplanes(),
                                         scale_factor,
                                         sensor_sigma,
                                         down_scaling,
                                         bicubic_warping));
  }
}

} // end namespace vidtk
