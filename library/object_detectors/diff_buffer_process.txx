/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "diff_buffer_process.h"

#include <vnl/vnl_double_3.h>

#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_polygon.h>
#include <vgl/vgl_convex.h>
#include <vgl/vgl_line_segment_2d.h>
#include <vgl/vgl_line_segment_3d.h>
#include <vgl/vgl_area.h>
#include <vgl/vgl_intersection.h>

#include <algorithm>

#include <logger/logger.h>

VIDTK_LOGGER( "diff_buffer_process_txx" );

namespace vidtk
{

template <class PixType>
diff_buffer_process<PixType>
::diff_buffer_process( std::string const& _name )
  : process( _name, "diff_buffer_process" ),
    masking_enabled_( false ),
    disable_capacity_error_( false ),
    reset_on_next_pass_( false ),
    check_shot_break_flags_( false ),
    input_gsd_( 0 ),
    differencing_flag_( true ),
    requested_action_( gui_frame_info::PROCESS ),
    min_allowed_( 0 )
{
  config_.add_parameter(
    "spacing",
    "2",
    "The spacing between the images to be differenced, in frames. "
    "This spacing should be such that the object moves more than twice "
    "its size between the current frame and the frame \"spacing\" units "
    "prior.  This will allow the \"ghost\" image to be removed."
  );
  config_.add_parameter(
    "masking_enabled",
    "false",
    "Determines whether or not masks are buffered alongside images."
  );
  config_.add_parameter(
    "check_shot_break_flags",
    "false",
    "Should we spend time checking the input shot break flags, and reset "
    "the internal buffer if a shot break was found on this frame?"
  );
  config_.add_parameter(
    "min_image_overlap",
    "0",
    "The minimum percent [0.0, 1.0] overlap for frames that we should perform "
    "image differencing over. If the first and last frame in the differencing "
    "operation have an overlap less than this percentage, then the number of "
    "frames we're differencing over will be reduced until it is met."
  );
  config_.add_parameter(
    "min_adaptive_thresh",
    "0",
    "The fewest number of frames that we want to perform differencing over "
    "despite any of the other criteria that can be set by this process (such "
    "as the min_image_overlap option). Units are in numbers of frames."
  );
  config_.add_parameter(
    "min_operating_gsd",
    "0.0",
    "Minimum input GSD that this buffer should operate on. If the input "
    "GSD is less than this amount, the output frames for differencing "
    "will all be the same and result in a no-op."
  );
  config_.add_parameter(
    "max_operating_gsd",
    "1000.0",
    "Maximum input GSD that this buffer should operate on. If the input "
    "GSD is greater than this amount, the output frames for differencing "
    "will all be the same and result in a no-op."
  );
}


template <class PixType>
diff_buffer_process<PixType>
::~diff_buffer_process()
{
}


template <class PixType>
config_block
diff_buffer_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
diff_buffer_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    spacing_ = blk.get< unsigned >( "spacing" );
    masking_enabled_ = blk.get< bool >( "masking_enabled" );
    check_shot_break_flags_ = blk.get< bool >( "check_shot_break_flags" );
    min_overlap_ = blk.get< double >( "min_image_overlap" );
    min_adaptive_thresh_ = blk.get< double >( "min_adaptive_thresh" );
    enable_min_overlap_ = min_overlap_ > 0.0001;

    img_buffer_.set_capacity( spacing_ + 1 );
    homog_buffer_.set_capacity( spacing_ + 1 );

    if( masking_enabled_ )
    {
      mask_buffer_.set_capacity( spacing_ + 1 );
    }

