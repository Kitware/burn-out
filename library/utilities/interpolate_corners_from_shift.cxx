/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "interpolate_corners_from_shift.h"

#include <vgl/vgl_area.h>
#include <vgl/vgl_distance.h>
#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_polygon.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/algo/vgl_h_matrix_2d_optimize_lmq.h>

#include <vnl/vnl_double_3.h>
#include <vnl/vnl_math.h>

#include <rrel/rrel_homography2d_est.h>
#include <rrel/rrel_lms_obj.h>
#include <rrel/rrel_lts_obj.h>
#include <rrel/rrel_irls.h>
#include <rrel/rrel_ransac_obj.h>
#include <rrel/rrel_trunc_quad_obj.h>
#include <rrel/rrel_mlesac_obj.h>
#include <rrel/rrel_muset_obj.h>
#include <rrel/rrel_ran_sam_search.h>

#include <cmath>
#include <algorithm>

#include <boost/circular_buffer.hpp>

#include <logger/logger.h>

VIDTK_LOGGER( "interpolate_corners_from_shift_cxx" );

namespace vidtk
{


class interpolate_corners_from_shift::priv
{
public:

  // Constructors and destructors
  priv() {}
  ~priv() {}

  // Internal members
  interpolate_corners_settings settings_;

  struct point_t
  {
    timestamp ts;
    geo_coord::geo_lat_lon loc;
    image_to_image_homography homog;
  };

  boost::circular_buffer< point_t > ref_points_;
  geo_coord::geo_lat_lon last_loc_;
  bool seen_corners_;
  rrel_ran_sam_search ransam_;

