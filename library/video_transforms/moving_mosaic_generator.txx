/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/moving_mosaic_generator.h>
#include <video_transforms/warp_image.h>
#include <video_transforms/nearest_neighbor_inpaint.h>

#include <utilities/point_view_to_region.h>

#include <string>
#include <algorithm>
#include <map>
#include <numeric>
#include <cassert>
#include <vector>
#include <limits>

#include <vil/vil_fill.h>
#include <vil/vil_save.h>
#include <vil/vil_copy.h>
#include <vil/vil_plane.h>
#include <vil/vil_convert.h>
#include <vil/algo/vil_threshold.h>

#include <vnl/vnl_inverse.h>
#include <vnl/vnl_double_3.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>
#include <vgl/vgl_polygon.h>
#include <vgl/vgl_convex.h>
#include <vgl/vgl_area.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "moving_mosaic_generator" );

template< typename PixType, typename MosaicType >
moving_mosaic_generator< PixType, MosaicType >
::moving_mosaic_generator()
{
  active_scene = false;
}

template< typename PixType, typename MosaicType >
moving_mosaic_generator< PixType, MosaicType >
::~moving_mosaic_generator()
{
  if( active_scene )
  {
    output_mosaic( mosaic );
  }
}

template< typename PixType, typename MosaicType >
bool
moving_mosaic_generator< PixType, MosaicType >
::configure( const moving_mosaic_settings& s )
{
  if( active_scene )
  {
    output_mosaic( mosaic );
  }

  settings = s;
  mosaic = new_mosaic();
  reset_mosaic( mosaic );
  increment_mosaic_id();

  ref_to_mosaic.set_identity();
  active_scene = false;
  is_first = true;

  if( !settings.mosaic_output_dir.empty() )
  {
    boost::filesystem::path dir_to_make( settings.mosaic_output_dir );
    boost::filesystem::create_directory( dir_to_make );
  }

  if( !settings.mosaic_homog_file.empty() && !settings.mosaic_output_dir.empty() )
  {
    homog_fn = settings.mosaic_output_dir + "/" + settings.mosaic_homog_file;
  }
  else
  {
    homog_fn = std::string();
  }

  return true;
}