    min_operating_gsd_ = blk.get< double >( "min_operating_gsd" );
    max_operating_gsd_ = blk.get< double >( "max_operating_gsd" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
diff_buffer_process<PixType>
::initialize()
{
  img_buffer_.clear();
  mask_buffer_.clear();
  homog_buffer_.clear();

  current_end_idx_ = 0;

  first_to_ref_.set_identity();
  second_to_ref_.set_identity();
  third_to_ref_.set_identity();
  third_to_first_.set_identity();
  third_to_second_.set_identity();
  return true;
}


template <class PixType>
bool
diff_buffer_process<PixType>
::reset()
{
  return this->initialize();
}

template <class PixType>
double
diff_buffer_process<PixType>
::po_per_index( const unsigned index )
{
  first_to_ref_ = homog_buffer_.datum_at( index );
  third_to_ref_ = homog_buffer_.datum_at( 0 );
  first_to_third_ = third_to_ref_.get_inverse() * first_to_ref_;

  return percent_overlap( img_buffer_.datum_at(0).ni(),
                          img_buffer_.datum_at(0).nj(),
                          first_to_third_ );
}

template <class PixType>
void
diff_buffer_process<PixType>
::compute_index()
{
  unsigned max_end_idx = std::min( img_buffer_.size()-1, spacing_ );
  current_end_idx_ = max_end_idx;
  min_allowed_ = std::min( max_end_idx, min_adaptive_thresh_ );

  if( enable_min_overlap_ && img_buffer_.size() > 3 )
  {
    double last_po = po_per_index( current_end_idx_ );

    if( last_po > min_overlap_ )
    {
      return;
    }

    current_end_idx_ = current_end_idx_ / 2;

    if( current_end_idx_ < 3 )
    {
      current_end_idx_ = 3;
    }

    if( po_per_index( current_end_idx_) < min_overlap_ )
    {
      if( current_end_idx_ == 3 )
      {
        return;
      }

      unsigned max_to_search = current_end_idx_;
      current_end_idx_ = 3;

      double first_po = po_per_index( current_end_idx_ );

      if( first_po < min_overlap_ )
      {
        if( last_po != 0 && ( first_po == 0 || ( first_po > last_po && std::abs( first_po - last_po ) / last_po < 1.1 ) ) )
        {
          current_end_idx_ = max_end_idx;
        }
        return;
      }

      for( ; current_end_idx_ < max_to_search; current_end_idx_++ )
      {
        if( po_per_index( current_end_idx_ ) < min_overlap_ )
        {
          current_end_idx_--;
          return;
        }
      }

      current_end_idx_--;
      return;
    }
    else
    {
      current_end_idx_++;
      for( ; current_end_idx_ <= max_end_idx; current_end_idx_++ )
      {
        if( po_per_index( current_end_idx_ ) < min_overlap_ )
        {
          current_end_idx_--;
          return;
        }
      }
      current_end_idx_ = max_end_idx;
    }
  }
}

template <class PixType>
process::step_status
diff_buffer_process<PixType>
::step2()
{
  // Validate inputs
  if( !src_img_ || ( masking_enabled_ && !src_mask_ ) )
  {
    LOG_ERROR( "Not all required inputs specified" );
    return process::FAILURE;
  }

  img_buffer_.insert( src_img_ );
  homog_buffer_.insert( src_homog_ );

  if( masking_enabled_ )
  {
    mask_buffer_.insert( src_mask_ );
  }

  differencing_flag_ = true;

  // Check against gsd parameters
  if( input_gsd_ < min_operating_gsd_ || input_gsd_ > max_operating_gsd_ )
  {
    current_end_idx_ = 0;
    first_to_ref_.set_identity();
    second_to_ref_.set_identity();
    third_to_ref_.set_identity();
    third_to_first_.set_identity();
    third_to_second_.set_identity();
    differencing_flag_ = false;
    return process::SUCCESS;
  }

  // If we're processing GUI commands, the requested_action variable can
  // be set to other options which need to be handled by this process.
  if( reset_on_next_pass_ )
  {
    this->reset_buffer_to_last_entry();
    reset_on_next_pass_ = false;
  }

  if( requested_action_ != gui_frame_info::PROCESS )
  {
    switch( requested_action_ )
    {
      case gui_frame_info::BUFFER:
      {
        return process::SKIP;
      }
      case gui_frame_info::RESET:
      {
        current_end_idx_ = 0;
        this->reset_buffer_to_last_entry();
        reset_on_next_pass_ = true;
        return process::SKIP;
      }
      case gui_frame_info::RESET_AND_BUFFER:
      {
        this->reset_buffer_to_last_entry();
        return process::SKIP;
      }
      case gui_frame_info::RESET_AND_PROCESS:
      {
        this->reset_buffer_to_last_entry();
        break;
      }
      default:
      {
        LOG_ERROR( "Invalid GUI command received: " << requested_action_ );
        return process::FAILURE;
      }
    }
  }

  if( check_shot_break_flags_ && shot_break_flags_.is_shot_end() )
  {
    reset_on_next_pass_ = true;
  }

  // Formulate desired outputs
  this->compute_index();

  if( current_end_idx_ < min_allowed_ )
  {
    current_end_idx_ = min_allowed_;
  }

  first_to_ref_ = homog_buffer_.datum_at( current_end_idx_ );
  second_to_ref_ = homog_buffer_.datum_at( current_end_idx_ / 2 );
  third_to_ref_ = homog_buffer_.datum_at( 0 );
  third_to_first_ = first_to_ref_.get_inverse() * third_to_ref_;
  third_to_second_ = second_to_ref_.get_inverse() * third_to_ref_;

  src_img_ = vil_image_view< PixType >();
  src_mask_ = vil_image_view< bool >();
  return process::SUCCESS;
}

template <class PixType>
void
diff_buffer_process<PixType>
::reset_buffer_to_last_entry()
{
  if( img_buffer_.size() > 1 )
  {
    img_type last_image = img_buffer_.datum_at( 0 );
    homog_type last_homog = homog_buffer_.datum_at( 0 );
    mask_type last_mask;

    if( masking_enabled_ )
    {
      last_mask = mask_buffer_.datum_at( 0 );
    }

    initialize();
    img_buffer_.insert( last_image );
    homog_buffer_.insert( last_homog );

    if( masking_enabled_ )
    {
      mask_buffer_.insert( last_mask );
    }
  }
}


// I/O Functions

template <class PixType>
void
diff_buffer_process<PixType>
::set_source_img( vil_image_view<PixType> const& item )
{
  // Make sure image is non-empty
  if( !item )
  {
    LOG_WARN( "Input Image to Buffer is Empty" );
  }

  src_img_ = item;
}

template <class PixType>
void
diff_buffer_process<PixType>
::set_source_mask( vil_image_view<bool> const& item )
{
  // Make sure image is non-empty
  if( !item )
  {
    LOG_WARN( "Input Mask to Buffer is Empty" );
  }

  src_mask_ = item;
}

template <class PixType>
void
diff_buffer_process<PixType>
::set_source_homog( vgl_h_matrix_2d<double> const& item )
{
  src_homog_ = item;
}

template <class PixType>
void
diff_buffer_process<PixType>
::set_source_vidtk_homog( image_to_image_homography const& item )
{
  src_homog_ = item.get_transform();
}

template <class PixType>
void
diff_buffer_process<PixType>
::add_source_gui_feedback( gui_frame_info const& fb )
{
  requested_action_ = fb.action();
}

template <class PixType>
void
diff_buffer_process<PixType>
::add_shot_break_flags( shot_break_flags const& sbf )
{
  shot_break_flags_ = sbf;
}

template <class PixType>
vil_image_view<PixType>
diff_buffer_process<PixType>
::get_first_image() const
{
  return img_buffer_.datum_at( current_end_idx_ );
}

template <class PixType>
vil_image_view<PixType>
diff_buffer_process<PixType>
::get_second_image() const
{
  return img_buffer_.datum_at( current_end_idx_ / 2 );
}

template <class PixType>
vil_image_view<PixType>
diff_buffer_process<PixType>
::get_third_image() const
{
  return img_buffer_.datum_at( 0 );
}

template <class PixType>
vil_image_view<bool>
diff_buffer_process<PixType>
::get_first_mask() const
{
  return mask_buffer_.datum_at( current_end_idx_ );
}

template <class PixType>
vil_image_view<bool>
diff_buffer_process<PixType>
::get_second_mask() const
{
  return mask_buffer_.datum_at( current_end_idx_ / 2 );
}

template <class PixType>
vil_image_view<bool>
diff_buffer_process<PixType>
::get_third_mask() const
{
  return mask_buffer_.datum_at( 0 );
}

template <class PixType>
vgl_h_matrix_2d<double>
diff_buffer_process<PixType>
::get_third_to_first_homog() const
{
  return third_to_first_;
}

template <class PixType>
vgl_h_matrix_2d<double>
diff_buffer_process<PixType>
::get_third_to_second_homog() const
{
  return third_to_second_;
}

template <class PixType>
bool
diff_buffer_process<PixType>
::get_differencing_flag() const
{
  return differencing_flag_;
}

bool does_intersect( const std::vector< vgl_point_2d<double> >& poly, const vgl_point_2d<double>& pt )
{
  vgl_polygon<double> pol( &poly[0], poly.size() );
  return pol.contains( pt );
}

void intersecting_points( const unsigned ni, const unsigned nj,
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

double convex_area( const std::vector< vgl_point_2d<double> >& poly )
{
  return vgl_area( vgl_convex_hull( poly ) );
}

bool compare_poly_pts( const vgl_point_2d<double>& p1, const vgl_point_2d<double>& p2 )
{
  if( p1.x() < p2.x() )
    return true;
  else if( p1.x() > p2.x() )
    return false;

  return ( p1.y() < p2.y() );
}

void remove_duplicates( std::vector< vgl_point_2d<double> >& poly )
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

template <class PixType>
double
diff_buffer_process<PixType>
::percent_overlap( const unsigned ni, const unsigned nj, vgl_h_matrix_2d<double> homog )
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

template <class PixType>
void
diff_buffer_process<PixType>
::add_gsd( double _gsd )
{
  input_gsd_ = _gsd;
}

} // end namespace vidtk