  // Internal functions
  bool update_metadata( const unsigned ni, const unsigned nj,
                        const image_to_image_homography& homog,
                        const timestamp& ts,
                        video_metadata& meta )
  {
    // Reset all reference points on new shots.
    if( homog.is_new_reference() )
    {
      ref_points_.clear();
    }

    if( settings_.remove_other_metadata )
    {
      clear_other_metadata_contents( meta );
    }

    if( seen_corners_ )
    {
      return false;
    }

    if( settings_.never_generate_if_seen && meta.has_valid_corners() )
    {
      seen_corners_ = true;
      return false;
    }

    // Update frame center with new location
    if( !meta.has_valid_frame_center() )
    {
      LOG_DEBUG( "Metadata packet has no frame center" );
      return false;
    }

    point_t current;

    current.ts = ts;
    current.loc = meta.frame_center();
    current.homog = homog;

    if( current.loc == last_loc_ )
    {
      LOG_DEBUG( "Current location matches last location received, ignoring" );
      return false;
    }

    ref_points_.push_back( current );
    last_loc_ = current.loc;

    if( ref_points_.size() < 2 )
    {
      LOG_DEBUG( "Not enough reference points to compute mapping" );
      return false;
    }

    // Attempt to interpolate corners using the shift in center points between
    // frames. Note: currently the math in the interpolation step is not guaranteed
    // to produce the correct orientation for corner points, only the correct scale.
    // In the future, using multiple points in the interpolation step (instead of
    // the first and last) could lead to better results.
    if( settings_.algorithm == interpolate_corners_settings::LAST_2_POINTS )
    {
      const point_t& past = ref_points_[0];

      if( past.homog.get_dest_reference() != current.homog.get_dest_reference() )
      {
        LOG_WARN( "Two homographies in the same shot have different references" );
        return false;
      }

      double center_lat = current.loc.get_latitude();
      double center_lon = current.loc.get_longitude();

      double lat_diff = center_lat - past.loc.get_latitude();
      double lon_diff = center_lon - past.loc.get_longitude();

      double distance = std::sqrt( lat_diff * lat_diff + lon_diff * lon_diff );

      if( lat_diff == 0 || lon_diff == 0 || distance < settings_.min_distance )
      {
        LOG_DEBUG( "Not enough reported movement for mapping" );
        return false;
      }

      int current_i_int = static_cast< int >( ni / 2 );
      int current_j_int = static_cast< int >( nj / 2 );

      vgl_h_matrix_2d< double > cur_to_past =
        past.homog.get_transform().get_inverse() * current.homog.get_transform();

      vnl_double_3 cur_point( current_i_int, current_j_int, 1.0 );
      vnl_double_3 past_point = cur_to_past.get_matrix() * cur_point;

      if( past_point[ 2 ] == 0 )
      {
        LOG_DEBUG( "Mapped points maps to infinity" );
        return false;
      }

      double past_center_i = past_point[ 0 ] / past_point[ 2 ];
      double past_center_j = past_point[ 1 ] / past_point[ 2 ];

      if( current_i_int == static_cast< int >( past_center_i ) ||
          current_j_int == static_cast< int >( past_center_j ) )
      {
        LOG_DEBUG( "Not enough pixel movement for mapping" );
        return false;
      }

      double i_diff = static_cast< double >( current_i_int ) - past_center_i;
      double j_diff = static_cast< double >( current_j_int ) - past_center_j;

      double image_half_width = ni / 2.0;
      double image_half_height = nj / 2.0;

      // A half image translation in the i direction adds this much lat/lon
      double i_lat_intvl = 0.5 * image_half_width * lat_diff / i_diff;
      double i_lon_intvl = 0.5 * image_half_width * lon_diff / i_diff;

      // A half image translation in the j direction adds this much lat/lon
      double j_lat_intvl = 0.5 * image_half_height * lat_diff / j_diff;
      double j_lon_intvl = 0.5 * image_half_height * lon_diff / j_diff;

      // Computer corner points using simple transformation
      geo_coord::geo_lat_lon ul( center_lat - i_lat_intvl - j_lat_intvl,
                                 center_lon - i_lon_intvl - j_lon_intvl );
      geo_coord::geo_lat_lon lr( center_lat + i_lat_intvl + j_lat_intvl,
                                 center_lon + i_lon_intvl + j_lon_intvl );

      geo_coord::geo_lat_lon ur( center_lat + i_lat_intvl + j_lat_intvl,
                                 center_lon - i_lon_intvl - j_lon_intvl );
      geo_coord::geo_lat_lon ll( center_lat - i_lat_intvl - j_lat_intvl,
                                 center_lon + i_lon_intvl + j_lon_intvl );

      // Set points in metadata
      meta.corner_ul( ul );
      meta.corner_lr( lr );
      meta.corner_ur( ur );
      meta.corner_ll( ll );

      LOG_DEBUG( "Corner points successfully interpolated" );
    }
    else if( settings_.algorithm == interpolate_corners_settings::REGRESSION )
    {
      if( ref_points_.size() < settings_.min_points_to_use )
      {
        LOG_DEBUG( "Not enough points for homography regression" );
        return false;
      }

      std::vector< vnl_vector<double> > from_pts;
      std::vector< vnl_vector<double> > to_pts;

      vnl_vector<double> new_point( 3 );
      new_point[2] = 1.0;

      // Handle all past points (and the current one)
      for( unsigned i = 0; i < ref_points_.size(); ++i )
      {
        const point_t& past_point = ref_points_[i];

        double current_i = 0.5 * ni;
        double current_j = 0.5 * nj;

        vgl_h_matrix_2d< double > cur_to_past =
          past_point.homog.get_transform().get_inverse() * current.homog.get_transform();

        vnl_double_3 cur_point( current_i, current_j, 1.0 );
        vnl_double_3 past_image_point = cur_to_past.get_matrix() * cur_point;

        if( past_image_point[ 2 ] == 0 )
        {
          LOG_DEBUG( "Mapped points maps to infinity" );
          return false;
        }

        double past_center_i = past_image_point[ 0 ] / past_image_point[ 2 ];
        double past_center_j = past_image_point[ 1 ] / past_image_point[ 2 ];

        new_point[0] = past_center_i;
        new_point[1] = past_center_j;
        from_pts.push_back( new_point );

        new_point[0] = past_point.loc.get_latitude();
        new_point[1] = past_point.loc.get_longitude();
        to_pts.push_back( new_point );
      }

      // Perform estimation
      vnl_double_3x3 temp_matrix;
      vgl_h_matrix_2d<double> final_homog;

      rrel_homography2d_est hg( from_pts, to_pts );

      rrel_trunc_quad_obj msac;
      ransam_.set_trace_level(0);

      if( !ransam_.estimate( &hg, &msac ) )
      {
        LOG_DEBUG( "Random sampling failed" );
        return false;
      }

      rrel_irls irls;
      irls.initialize_params( ransam_.params() );

      if( !irls.estimate( &hg, &msac ) )
      {
        hg.params_to_homog( ransam_.params(), temp_matrix.as_ref().non_const() );
      }
      else
      {
        hg.params_to_homog( irls.params(), temp_matrix.as_ref().non_const() );
      }

      if( settings_.refine_geometric_error )
      {
        std::vector< vgl_homg_point_2d< double > > from_pts_refined;
        std::vector< vgl_homg_point_2d< double > > to_pts_refined;

        for( unsigned i = 0; i < from_pts.size(); ++i )
        {
          from_pts_refined.push_back( vgl_homg_point_2d< double >( from_pts[i][0], from_pts[i][1] ) );
          to_pts_refined.push_back( vgl_homg_point_2d< double >( to_pts[i][0], to_pts[i][1] ) );
        }

        vgl_h_matrix_2d<double> initial_homog( temp_matrix );

        vgl_h_matrix_2d_optimize_lmq opt( initial_homog );
        opt.set_verbose( false );

        if( !opt.optimize( from_pts_refined, to_pts_refined, final_homog ) )
        {
          LOG_WARN( "LMQ Refinement failed" );
        }
      }
      else
      {
        final_homog = vgl_h_matrix_2d<double>( temp_matrix );
      }

      // Set homography in metadata packet
      video_metadata::homography_t refined_homography;
      refined_homography.set_transform( final_homog );
      refined_homography.set_source_reference( ts );
      meta.img_to_wld_homog( refined_homography );

      LOG_DEBUG( "Image to world matrix successfully generated" );

      // Set corner points in metadata packet
      vgl_homg_point_2d< double > ul_img( 0, 0 );
      vgl_homg_point_2d< double > lr_img( ni, nj );
      vgl_homg_point_2d< double > ur_img( ni, 0 );
      vgl_homg_point_2d< double > ll_img( 0, nj );

      vgl_homg_point_2d< double > ul_wld = final_homog * ul_img;
      vgl_homg_point_2d< double > lr_wld = final_homog * lr_img;
      vgl_homg_point_2d< double > ur_wld = final_homog * ur_img;
      vgl_homg_point_2d< double > ll_wld = final_homog * ll_img;

      if( ul_wld.w() != 0 && lr_wld.w() != 0 && ur_wld.w() != 0 && ll_wld.w() )
      {
        geo_coord::geo_lat_lon ul( ul_wld.x() / ul_wld.w(), ul_wld.y() / ul_wld.w() );
        geo_coord::geo_lat_lon lr( lr_wld.x() / lr_wld.w(), lr_wld.y() / lr_wld.w() );
        geo_coord::geo_lat_lon ur( ur_wld.x() / ur_wld.w(), ur_wld.y() / ur_wld.w() );
        geo_coord::geo_lat_lon ll( ll_wld.x() / ll_wld.w(), ll_wld.y() / ll_wld.w() );

        meta.corner_ul( ul );
        meta.corner_lr( lr );
        meta.corner_ur( ur );
        meta.corner_ll( ll );

        LOG_DEBUG( "Corner points filled into metadata" );
      }
    }

    return true;
  }

  void clear_other_metadata_contents( video_metadata& md )
  {
    md.sensor_vert_fov( 0 );
    md.sensor_horiz_fov( 0 );

    md.slant_range( 0.0 );

    md.horizontal_gsd( 0 );
    md.vertical_gsd( 0 );

    md.img_to_wld_homog( video_metadata::homography_t() );
  }
};


interpolate_corners_from_shift
::interpolate_corners_from_shift()
  : d( new priv() )
{
  configure( interpolate_corners_settings() );
}


interpolate_corners_from_shift
::~interpolate_corners_from_shift()
{
}


void
interpolate_corners_from_shift
::configure( const interpolate_corners_settings& settings )
{
  d->settings_ = settings;

  d->seen_corners_ = false;
  d->ref_points_.set_capacity( settings.max_points_to_use );
}


bool
interpolate_corners_from_shift
::process_packet( const unsigned ni, const unsigned nj,
                  const image_to_image_homography& homog,
                  const timestamp ts,
                  video_metadata& meta )
{
  return d->update_metadata( ni, nj, homog, ts, meta );
}


} // end namespace vidtk