namespace
{

template< typename T1, typename T2 >
inline void
interp_use_latest( T1* dest, T2* src,
                   double sx, double sy,
                   std::ptrdiff_t sxstep, std::ptrdiff_t systep,
                   unsigned planes,
                   std::ptrdiff_t spstep, std::ptrdiff_t dpstep )
{
  int ix = static_cast<int>( sx + 0.5 );
  int iy = static_cast<int>( sy + 0.5 );

  const T2* src_ptr = src + ix*sxstep + iy*systep;

  if( *src_ptr == 0 )
  {
    return;
  }

  T1* dest_ptr = dest;

  for( unsigned p = 0; p < planes; ++p, src_ptr+=spstep, dest_ptr+=dpstep )
  {
    *dest_ptr = static_cast<T1>( *src_ptr );
  }
}

template< typename T1, typename T2 >
inline void
interp_exp_average( T1* dest, T2* src,
                    double sx, double sy,
                    std::ptrdiff_t sxstep, std::ptrdiff_t systep,
                    unsigned planes,
                    std::ptrdiff_t spstep, std::ptrdiff_t dpstep )
{
  int ix = static_cast<int>( sx + 0.5 );
  int iy = static_cast<int>( sy + 0.5 );

  const T2* src_ptr = src + ix*sxstep + iy*systep;

  if( *src_ptr == 0 )
  {
    return;
  }

  T1* dest_ptr = dest;

  if( *dest_ptr == 0 )
  {
    for( unsigned p = 0; p < planes; ++p, src_ptr+=spstep, dest_ptr+=dpstep )
    {
      *dest_ptr = static_cast<T1>( *src_ptr );
    }
  }
  else
  {
    for( unsigned p = 0; p < planes; ++p, src_ptr+=spstep, dest_ptr+=dpstep )
    {
      *dest_ptr = ( 5*static_cast<unsigned>( *src_ptr ) +
                    3*static_cast<unsigned>( *dest_ptr ) + 4 ) >> 3;
    }
  }
}

template< typename T1, typename T2 >
inline void
interp_cum_average( T1* dest, T2* src,
                    double sx, double sy,
                    std::ptrdiff_t sxstep, std::ptrdiff_t systep,
                    unsigned planes,
                    std::ptrdiff_t spstep, std::ptrdiff_t dpstep )
{
  int ix = static_cast<int>( sx + 0.5 );
  int iy = static_cast<int>( sy + 0.5 );

  const T2* src_ptr = src + ix*sxstep + iy*systep;

  if( *src_ptr == 0 )
  {
    return;
  }

  T1* dest_ptr = dest;
  unsigned p;

  for( p = 0; p < planes; ++p, src_ptr+=spstep, dest_ptr+=dpstep )
  {
    *dest_ptr += static_cast<T1>( *src_ptr );
  }

  *dest_ptr = *dest_ptr + 1.0;
}

bool
does_intersect( const std::vector< vgl_point_2d<double> >& poly,
                const vgl_point_2d<double>& pt )
{
  vgl_polygon<double> pol( &poly[0], poly.size() );
  return pol.contains( pt );
}


void
intersecting_points( const unsigned ni, const unsigned nj,
                     const std::vector< vgl_point_2d<double> >& opoly,
                     std::vector< vgl_point_2d<double> >& output )
{
  vgl_box_2d<double> borders( 0, ni, 0, nj );
  vgl_line_segment_3d<double> lines1[4];
  vgl_line_segment_3d<double> lines2[4];

  vgl_point_3d<double> poly[4];

  for( unsigned i = 0; i < opoly.size(); i++ )
  {
    poly[i] = vgl_point_3d<double>( opoly[i].x(), opoly[i].y(), 0 );
  }

  lines1[0] = vgl_line_segment_3d<double>( poly[0], poly[1] );
  lines1[1] = vgl_line_segment_3d<double>( poly[1], poly[2] );
  lines1[2] = vgl_line_segment_3d<double>( poly[2], poly[3] );
  lines1[3] = vgl_line_segment_3d<double>( poly[3], poly[0] );

  lines2[0] = vgl_line_segment_3d<double>( vgl_point_3d<double>( 0, 0, 0 ), vgl_point_3d<double>( ni, 0, 0 ) );
  lines2[1] = vgl_line_segment_3d<double>( vgl_point_3d<double>( ni, 0, 0 ), vgl_point_3d<double>( ni, nj, 0 ) );
  lines2[2] = vgl_line_segment_3d<double>( vgl_point_3d<double>( ni, nj, 0 ), vgl_point_3d<double>( 0, nj, 0 ) );
  lines2[3] = vgl_line_segment_3d<double>( vgl_point_3d<double>( 0, nj, 0 ), vgl_point_3d<double>( 0, 0, 0 ) );

  for( unsigned i = 0; i < 4; i++ )
  {
    for( unsigned j = 0; j < 4; j++ )
    {
      vgl_point_3d<double> new_pt;

      if( vgl_intersection( lines1[i], lines2[j], new_pt ) )
      {
        output.push_back( vgl_point_2d<double>( new_pt.x(), new_pt.y() ) );
      }
    }
  }
}

double
convex_area( const std::vector< vgl_point_2d<double> >& poly )
{
  return vgl_area( vgl_convex_hull( poly ) );
}

bool
compare_poly_pts( const vgl_point_2d<double>& p1, const vgl_point_2d<double>& p2 )
{
  if( p1.x() < p2.x() )
    return true;
  else if( p1.x() > p2.x() )
    return false;

  return ( p1.y() < p2.y() );
}

void
remove_duplicates( std::vector< vgl_point_2d<double> >& poly )
{
  if( poly.size() == 0 )
    return;

  std::sort( poly.begin(), poly.end(), compare_poly_pts );

  std::vector< vgl_point_2d<double> > filtered;

  filtered.push_back( poly[0] );
  for( unsigned i = 1; i < poly.size(); i++ )
  {
    if( poly[i] != poly[i-1] )
    {
      filtered.push_back( poly[i] );
    }
  }

  if( filtered.size() != poly.size() )
  {
    poly = filtered;
  }
}

double
compute_reference_overlap( const vgl_h_matrix_2d< double >& homog,
                           const unsigned ni, const unsigned nj )
{
  std::vector< vgl_point_2d<double> > polygon_points;

  // 1. Warp corner points to current image
  std::vector< vgl_point_2d<double> > img1_pts;
  img1_pts.push_back( vgl_point_2d<double>( 0, 0 ) );
  img1_pts.push_back( vgl_point_2d<double>( ni, 0 ) );
  img1_pts.push_back( vgl_point_2d<double>( ni, nj ) );
  img1_pts.push_back( vgl_point_2d<double>( 0, nj ) );

  const vnl_double_3x3& transform = homog.get_matrix();
  std::vector< vgl_point_2d<double> > img2_pts( 4 );
  for( unsigned i = 0; i < 4; i++ )
  {
    vnl_double_3 hp( img1_pts[i].x(), img1_pts[i].y(), 1.0 );
    vnl_double_3 wp = transform * hp;

    if( wp[2] != 0 )
    {
      img2_pts[i] = vgl_point_2d<double>( wp[0] / wp[2] , wp[1] / wp[2] );
    }
    else
    {
      return 0.0;
    }
  }

  // 2. Calculate intersection points of the input image with each other
  for( unsigned i = 0; i < 4; i++ )
  {
    if( does_intersect( img1_pts, img2_pts[i] ) )
    {
      polygon_points.push_back( img2_pts[i] );
    }
    if( does_intersect( img2_pts, img1_pts[i] ) )
    {
      polygon_points.push_back( img1_pts[i] );
    }
  }

  intersecting_points( ni, nj, img2_pts, polygon_points );

  remove_duplicates( polygon_points );

  if( polygon_points.size() < 3 )
  {
    return 0.0;
  }

  // 3. Compute triangulized area
  double intersection_area = convex_area( polygon_points );
  double frame_area = ni * nj;

  // 4. Return area overlap
  return ( frame_area > 0 ? intersection_area / frame_area : 0 );
}

}

