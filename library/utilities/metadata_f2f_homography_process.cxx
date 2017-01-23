/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_f2f_homography_process.h"
#include <utilities/geo_coordinate.h>
#include <utilities/compute_transformations.h>
#include <logger/logger.h>

namespace vidtk
{

using namespace geo_coord;

VIDTK_LOGGER("metadata_f2f_homography_process_cxx");

struct metadata_f2f_homography_process::impl
{
  bool use_lat_lon_;
  bool first_;
  unsigned int w_, h_;
  video_metadata meta_cur_;
  vgl_h_matrix_2d<double> prev_f_world_translate_;
  vgl_h_matrix_2d<double> prev_f_2_wld_;
  vgl_h_matrix_2d<double> prev_f_2_cur_f_;
};


metadata_f2f_homography_process
::metadata_f2f_homography_process(const std::string &n)
: process(n, "metadata_f2f_homography_process"),
  disabled_(false), p_(new impl)
{
  this->config_.add_parameter("disabled", "true", "If disabled, identity matrices are output");
  this->config_.add_parameter("use_latlon_for_xfm", "false", "Use lat/lon coordinates in affine transformations.  Default is to use UTM");
}


metadata_f2f_homography_process
::~metadata_f2f_homography_process()
{
  delete this->p_;
}


config_block
metadata_f2f_homography_process
::params() const
{
  return this->config_;
}


bool
metadata_f2f_homography_process
::set_params(const config_block &blk)
{
  try
  {
    this->disabled_ = blk.get<bool>("disabled");
    this->p_->use_lat_lon_ = blk.get<bool>("use_latlon_for_xfm");
  }
  catch(const config_block_parse_error &e)
  {
    LOG_ERROR(this->name() << e.what());
    return false;
  }
  this->config_ = blk;
  return true;
}


bool
metadata_f2f_homography_process
::initialize()
{
  this->p_->first_ = true;
  this->p_->prev_f_2_wld_.set_identity();
  this->p_->prev_f_world_translate_.set_identity();
  this->p_->prev_f_2_cur_f_.set_identity();
  return true;
}

inline vgl_homg_point_2d<double> geo2homg(const geo_coord::geo_lat_lon &g)
{
  return vgl_homg_point_2d<double>(g.get_longitude(), g.get_latitude());
}
inline vgl_homg_point_2d<double> geo2homg(const geo_coord::geo_UTM &g)
{
  return vgl_homg_point_2d<double>(g.get_easting(), g.get_northing());
}

process::step_status
metadata_f2f_homography_process
::step2()
{
  if(this->disabled_)
  {
    this->p_->prev_f_2_cur_f_.set_identity();
    return process::SUCCESS;
  }
  if(!this->p_->meta_cur_.has_valid_corners())
  {
    LOG_WARN(this->name() << ": Valid corner missing, time: "
      << p_->meta_cur_.timeUTC() << ", producing identity transform." );
    this->p_->prev_f_2_cur_f_.set_identity();
    return process::SUCCESS;
  }

  std::vector<vgl_homg_point_2d<double> > src_img;
  src_img.push_back(vgl_homg_point_2d<double>(             0,              0));
  src_img.push_back(vgl_homg_point_2d<double>(this->p_->w_-1,              0));
  src_img.push_back(vgl_homg_point_2d<double>(this->p_->w_-1, this->p_->h_-1));
  src_img.push_back(vgl_homg_point_2d<double>(             0, this->p_->h_-1));

  std::vector<vgl_homg_point_2d<double> > dst_wld;
  dst_wld.reserve(4);
  vgl_h_matrix_2d<double> world_translate;
  world_translate.set_identity();
  if(this->p_->use_lat_lon_)
  {
    dst_wld.push_back( geo2homg(this->p_->meta_cur_.corner_ul()) );
    dst_wld.push_back( geo2homg(this->p_->meta_cur_.corner_ur()) );
    dst_wld.push_back( geo2homg(this->p_->meta_cur_.corner_lr()) );
    dst_wld.push_back( geo2homg(this->p_->meta_cur_.corner_ll()) );
  }
  else
  {
    geo_coordinate w_ul = geo_coordinate( this->p_->meta_cur_.corner_ul() );
    geo_coordinate w_ur = geo_coordinate( this->p_->meta_cur_.corner_ur() );
    geo_coordinate w_lr = geo_coordinate( this->p_->meta_cur_.corner_lr() );
    geo_coordinate w_ll = geo_coordinate( this->p_->meta_cur_.corner_ll() );

    dst_wld.push_back( geo2homg( w_ul.get_utm()) );
    dst_wld.push_back( geo2homg( w_ur.get_utm()) );
    dst_wld.push_back( geo2homg( w_lr.get_utm()) );
    dst_wld.push_back( geo2homg( w_ll.get_utm()) );

    //UTM are very large number and can have a negative effect
    //on numerical stability, so we will translate them into
    //a more reasonable meter space
    double mean_pt[2] = {0.0, 0.0};
    for( size_t i = 0; i < 4; i++ )
    {
      mean_pt[0] += dst_wld[i].x();
      mean_pt[1] += dst_wld[i].y();
    }
    mean_pt[0] /= 4;
    mean_pt[1] /= 4;
    world_translate.set_translation( -mean_pt[0], -mean_pt[1] );
    for( size_t i = 0; i < 4; i++ )
    {
      dst_wld[i] = world_translate*dst_wld[i];
    }
  }

  vgl_h_matrix_2d<double> cur_f2w;
  if(!compute_homography(src_img, dst_wld, cur_f2w))
  {
    LOG_ERROR(this->name() << ": Could not compute transformation." );
    return process::FAILURE;
  }

  LOG_INFO(this->name() << ": F2W\n" << cur_f2w);

  if(!this->p_->first_)
  {
    vgl_h_matrix_2d<double> cur_w2f = cur_f2w.get_inverse();
    vgl_h_matrix_2d<double> cur_wld_inv = world_translate;
    //Because cur_wld_inv and prev_f_world_translate_ are working with large values
    //we do not really care about, we will multiply them first to help with numerical
    //stability.
    this->p_->prev_f_2_cur_f_ =  cur_w2f * (cur_wld_inv * this->p_->prev_f_world_translate_) * this->p_->prev_f_2_wld_;
  }
  this->p_->prev_f_2_wld_ = cur_f2w;
  this->p_->first_ = false;
  this->p_->prev_f_world_translate_ =  world_translate.get_inverse();
  LOG_INFO(this->name() << ": F2F\n" << this->p_->prev_f_2_cur_f_);
  return process::SUCCESS;
}


void
metadata_f2f_homography_process
::set_image(const vil_image_view_base &img)
{
  this->p_->w_ = img.ni();
  this->p_->h_ = img.nj();
}


void
metadata_f2f_homography_process
::set_metadata(const video_metadata &m)
{
  this->p_->meta_cur_ = m;
}


void
metadata_f2f_homography_process
::set_metadata_v(const std::vector<video_metadata> &m)
{
  this->p_->meta_cur_ = m[0];
}


vgl_h_matrix_2d<double>
metadata_f2f_homography_process
::h_prev_2_cur(void) const
{
  return this->p_->prev_f_2_cur_f_;
}

}  // namespace vidtk
