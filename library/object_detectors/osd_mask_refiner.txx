/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "osd_mask_refiner.h"

#include <utilities/point_view_to_region.h>

#include <video_transforms/automatic_white_balancing.h>

#include <learning/learner_data_class_simple_wrapper.h>
#include <learning/adaboost.h>
#include <classifier/hashed_image_classifier.h>
#include <utilities/timestamp.h>
#include <utilities/point_view_to_region.h>

#include <vil/vil_convert.h>
#include <vil/vil_math.h>
#include <vil/vil_fill.h>
#include <vil/vil_load.h>
#include <vil/vil_transform.h>
#include <vil/algo/vil_threshold.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_binary_erode.h>
#include <vil/algo/vil_blob.h>

#include <cmath>
#include <limits>
#include <fstream>
#include <sstream>
#include <limits>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <logger/logger.h>


VIDTK_LOGGER( "osd_mask_refiner" );


namespace vidtk
{


template <typename PixType, typename FeatureType>
bool
osd_mask_refiner<PixType,FeatureType>
::configure( const osd_mask_refiner_settings& options )
{
  // Load internal settings
  options_ = options;

  if( options_.dilation_radius != 0.0 )
  {
    dilation_el_.set_to_disk( options.dilation_radius );
  }

  if( options_.closing_radius != 0.0 )
  {
    closing_el_.set_to_disk( options.closing_radius );
  }

  if( options_.opening_radius != 0.0 )
  {
    opening_el_.set_to_disk( options.opening_radius );
  }

  pixel_training_mode_ = !options_.training_file.empty() &&
    options_.training_mode == osd_mask_refiner_settings::PIXEL_CLFR;
  component_training_mode_ = !options_.training_file.empty() &&
    options_.training_mode == osd_mask_refiner_settings::COMPONENT_CLFR;

  if( pixel_training_mode_ )
  {
    training_data_extractor_.reset( new feature_writer_t() );

    pixel_feature_writer_settings trainer_options;
    trainer_options.output_file = options_.training_file;
    trainer_options.gt_loader.load_mode = pixel_annotation_loader_settings::DIRECTORY;
    trainer_options.gt_loader.path = options.groundtruth_dir;

    if( !training_data_extractor_->configure( trainer_options ) )
    {
      LOG_ERROR( "Error setting up training data extractor" );
      return false;
    }
  }

  return true;
}

namespace
{

inline bool
union_functor( bool x1, bool x2 )
{
  return x1 || x2;
}


inline bool
intersection_functor( bool x1, bool x2 )
{
  return x1 && x2;
}


inline image_region
template_to_image_region( const vgl_box_2d<double>& region,
                          const unsigned ni, const unsigned nj )
{
  image_region scaled_region( static_cast<unsigned>( region.min_x() * ni ),
                              static_cast<unsigned>( region.max_x() * ni ),
                              static_cast<unsigned>( region.min_y() * nj ),
                              static_cast<unsigned>( region.max_y() * nj ) );

  return scaled_region;
}

inline void
check_pixel( bool* ptr, const std::ptrdiff_t istep, const std::ptrdiff_t jstep )
{
  if( ! *ptr )
  {
    return;
  }

  for( int dj = -1; dj <= 1; ++dj )
  {
    for( int di = -1; di <= 1; ++di )
    {
      if( *( ptr + di * istep + dj * jstep ) && di != 0 && dj != 0 )
      {
        return;
      }
    }
  }

  *ptr = false;
}


// Simple filter: remove pixels with no neighbors
void
filter_lone_pixels( vil_image_view< bool >& image )
{
  assert( image.nplanes() == 1 );

  const std::ptrdiff_t istep = image.istep();
  const std::ptrdiff_t jstep = image.jstep();

  for( unsigned j = 1; j < image.nj()-1; ++j )
  {
    bool* ptr = &image( 1, j );

    for( unsigned i = 1; i < image.ni()-1; ++i, ptr+=istep )
    {
      check_pixel( ptr, istep, jstep );
    }
  }
}

double
count_pixels( vil_image_view< bool >& image )
{
  unsigned counter = 0;

  const std::ptrdiff_t istep = image.istep();

  for( unsigned j = 0; j < image.nj(); ++j )
  {
    bool* ptr = &image( 0, j );

    for( unsigned i = 0; i < image.ni(); ++i, ptr+=istep )
    {
      if( *ptr )
      {
        counter++;
      }
    }
  }

  return boost::lexical_cast< double >( counter );
}

void
tweak_region( const vil_image_view<double>& classified,
              vil_image_view<bool>& results,
              double pixel_percent = 0.0 )
{
  double adj = -0.1;

  unsigned pix_count = pixel_percent * classified.ni() * classified.nj();

  if( pix_count > 0.0 && count_pixels( results ) < pix_count )
  {
    for( unsigned i = 0; i < 10; ++i, adj*=2 )
    {
      vil_threshold_above( classified, results, adj );

      if( count_pixels( results ) > pix_count )
      {
        break;
      }
    }
  }
}

}

// Formulates the final output mask given all information acquired
template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::refine_mask( const mask_image_t& input_mask,
               const osd_info_sptr_t input_osd_info,
               std::vector< mask_image_t >& output_masks,
               std::vector< osd_info_sptr_t >& output_osd_info )
{
  // Handle training mode
  if( pixel_training_mode_ )
  {
    this->training_data_extractor_->step( input_osd_info->features );
    output_masks.push_back( input_mask );
    output_osd_info.push_back( input_osd_info );
    return;
  }

  // Handle buffering option
  if( input_osd_info && input_osd_info->properties.break_flag_ )
  {
    frames_since_reset_ = 0;
    output_masks = mask_buffer_;
    output_osd_info = osd_info_buffer_;
    mask_buffer_.clear();
    osd_info_buffer_.clear();
  }

  if( frames_since_reset_ < options_.reset_buffer_length )
  {
    mask_buffer_.push_back( input_mask );
    osd_info_buffer_.push_back( input_osd_info );
  }
  else if( !mask_buffer_.empty() )
  {
    output_masks = mask_buffer_;
    output_osd_info = osd_info_buffer_;
    mask_buffer_.clear();
    osd_info_buffer_.clear();

    for( unsigned i = 0; i < output_masks.size(); ++i )
    {
      output_masks[i].deep_copy( input_mask );
    }

    output_masks.push_back( input_mask );
    output_osd_info.push_back( input_osd_info );
  }
  else
  {
    output_masks.push_back( input_mask );
    output_osd_info.push_back( input_osd_info );
  }

  // Handle mask refinement stages for all output masks
  for( unsigned i = 0; i < output_masks.size(); ++i )
  {
    // Handle OSD action items
    apply_actions( output_osd_info[i], output_masks[i] );

    // Perform opening, closing, and dilation operations
    if( options_.closing_radius > 0 )
    {
      mask_image_t tmp_mask;
      vil_binary_dilate( output_masks[i], tmp_mask, closing_el_ );
      vil_binary_erode( tmp_mask, output_masks[i], closing_el_ );
    }
    if( options_.opening_radius > 0 )
    {
      mask_image_t tmp_mask;
      vil_binary_erode( output_masks[i], tmp_mask, opening_el_ );
      vil_binary_dilate( tmp_mask, output_masks[i], opening_el_ );
    }
    if( options_.dilation_radius > 0 )
    {
      mask_image_t tmp_mask;
      vil_binary_dilate( output_masks[i], tmp_mask, dilation_el_ );
      output_masks[i] = tmp_mask;
    }
  }

  // Increment frame counter
  frames_since_reset_++;
}


template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::flush_masks( std::vector< mask_image_t >& output_masks,
               std::vector< osd_info_sptr_t >& output_osd_info )
{
  output_masks = mask_buffer_;
  output_osd_info = osd_info_buffer_;
  mask_buffer_.clear();
  osd_info_buffer_.clear();
}


template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::apply_actions( const osd_info_sptr_t osd_info,
                 mask_image_t& output_mask )
{
  if( !osd_info || !osd_info->actions )
  {
    return;
  }

  const std::vector< osd_action_item >& recommended_actions = osd_info->actions->actions;
  const source_image_t& input_image = osd_info->source;
  const image_border& input_border = osd_info->border;
  const feature_array_t& input_features = osd_info->features;
  const std::map< std::string, mask_image_t >& input_masks = osd_info->actions->masks;
  const std::map< std::string, pixel_classifier_t >& input_clfrs = osd_info->actions->clfrs;

  image_region center_region( 0, input_image.ni(), 0, input_image.nj() );

  if( input_border.area() > 0 )
  {
    center_region = image_region( input_border.min_x(),
                                  input_border.max_x(),
                                  input_border.min_y(),
                                  input_border.max_y() );
  }


#define retrieve_model( model_map, model_name ) \
  model_map .find( model_name )->second

  for( unsigned i = 0; i < recommended_actions.size(); ++i )
  {
    const osd_action_item& todo = recommended_actions[i];

    if( todo.node_id_ != options_.refiner_index && !pixel_training_mode_ )
    {
      continue;
    }

    const image_region scaled_region = template_to_image_region( todo.rect_,
                                                                 input_image.ni(),
                                                                 input_image.nj() );

    // Ignore blinker actions if blinking component is not present
    if( todo.blinker_ )
    {
      const image_region blinker_region = template_to_image_region( todo.blinker_region_,
                                                                    input_image.ni(),
                                                                    input_image.nj() );

      if( !check_blinker_component( blinker_region,
                                    todo.blinker_per_threshold_,
                                    output_mask ) )
      {
        continue;
      }
    }

    // Perform reccommended action
    switch( todo.action_ )
    {
      case osd_action_item::FILL_RECTANGLE:
      {
        if( !options_.apply_fills )
        {
          break;
        }

        vil_image_view<bool> region_ptr = point_view_to_region( output_mask, scaled_region );
        region_ptr.fill( true );
        break;
      }

      case osd_action_item::UNFILL_RECTANGLE:
      {
        vil_image_view<bool> region_ptr = point_view_to_region( output_mask, scaled_region );
        region_ptr.fill( false );
        break;
      }

      case osd_action_item::CLASSIFY_REGION:
      {
#ifdef USE_CAFFE
        if( todo.key_.substr( todo.key_.find_last_of( "." ) + 1 ) == "cnn" )
        {
          mask_image_t classification;

          source_image_t input_region = point_view_to_region( osd_info->source, scaled_region );
          mask_image_t output_region = point_view_to_region( output_mask, scaled_region );

          const_cast< cnn_detector< PixType >& >(
            retrieve_model( osd_info->actions->cnn_clfrs, todo.key_ ) )
              .detect( input_region, classification );

          union_region( classification, output_region );
          break;
        }
#endif

        classify_and_union_region( input_features,
                                   scaled_region,
                                   retrieve_model( input_clfrs, todo.key_ ),
                                   output_mask,
                                   todo.param3_, i,
                                   todo.param1_, todo.param2_, todo.param4_ );
        break;
      }

      case osd_action_item::RECLASSIFY_IMAGE:
      {
        vil_image_view<bool> output_ptr = point_view_to_region( output_mask, center_region );

        classify_region( input_features,
                         center_region,
                         retrieve_model( input_clfrs, todo.key_ ),
                         output_ptr );
        break;
      }

      case osd_action_item::RECLASSIFY_REGION:
      {
        reclassify_region( input_features,
                           scaled_region,
                           retrieve_model( input_clfrs, todo.key_ ),
                           output_mask,
                           todo.param3_, i,
                           todo.param1_, todo.param2_, todo.param4_ );
        break;
      }

      case osd_action_item::UNION_MASK:
      {
        vil_transform( output_mask,
                       retrieve_model( input_masks, todo.key_ ),
                       output_mask,
                       union_functor );
        break;
      }

      case osd_action_item::INTERSECT_MASK:
      {
        vil_transform( output_mask,
                       retrieve_model( input_masks, todo.key_ ),
                       output_mask,
                       intersection_functor );
        break;
      }

      case osd_action_item::USE_MASK:
      {
        output_mask.deep_copy( retrieve_model( input_masks, todo.key_ ) );
        break;
      }

      case osd_action_item::XHAIR_SELECT:
      {
        xhair_selection( input_features,
                         scaled_region,
                         retrieve_model( input_clfrs, todo.key_ ),
                         retrieve_model( input_clfrs, todo.key2_ ),
                         output_mask );
        break;
      }

      case osd_action_item::ERODE:
      {
        apply_morph_to_region( scaled_region,
                               todo.param1_,
                               false,
                               output_mask );
        break;
      }

      case osd_action_item::DILATE:
      {
        apply_morph_to_region( scaled_region,
                               todo.param1_,
                               true,
                               output_mask );
        break;
      }

      case osd_action_item::USE_TEMPLATE_MATCHING:
      case osd_action_item::PARSE_TEXT:
      {
        // Do nothing, these are handled by other external processes.
        break;
      }

      default:
      {
        LOG_WARN( "Received unknown mask action: " << static_cast<int>( todo.action_ ) );
        break;
      }
    }
  }
}


// Classify a region in the image according to some classifier and the input features
template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::classify_region( const feature_array_t& features,
                   const image_region& region,
                   const pixel_classifier_t& clfr,
                   mask_image_t& output )
{
  vil_image_view<double> classified;
  classify_region( features, region, clfr, classified );
  output.set_size( classified.ni(), classified.nj() );
  vil_threshold_above( classified, output, 0.0 );
}

template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::classify_region( const feature_array_t& features,
                   const image_region& region,
                   const pixel_classifier_t& clfr,
                   vil_image_view< double >& output,
                   const double additional_offset )
{
  LOG_ASSERT( features.size() > 0, "Action requires input features" );

  feature_array_t input_array;
  for( unsigned i = 0; i < features.size(); ++i )
  {
    input_array.push_back( point_view_to_region( features[i], region ) );
  }

  clfr.classify_images( input_array, output, options_.clfr_adjustment + additional_offset );
}

// Reclassify a region, overriding any results currently in the output mask
template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::classify_region_helper( const feature_array_t& features,
                          const image_region& region,
                          const pixel_classifier_t& clfr,
                          mask_image_t& output,
                          const unsigned spec_option,
                          const unsigned command_id,
                          const double parameter1,
                          const double parameter2,
                          const double parameter3 )
{
  // Compute classifier threshold / offset
  double classifier_offset = 0.0;
  double classifier_thresh = 0.0;

  if( spec_option == 4 )
  {
    double fraction = 0.0;

    if( local_averages_.find( command_id ) != local_averages_.end() )
    {
      fraction = static_cast<double>( local_averages_[command_id].frame_count() ) / parameter1;
    }

    classifier_offset = parameter2 * fraction;
    classifier_thresh = classifier_offset - ( fraction < 1.0 ? parameter3 : 0.0 );
  }
  else if( spec_option == 7 )
  {
    classifier_thresh = parameter1;
  }

  // Generate initial heatmap
  vil_image_view<double> classified, average;
  vil_image_view<bool> avg_mask;
  classify_region( features, region, clfr, classified, classifier_offset );

  // Perform initial cut of thresholding
  output.set_size( classified.ni(), classified.nj() );
  vil_threshold_above( classified, output, classifier_thresh );

  // Perform averaging of output map
  if( spec_option >= 4 && spec_option < 7 )
  {
    unsigned window = static_cast<unsigned>( spec_option == 4 ? parameter1 : parameter3 );

    if( local_averages_.find( command_id ) == local_averages_.end() )
    {
      local_averages_[command_id] = frame_averager_t( false, window );
    }

    local_averages_[command_id].process_frame( classified, average );

    vil_threshold_above( average, avg_mask, 0.0 );

    vil_transform( output, avg_mask, output,
                   union_functor );
  }

  // Handle horizontal sliding bar option
  if( spec_option == 2 || spec_option == 5 )
  {
    std::vector< double > max_col_clf( classified.ni(), 0.0 );
    vil_image_view<double> heatmap = ( average.ni() > 0 ? average : classified );

    for( unsigned i = 0; i < heatmap.ni(); ++i )
    {
      double sum = 0.0;

      for( unsigned j = 0; j < heatmap.nj(); ++j )
      {
        sum += heatmap( i, j );
      }

      max_col_clf[i] = sum;
    }

    const unsigned scan_size = static_cast<unsigned>( parameter1 * features[0].ni() );
    unsigned top_position = 0;
    double top_sum = -1000;

    if( scan_size < heatmap.ni() )
    {
      for( unsigned i = 0; i < heatmap.ni() - scan_size; ++i )
      {
        double sum = 0.0;

        for( unsigned j = 0; j < scan_size; ++j )
        {
          sum += max_col_clf[j];
        }

        if( sum > top_sum )
        {
          top_position = i;
          top_sum = sum;
        }
      }

      image_region subregion_loc( top_position, top_position + scan_size, 0, heatmap.nj() );
      mask_image_t subregion = point_view_to_region( output, subregion_loc );
      subregion.fill( true );
    }
  }

  // Handle vertical bar special case
  if( spec_option == 3 || spec_option == 6 )
  {
    std::vector< double > max_row_clf( classified.nj(), 0.0 );
    vil_image_view<double> heatmap = ( average.ni() > 0 ? average : classified );

    for( unsigned j = 0; j < heatmap.nj(); ++j )
    {
      double sum = 0.0;

      for( unsigned i = 0; i < heatmap.ni(); ++i )
      {
        sum += heatmap( i, j );
      }

      max_row_clf[j] = sum;
    }

    const unsigned scan_size = static_cast<unsigned>( parameter1 * features[0].nj() );
    unsigned top_position = 0;
    double top_sum = -1000;

    if( scan_size < heatmap.nj() )
    {
      for( unsigned j = 0; j < heatmap.nj() - scan_size; ++j )
      {
        double sum = 0.0;

        for( unsigned i = 0; i < scan_size; ++i )
        {
          sum += max_row_clf[i];
        }

        if( sum > top_sum )
        {
          top_position = j;
          top_sum = sum;
        }
      }

      image_region subregion_loc( 0, heatmap.ni(), top_position, top_position + scan_size );
      mask_image_t subregion = point_view_to_region( output, subregion_loc );
      subregion.fill( true );
    }
  }

  // Perform NMS if enabled
  if( spec_option == 1 )
  {
    filter_lone_pixels( output );
  }
}


// Reclassify a region, overriding any results currently in the output mask
template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::reclassify_region( const feature_array_t& features,
                     const image_region& region,
                     const pixel_classifier_t& clfr,
                     mask_image_t& output,
                     const unsigned spec_option,
                     const unsigned command_id,
                     const double parameter1,
                     const double parameter2,
                     const double parameter3 )
{
  mask_image_t region_mask;
  classify_region_helper( features, region, clfr, region_mask, spec_option,
                          command_id, parameter1, parameter2, parameter3 );

  mask_image_t output_region = point_view_to_region( output, region );
  vil_copy_reformat( region_mask, output_region );
}


template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::union_region( const mask_image_t& mask1,
                mask_image_t& mask2 )
{
  const std::ptrdiff_t istep = mask1.istep();

  for( unsigned j = 0; j < mask2.nj(); ++j )
  {
    const bool* ptr = &mask1( 0, j );

    for( unsigned i = 0; i < mask2.ni(); ++i, ptr+=istep )
    {
      if( *ptr )
      {
        mask2( i, j ) = true;
      }
    }
  }
}

template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::classify_and_union_region( const feature_array_t& features,
                             const image_region& region,
                             const pixel_classifier_t& clfr,
                             mask_image_t& output,
                             unsigned spec_option,
                             unsigned command_id,
                             double parameter1,
                             double parameter2,
                             double parameter3  )
{
  mask_image_t region_mask;
  classify_region_helper( features, region, clfr, region_mask, spec_option,
                          command_id, parameter1, parameter2, parameter3 );

  vil_image_view<bool> output_region = point_view_to_region( output, region );
  union_region( region_mask, output_region );
}


// Apply morphology to some region in the output mask
template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::apply_morph_to_region( const image_region& region,
                         const double kernel_size,
                         const bool dilate,
                         mask_image_t& output )
{
  vil_image_view< bool > output_ptr = point_view_to_region( output, region );
  morphology_buffer_.deep_copy( output_ptr );
  vil_structuring_element dilation_el;
  dilation_el.set_to_disk( kernel_size );

  if( dilate )
  {
    vil_binary_dilate( morphology_buffer_, output_ptr, dilation_el );
  }
  else
  {
    vil_binary_erode( morphology_buffer_, output_ptr, dilation_el );
  }
}


// Reclassify a region, and determine which of two classifiers fits it better
template <typename PixType, typename FeatureType>
void
osd_mask_refiner<PixType,FeatureType>
::xhair_selection( const feature_array_t& features,
                   const image_region& region,
                   const pixel_classifier_t& clfr1,
                   const pixel_classifier_t& clfr2,
                   mask_image_t& output )
{
  // Classify according to our 2 classifiers
  vil_image_view<bool> results1, results2;
  classify_region( features, region, clfr1, results1 );
  classify_region( features, region, clfr2, results2 );

  // Decide which one (if either) looks more like a crosshair
  unsigned sum1, sum2;
  vil_math_sum( sum1, results1, 0 );
  vil_math_sum( sum2, results2, 0 );

  // Fill in detected components
  vil_image_view<bool> *to_fill = NULL;
  if( sum1 > sum2 )
  {
    to_fill = &results1;
  }
  else
  {
    to_fill = &results2;
  }

  vil_image_view<bool> output_region = point_view_to_region( output, region );
  output_region.deep_copy( *to_fill );
}


// Check if we think some blinking component specified by the template is active
template <typename PixType, typename FeatureType>
bool
osd_mask_refiner<PixType,FeatureType>
::check_blinker_component( const image_region& region,
                           const double per_thresh,
                           const mask_image_t& output )
{
  // Get number of pixels in region
  vil_image_view<bool> region_ptr = point_view_to_region( output, region );

  if( region_ptr.size() == 0 )
  {
    return false;
  }

  // Threshold percent true count
  unsigned count = 0;
  vil_math_sum( count, region_ptr, 0 );

  // Are there at least a small number of pixels (>20%) active in this region?
  if( static_cast<double>( count ) > per_thresh * region_ptr.size() )
  {
    return true;
  }
  return false;
}

} // end namespace vidtk