template< typename PixType, typename MosaicType >
double
moving_mosaic_generator< PixType, MosaicType >
::add_image( const image_t& image,
             const mask_t& mask,
             const homography_t& homog )
{
  if( !homog.is_valid() )
  {
    return -1.0;
  }

  // Modify source image by filling in masked pixels
  image_t source;
  source.deep_copy( image );

  const unsigned si = source.ni();
  const unsigned sj = source.nj();
  const unsigned sp = source.nplanes();

  PixType* src_start              = source.top_left_ptr();
  const std::ptrdiff_t src_j_step = source.jstep();
  const std::ptrdiff_t src_i_step = source.istep();
  const std::ptrdiff_t src_p_step = source.planestep();

  const bool* mask_start           = mask.top_left_ptr();
  const std::ptrdiff_t mask_j_step = mask.jstep();
  const std::ptrdiff_t mask_i_step = mask.istep();

  PixType* src_row = src_start;
  const bool* mask_row = mask_start;

  for( unsigned j = 0; j < sj; ++j, src_row+=src_j_step, mask_row+=mask_j_step )
  {
    PixType* src_ptr = src_row;
    const bool* mask_ptr = mask_row;

    for( unsigned i = 0; i < si; ++i, src_ptr+=src_i_step, mask_ptr+=mask_i_step )
    {
      if( *mask_ptr == true )
      {
        *src_ptr = 0;
      }
      else if( *src_ptr == 0 )
      {
        *src_ptr = 1;
      }
    }
  }

  // Determine if this is a new shot
  transform_t transform = homog.get_transform();

  bool new_shot = ( homog.get_dest_reference() != ref_frame ||
                    homog.is_new_reference() );

  ref_frame = homog.get_dest_reference();

  // Only reset mosaic image when required for efficiency
  if( new_shot && active_scene )
  {
    output_mosaic( mosaic );
    increment_mosaic_id();
    reset_mosaic( mosaic );
    active_scene = false;
  }

  // On first image of new shot compute initial translation ref_to_mosaic
  const unsigned mi = mosaic.ni();
  const unsigned mj = mosaic.nj();

  if( new_shot )
  {
    unsigned oi = ( mi - si ) / 2;
    unsigned oj = ( mj - sj ) / 2;

    ref_to_mosaic.set_identity();
    ref_to_mosaic.set_translation( oi, oj );

    is_first = true;
  }
  else
  {
    is_first = false;
  }

  transform = ref_to_mosaic * transform;

  // Determine if we need to update the translation matrix
  typedef vgl_homg_point_2d<double> homog_point_t;
  typedef vgl_point_2d<double> point_t;
  typedef vgl_box_2d<double> box_t;

  box_t src_on_mosaic_bbox;

  homog_point_t cnrs[4] =
    { homog_point_t( 0,      0      ),
      homog_point_t( si - 1, 0      ),
      homog_point_t( si - 1, sj - 1 ),
      homog_point_t( 0,      sj - 1 ) };

  bool adjustment_required = false;

  point_t pts[4];

  for( unsigned i = 0; i < 4; ++i )
  {
    point_t p = transform * cnrs[i];
    src_on_mosaic_bbox.add( p );

    if( p.x() < 0 || p.x() > mi || p.y() < 0 || p.y() > mj )
    {
      adjustment_required = true;
    }

    if( p.x() != p.x() || p.y() != p.y() )
    {
      return -1.0;
    }

    pts[i] = p;
  }

  if( adjustment_required )
  {
    bool transform_updated = false;

    //Check if scaling is also desirable
    double width = std::max( std::abs( pts[0].x() - pts[1].x() ),
                             std::abs( pts[0].x() - pts[2].x() ) );
    double height = std::max( std::abs( pts[1].y() - pts[2].y() ),
                              std::abs( pts[1].y() - pts[3].y() ) );

    // Expensive case, we are not doing simply a translation
    if( width > ( si * 1.25 ) || width < ( si * 0.50 ) ||
        height > ( sj * 1.25 ) || height < ( sj * 0.50 ) )
    {
      // Generate current frame to new mosaic homography
      transform_t cur_to_new;

      unsigned oi = ( mi - si ) / 2;
      unsigned oj = ( mj - sj ) / 2;

      cur_to_new.set_identity();
      cur_to_new.set_translation( oi, oj );

      // Generate old mosaic to new mosaic homography
      transform_t cur_to_old = transform;
      transform_t new_to_old = cur_to_old * cur_to_new.get_inverse();

      // Output old mosaic
      output_mosaic( mosaic );
      increment_mosaic_id();

      // Warp old mosaic to new mosaic
      mosaic_t updated_mosaic = new_mosaic();

      warp_image_parameters param;
      param.set_fill_unmapped( true );
      param.set_unmapped_value( 0.0 );
      param.set_interpolator( warp_image_parameters::NEAREST );

      warp_image( mosaic, updated_mosaic, new_to_old, param );
      mosaic = updated_mosaic;

      // Update stored transform, ref_to_mosaic is ref to mosaic before
      ref_to_mosaic = new_to_old.get_inverse() * ref_to_mosaic;
      transform = ref_to_mosaic * homog.get_transform();
      transform_updated = true;
    }
    else // Standard case, we are doing a simple optimized translation
    {    // of the old mosaic

      // Estimate optimal translation values
      point_t centroid = transform * homog_point_t( si / 2, sj / 2 );

      int oi = mi / 2 - centroid.x();
      int oj = mj / 2 - centroid.y();

      if( oi != 0 || oj != 0 )
      {
        output_mosaic( mosaic );
        increment_mosaic_id();

        mosaic_t trnsl_mosaic = new_mosaic();
        reset_mosaic( trnsl_mosaic );

        // Update matrices
        transform_t new_ref_to_mosaic;
        new_ref_to_mosaic.set_identity();
        new_ref_to_mosaic.set_translation( oi, oj );
        transform = new_ref_to_mosaic * transform;
        ref_to_mosaic = new_ref_to_mosaic * ref_to_mosaic;

        // Switch to use new mosaic
        vgl_box_2d< int > boundaries( 0, mosaic.ni(), 0, mosaic.nj() );
        vgl_box_2d< int > old_box( 0, mosaic.ni(), 0, mosaic.nj() );
        vgl_box_2d< int > new_box( 0, mosaic.ni(), 0, mosaic.nj() );

        old_box.set_centroid_x( old_box.centroid_x() - oi );
        old_box.set_centroid_y( old_box.centroid_y() - oj );

        new_box.set_centroid_x( new_box.centroid_x() + oi );
        new_box.set_centroid_y( new_box.centroid_y() + oj );

        old_box = vgl_intersection( boundaries, old_box );
        new_box = vgl_intersection( boundaries, new_box );

        mosaic_t old_region = point_view_to_region( mosaic, old_box );
        mosaic_t new_region = point_view_to_region( trnsl_mosaic, new_box );

        vil_copy_reformat( old_region, new_region );
        mosaic = trnsl_mosaic;

        transform_updated = true;
      }
    }

    if( transform_updated )
    {
      // Update bounding box corners from current image to mosaic
      src_on_mosaic_bbox = box_t();

      for( unsigned i = 0; i < 4; ++i )
      {
        point_t p = transform * cnrs[i];
        src_on_mosaic_bbox.add( p );
      }
    }
  }

  // Optionally output mosaic to file
  if( !homog_fn.empty() )
  {
    std::ofstream fout( homog_fn.c_str(), std::ofstream::app );
    const vidtk::timestamp ts = homog.get_source_reference();

    fout.precision( 15 );
    fout << ts.frame_number() << " " << ts.time() << std::endl;
    fout << get_mosaic_fn( false ) << std::endl;
    fout.precision( 20 );
    fout << transform << std::endl;

    fout.close();
  }

  // Performing warping to mosaic
  typedef void (*interpolator_func)
    (MosaicType*, PixType*, double, double, std::ptrdiff_t,
    std::ptrdiff_t, unsigned, std::ptrdiff_t, std::ptrdiff_t);

  interpolator_func interp;
  switch( settings.mosaic_method )
  {
  case moving_mosaic_settings::USE_LATEST:
    interp = &interp_use_latest<MosaicType,PixType>;
    break;
  case moving_mosaic_settings::EXP_AVERAGE:
    interp = &interp_exp_average<MosaicType,PixType>;
    break;
  case moving_mosaic_settings::CUM_AVERAGE:
    interp = &interp_cum_average<MosaicType,PixType>;
    break;
  default:
    LOG_FATAL( "Unrecognized method: " << settings.mosaic_method );
    return -1.0;
  }

  box_t mosaic_boundaries( 0, mi - 1, 0, mj - 1 );
  box_t intersection = vgl_intersection( src_on_mosaic_bbox, mosaic_boundaries );

  bool point_intercept = false;
  if( intersection.min_x() == intersection.max_x() &&
      intersection.min_y() == intersection.max_y() )
  {
    point_intercept = true;
  }

  if( intersection.width() == 0 && intersection.height() == 0 && !point_intercept)
  {
    return -1.0;
  }

  int src_ni_low_bound = 0;
  int src_nj_low_bound = 0;
  int src_ni_up_bound  = si - 1;
  int src_nj_up_bound  = sj - 1;

  // Extract start and end, row/col scanning ranges [start..end-1]
  const int start_j = static_cast<int>( std::floor( intersection.min_y() ) );
  const int start_i = static_cast<int>( std::floor( intersection.min_x() ) );
  const int end_j   = static_cast<int>( std::floor( intersection.max_y() + 1 ) );
  const int end_i   = static_cast<int>( std::floor( intersection.max_x() + 1 ) );

  // Get pointers to image data and retrieve required step values
  MosaicType* row_start              = mosaic.top_left_ptr();
  const std::ptrdiff_t mosaic_j_step = mosaic.jstep();
  const std::ptrdiff_t mosaic_i_step = mosaic.istep();
  const std::ptrdiff_t mosaic_p_step = mosaic.planestep();

  // Adjust mosaicination itr position to start of bounding box
  row_start = row_start + start_j * mosaic_j_step + mosaic_i_step * start_i;

  // Precompute partial column homography values
  int factor_size = end_i - start_i;

  vnl_matrix_fixed<double,3,3> h3x3 = transform.get_inverse().get_matrix();
  vnl_double_3 homog_col_1 = h3x3.get_column( 0 );
  vnl_double_3 homog_col_2 = h3x3.get_column( 1 );
  vnl_double_3 homog_col_3 = h3x3.get_column( 2 );

  std::vector< vnl_double_3 > col_factors( factor_size );

  for( int i = 0; i < factor_size; i++ )
  {
    double col = double( i + start_i );
    col_factors[ i ] = col * homog_col_1 + homog_col_3;
  }

  // Perform scan of boxed region
  for( int j = start_j; j < end_j; j++, row_start += mosaic_j_step )
  {
    // mosaic_col_ptr now points to the start of the BB region for this row
    MosaicType* mosaic_col_ptr = row_start;

    // Precompute row homography partials for this row
    const vnl_double_3 row_factor = homog_col_2 * static_cast<double>( j );

    // Get pointer to start of precomputed column values
    vnl_double_3* col_factor_ptr = &col_factors[0];

    // Iterate through each column in the BB
    for( int i = start_i; i < end_i; i++, col_factor_ptr++, mosaic_col_ptr += mosaic_i_step )
    {
      // Compute homography mapping for this point (mosaic->src)
      vnl_double_3 pt = row_factor + (*col_factor_ptr);

      // Normalize by dividing out third term
      double& x = pt[0];
      double& y = pt[1];
      double& w = pt[2];

      x /= w;
      y /= w;

      // Check if we can perform interp at this point
      if( x >= src_ni_low_bound && y >= src_nj_low_bound && x <= src_ni_up_bound && y <= src_nj_up_bound )
      {
        (*interp)( mosaic_col_ptr, src_start,
                   x, y, src_i_step, src_j_step,
                   sp, src_p_step, mosaic_p_step );
      }
    }
  }

  // Set active scene flag if homography is non-identity
  if( !homog.get_transform().is_identity() )
  {
    active_scene = true;
  }

  return compute_reference_overlap( homog.get_transform(), si, sj );
}

