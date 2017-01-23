/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "fg_matcher.h"

#include <vgl/vgl_intersection.h>
#include <vgl/vgl_box_2d.h>

bool
fg_search_region_box
::update( vnl_double_2 const& cur, vnl_double_2 const& pred, vgl_box_2d<unsigned> const& wh_bbox,
          bool is_bottom, bool both, unsigned int img_ni, unsigned int img_nj )
{

  //predicted position lies outside image space
  //NOTE: this might be a problem with search both locations.  The predicted could be outside
  //      the image, but the previous is still inside.  It is copied from da_tracker_process.
  if(  pred[0] < 0 || pred[1] < 0
    || pred[0] >= img_ni || pred[1] >= img_nj )
  {
    return false;
  }

  current_loc_img_ = cur;
  predict_loc_img_ = pred;
  centroid_loc_img_ = (cur + pred)*0.5;

  vgl_box_2d<double> query(wh_bbox.min_x(), wh_bbox.max_x(), wh_bbox.min_y(), wh_bbox.max_y());
  double offx = 0, offy = (is_bottom)?wh_bbox.height()*-0.5:0;
  double tmp_cent[] = { predict_loc_img_[0] + offx, predict_loc_img_[1] + offy };
  query.set_centroid(tmp_cent);
  if(both)
  {
    vgl_box_2d<double> tmp(wh_bbox.min_x(), wh_bbox.max_x(), wh_bbox.min_y(), wh_bbox.max_y());
    tmp_cent[0] = current_loc_img_[0] + offx;
    tmp_cent[1] = current_loc_img_[1] + offy;
    tmp.set_centroid(tmp_cent);
    query.add( tmp );
  }
  vgl_box_2d< double >  inter = vgl_intersection(vgl_box_2d<double>(0, img_ni, 0, img_nj), query );

  //box is smaller than is reasonable for searching
  if( inter.is_empty() ||
      inter.width() < wh_bbox.width() ||
      inter.height() < wh_bbox.height() )
  {
    return false;
  }

  bbox_ = vgl_box_2d<unsigned>( static_cast<unsigned>(inter.min_x()),
                                static_cast<unsigned>(inter.max_x()),
                                static_cast<unsigned>(inter.min_y()),
                                static_cast<unsigned>(inter.max_y()) );
  return true;
}