template< typename PixType, typename MosaicType >
bool
moving_mosaic_generator< PixType, MosaicType >
::get_pixels( const mask_t& input_mask,
              const homography_t& homog,
              image_t& output_image,
              mask_t& output_mask )
{
  // Resize output images, validate inputs
  output_image.set_size( input_mask.ni(), input_mask.nj(), settings.mosaic_resolution[2] );
  output_mask.set_size( input_mask.ni(), input_mask.nj() );
  output_mask.fill( true );

  if( !homog.is_valid() || homog.get_dest_reference() != ref_frame || is_first )
  {
    return false;
  }

  transform_t dest_to_src_homography = ref_to_mosaic * homog.get_transform();
  transform_t src_to_dest_homography = dest_to_src_homography.get_inverse();

  // Retrieve destination and source image properties
  unsigned const dni = output_image.ni();
  unsigned const dnj = output_image.nj();

  unsigned const sni = mosaic.ni();
  unsigned const snj = mosaic.nj();
  unsigned const snp = settings.mosaic_resolution[2];

  typedef vgl_homg_point_2d<double> homog_point_t;
  typedef vgl_point_2d<double> point_t;
  typedef vgl_box_2d<double> box_t;

  box_t src_on_dest_bounding_box;

  homog_point_t cnrs[4] = { homog_point_t( 0, 0 ),
                            homog_point_t( sni - 1, 0 ),
                            homog_point_t( sni - 1, snj - 1 ),
                            homog_point_t( 0, snj - 1 ) };

  for( unsigned i = 0; i < 4; ++i )
  {
    point_t p = src_to_dest_homography * cnrs[i];
    src_on_dest_bounding_box.add( p );
  }

  box_t dest_boundaries( 0, dni - 1, 0, dnj - 1 );
  box_t intersection = vgl_intersection( src_on_dest_bounding_box, dest_boundaries );

  bool point_intercept = false;

  if( intersection.min_x() == intersection.max_x() &&
      intersection.min_y() == intersection.max_y() )
  {
    point_intercept = true;
  }

  if( intersection.width() == 0 && intersection.height() == 0 && !point_intercept)
  {
    return false;
  }

  int src_ni_low_bound = 0;
  int src_nj_low_bound = 0;
  int src_ni_up_bound  = sni - 1;
  int src_nj_up_bound  = snj - 1;

  const int start_j = static_cast<int>( std::floor( intersection.min_y() ) );
  const int start_i = static_cast<int>( std::floor( intersection.min_x() ) );
  const int end_j   = static_cast<int>( std::floor( intersection.max_y() + 1 ) );
  const int end_i   = static_cast<int>( std::floor( intersection.max_x() + 1 ) );

  const bool* mask_row             = input_mask.top_left_ptr();
  PixType* dest_row                = output_image.top_left_ptr();
  const MosaicType* src_start      = mosaic.top_left_ptr();
  const std::ptrdiff_t mask_j_step = input_mask.jstep();
  const std::ptrdiff_t dest_j_step = output_image.jstep();
  const std::ptrdiff_t src_j_step  = mosaic.jstep();
  const std::ptrdiff_t mask_i_step = input_mask.istep();
  const std::ptrdiff_t dest_i_step = output_image.istep();
  const std::ptrdiff_t src_i_step  = mosaic.istep();
  const std::ptrdiff_t dest_p_step = output_image.planestep();
  const std::ptrdiff_t src_p_step  = mosaic.planestep();

  // Adjust destination itr position to start of bounding box
  dest_row = dest_row + start_j * dest_j_step + dest_i_step * start_i;
  mask_row = mask_row + start_j * mask_j_step + mask_i_step * start_i;

  // Precompute partial column homography values
  int factor_size = end_i - start_i;

  vnl_matrix_fixed<double,3,3> h3x3 = dest_to_src_homography.get_matrix();
  vnl_double_3 homog_col_1 = h3x3.get_column( 0 );
  vnl_double_3 homog_col_2 = h3x3.get_column( 1 );
  vnl_double_3 homog_col_3 = h3x3.get_column( 2 );

  std::vector< vnl_double_3 > col_factors( factor_size );

  for( int i = 0; i < factor_size; i++ )
  {
    double col = double( i + start_i );
    col_factors[ i ] = col * homog_col_1 + homog_col_3;
  }

  // Perform scan of boxed region
  for( int j = start_j; j < end_j; j++, dest_row += dest_j_step, mask_row += mask_j_step )
  {
    // dest_col_ptr now points to the start of the BB region for this row
    PixType* dest_col_ptr = dest_row;
    const bool* mask_col_ptr = mask_row;

    // Precompute row homography partials for this row
    const vnl_double_3 row_factor = homog_col_2 * static_cast<double>(j);

    // Get pointer to start of precomputed column values
    vnl_double_3* col_factor_ptr = &col_factors[0];

    // Iterate through each column in the BB
    for( int i = start_i; i < end_i; i++, col_factor_ptr++, dest_col_ptr += dest_i_step, mask_col_ptr += mask_i_step )
    {
      // Skip pixels we don't care about
      if( ! *mask_col_ptr )
      {
        continue;
      }

      // Compute homography mapping for this point (dest->src)
      vnl_double_3 pt = row_factor + (*col_factor_ptr);

      // Normalize by dividing out third term
      double& x = pt[0];
      double& y = pt[1];
      double& w = pt[2];

      x /= w;
      y /= w;

      // Check if we can perform interp at this point
      if( x >= src_ni_low_bound && y >= src_nj_low_bound && x <= src_ni_up_bound && y <= src_nj_up_bound )
      {
        // For each channel interpolate from src
        PixType* dest_ptr = dest_col_ptr;

        const MosaicType* src_ptr =
          src_start + static_cast<int>( x + 0.5 ) * src_i_step
                    + static_cast<int>( y + 0.5 ) * src_j_step;

        if( *src_ptr != 0 )
        {
          output_mask( i, j ) = false;

          for( unsigned p = 0; p < snp; ++p, src_ptr+=src_p_step, dest_ptr+=dest_p_step )
          {
            *dest_ptr = static_cast<PixType>( *src_ptr );
          }
        }
      }
    }
  }

  return true;
}

template< typename PixType, typename MosaicType >
vil_image_view< MosaicType >
moving_mosaic_generator< PixType, MosaicType >
::new_mosaic()
{
  return mosaic_t( settings.mosaic_resolution[0],
                   settings.mosaic_resolution[1],
                   settings.mosaic_resolution[2]
   + ( settings.mosaic_method ==
       moving_mosaic_settings::CUM_AVERAGE ? 1 : 0 ) );
}

template< typename PixType, typename MosaicType >
void
moving_mosaic_generator< PixType, MosaicType >
::reset_mosaic( mosaic_t& image )
{
  vil_fill( image, static_cast< MosaicType>( 0 ) );
}

template< typename PixType, typename MosaicType >
void
moving_mosaic_generator< PixType, MosaicType >
::increment_mosaic_id()
{
  static boost::mutex mmg_mosaic_id_mutex;
  static unsigned global_mosaic_id = 0;

  boost::unique_lock< boost::mutex > lock( mmg_mosaic_id_mutex );
  global_mosaic_id++;
  mosaic_id = global_mosaic_id;
}

template< typename PixType, typename MosaicType >
std::string
moving_mosaic_generator< PixType, MosaicType >
::get_mosaic_fn( bool with_path )
{
  std::string page_str = boost::lexical_cast< std::string >( mosaic_id );

  while( page_str.size() < 5 )
  {
    page_str = "0" + page_str;
  }

  std::string output_fn = "mosaic_" + page_str + ".png";

  if( with_path )
  {
    output_fn = settings.mosaic_output_dir + "/" + output_fn;
  }

  return output_fn;
}

template< typename PixType, typename MosaicType >
void
moving_mosaic_generator< PixType, MosaicType >
::output_mosaic( mosaic_t& image )
{
  if( settings.mosaic_output_dir.empty() )
  {
    return;
  }

  if( settings.inpaint_output_mosaics )
  {
    inpaint_mosaic( image );
  }

  vil_save( image, get_mosaic_fn( true ).c_str() );
}

void
suppress_border( vil_image_view< bool >& inpaint_mask )
{
  const unsigned mi = inpaint_mask.ni();
  const unsigned mj = inpaint_mask.nj();

  bool* mask_start = inpaint_mask.top_left_ptr();
  std::ptrdiff_t mask_j_step = inpaint_mask.jstep();
  std::ptrdiff_t mask_i_step = inpaint_mask.istep();

  bool* mask_row = mask_start;

  for( unsigned j = 0; j < mj; ++j, mask_row+=mask_j_step )
  {
    bool* mask_ptr = mask_row;
    unsigned i;

    for( i = 0; i < mi; ++i, mask_ptr+=mask_i_step )
    {
      if( *mask_ptr == true )
      {
        *mask_ptr = false;
      }
      else
      {
        break;
      }
    }

    if( i == mi )
    {
      continue;
    }
    else
    {
      mask_ptr = mask_row + mask_i_step * ( mi - 1 );

      for( i = 0; i < mi; ++i, mask_ptr-=mask_i_step )
      {
        if( *mask_ptr == true )
        {
          *mask_ptr = false;
        }
        else
        {
          break;
        }
      }
    }
  }
}

void
recursive_fill( bool* ptr, std::ptrdiff_t istep, std::ptrdiff_t jstep )
{
  *ptr = false;

  if( *( ptr + istep ) )
  {
    recursive_fill( ptr + istep, istep, jstep );
  }
  if( *( ptr - istep ) )
  {
    recursive_fill( ptr - istep, istep, jstep );
  }
  if( *( ptr + jstep ) )
  {
    recursive_fill( ptr + jstep, istep, jstep );
  }
  if( *( ptr - jstep ) )
  {
    recursive_fill( ptr - jstep, istep, jstep );
  }
}

void
suppress_loops( const vil_image_view< bool >& initial, vil_image_view< bool >& refined )
{
  const unsigned ni = initial.ni();
  const unsigned nj = initial.nj();

  std::ptrdiff_t init_i_step = initial.istep();
  std::ptrdiff_t init_j_step = initial.jstep();

  std::ptrdiff_t ref_i_step = refined.istep();
  std::ptrdiff_t ref_j_step = refined.jstep();

  const bool* init_row = initial.top_left_ptr() + init_i_step + init_j_step;
  bool* ref_row = refined.top_left_ptr() + ref_i_step + ref_j_step;

  for( unsigned j = 1; j < nj-1; ++j, init_row+=init_j_step, ref_row+=ref_j_step )
  {
    const bool* init_ptr = init_row;
    bool* ref_ptr = ref_row;
    unsigned i;

    for( i = 0; i < ni-1; ++i, init_ptr+=init_i_step, ref_ptr+=ref_i_step )
    {
      if( *init_ptr == true )
      {
        recursive_fill( ref_ptr, ref_i_step, ref_j_step );
      }
      else
      {
        break;
      }
    }

    if( i >= ni-1 )
    {
      continue;
    }
    else
    {
      init_ptr = init_row + init_i_step * ( ni - 3 );
      ref_ptr = ref_row + ref_i_step * ( ni - 3 );

      for( i = 0; i < ni-2; ++i, init_ptr-=init_i_step, ref_ptr-=ref_i_step )
      {
        if( *init_ptr == true )
        {
          recursive_fill( ref_ptr, ref_i_step, ref_j_step );
        }
        else
        {
          break;
        }
      }
    }
  }
}

template< typename PixType, typename MosaicType >
void
moving_mosaic_generator< PixType, MosaicType >
::inpaint_mosaic()
{
  inpaint_mosaic( mosaic );
}

template< typename PixType, typename MosaicType >
void
moving_mosaic_generator< PixType, MosaicType >
::inpaint_mosaic( mosaic_t& image )
{
  if( image.size() == 0 )
  {
    return;
  }

  // Generate mask - 2 pass approach, one to knock off a bulk of the
  // black pixels around the mosaic, and a second to take care of
  // any alcolves within the mosaic touching the border.
  mask_t black_mask( image.ni(), image.nj() );
  mask_t inpaint_mask( image.ni(), image.nj() );

  vil_threshold_below( vil_plane( image, 0 ), black_mask,
    static_cast<MosaicType>( 0 ) );

  inpaint_mask.deep_copy( black_mask );

  suppress_border( inpaint_mask );
  suppress_loops( black_mask, inpaint_mask );

  // Inpaint mask
  if( settings.inpainting_method == moving_mosaic_settings::NEAREST )
  {
    nn_inpaint( image, inpaint_mask );
  }
#ifdef USE_OPENCV
  else if( settings.inpainting_method == moving_mosaic_settings::TELEA ||
           settings.inpainting_method == moving_mosaic_settings::NAVIER )
  {
    vil_image_view< vxl_byte > casted_input;
    vil_image_view< vxl_byte > formatted_input;

    vil_convert_cast( image, casted_input );

    formatted_input = vil_image_view< vxl_byte >( casted_input.ni(),
                                                  casted_input.nj(),
                                                  1,
                                                  casted_input.nplanes() );

    vil_copy_reformat( casted_input, formatted_input );

    vil_image_view< vxl_byte > byte_mask;
    vil_convert_stretch_range< bool >( inpaint_mask, byte_mask );

    cv::Mat ocv_input( formatted_input.nj(),
                       formatted_input.ni(),
                       ( formatted_input.nplanes() == 1 ) ? CV_8UC1 : CV_8UC3,
                       formatted_input.top_left_ptr(),
                       formatted_input.jstep() * sizeof( PixType ) );

    cv::Mat ocv_mask( byte_mask.nj(),
                      byte_mask.ni(),
                      CV_8UC1,
                      byte_mask.top_left_ptr(),
                      byte_mask.jstep() * sizeof( vxl_byte ) );

    // Create output image
    if( image.nplanes() != image.planestep() )
    {
      image = mosaic_t( casted_input.ni(),
                        casted_input.nj(),
                        1,
                        casted_input.nplanes() );

    }

    cv::Mat output_wrapper( image.nj(),
                            image.ni(),
                            ocv_input.type(),
                            image.top_left_ptr(),
                            image.jstep() * sizeof( PixType ) );

    // Run opencv inpaint
    int method = ( settings.inpainting_method == moving_mosaic_settings::TELEA
                   ? CV_INPAINT_TELEA : CV_INPAINT_NS );
    cv::inpaint( ocv_input, ocv_mask, output_wrapper, 4.0, method );
  }
#endif
  else
  {
    LOG_FATAL( "Support for selected inpainting mode not enabled at compile time" );
  }
}

template< typename PixType, typename MosaicType >
void
moving_mosaic_generator< PixType, MosaicType >
::reset_mosaic()
{
  reset_mosaic( mosaic );
}

} // end namespace vidtk
