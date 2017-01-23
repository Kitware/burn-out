/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "osd_recognizer.h"

#include <utilities/point_view_to_region.h>

#include <video_transforms/automatic_white_balancing.h>

#include <learning/learner_data_class_simple_wrapper.h>

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


VIDTK_LOGGER( "osd_recognizer" );


namespace vidtk
{


template <typename PixType, typename FeatureType>
bool
osd_recognizer<PixType,FeatureType>
::configure( const osd_recognizer_settings& options )
{
  // Load internal settings
  options_ = options;

  if( options.use_templates )
  {
    if( !read_templates_file( options.template_filename, templates_ ) )
    {
      LOG_ERROR( "Unable to parse template file!" );
      return false;
    }
  }

  if( options.static_dilation_amount != 0.0 )
  {
    static_dilation_el_.set_to_disk( options.static_dilation_amount );
  }
  if( options.full_dilation_amount != 0.0 )
  {
    approximation_dilation_el_.set_to_disk( options.full_dilation_amount );
  }

  // Reset text parsing flag (will be set later if any templates are loaded
  // containing text parsing commands)
  contains_text_option_ = false;
  last_template_ = -1;

  // Set the classifier folder path
  boost::filesystem::path p( options.template_filename );
  template_path_ = p.parent_path().string();

  // Read all classifiers and masks in the template file action fields
  for( unsigned i = 0; i < templates_.size(); ++i )
  {
    // Generate a new model class for this template
    osd_action_items_sptr_t new_actions( new osd_action_items_t() );
    actions_.push_back( new_actions );

    // Handle any template classifiers
    const std::vector< std::string >& template_classifier_ts = templates_[i].get_classifiers();
    for( unsigned j = 0; j < template_classifier_ts.size(); ++j )
    {
      std::string key = template_classifier_ts[j];

      if( key.size() > 0 )
      {
        std::string classifier_filename = template_path_ + "/" + key;
        template_classifiers_[key] = template_classifier_t();
        if( !template_classifiers_[key].read_from_file( classifier_filename ) )
        {
          LOG_ERROR( "Could not load " << classifier_filename );
          return false;
        }
      }
    }

    // Handle actions stored in the template
    bool found_template_parse_id = false;
    std::vector< osd_action_item > actions = templates_[i].get_actions();
    new_actions->actions = actions;

    for( unsigned j = 0; j < actions.size(); ++j )
    {
      const osd_action_item& action = actions[j];
      std::string key = action.key_;
      std::string key2 = action.key2_;

      if( action.action_ == osd_action_item::USE_TEMPLATE_MATCHING ||
          action.action_ == osd_action_item::PARSE_TEXT )
      {
        new_actions->template_matching = true;
      }

      if( action.action_ == osd_action_item::USE_TEMPLATE_MATCHING )
      {
        if( found_template_parse_id )
        {
          LOG_ERROR( "Template file cannot contain 2 template matcher strings" );
          return false;
        }

        found_template_parse_id = true;

        if( !key.empty() && key != "DISABLE" && key != "DISABLED" )
        {
          new_actions->dyn_instruction_1 = template_path_ + "/" + key;
        }
        else
        {
          new_actions->dyn_instruction_1 = "DISABLE";
        }

        if( !key2.empty() && key2 != "DISABLE" && key2 != "DISABLED" )
        {
          new_actions->dyn_instruction_2 = template_path_ + "/" + key2;
        }
        else
        {
          new_actions->dyn_instruction_2 = "DISABLE";
        }
      }
      else if( key.size() > 0 )
      {
        if( action.action_ == osd_action_item::UNION_MASK ||
            action.action_ == osd_action_item::INTERSECT_MASK ||
            action.action_ == osd_action_item::USE_MASK )
        {
          // Load image
          std::string classifier_filename = template_path_ + "/" + key;
          vil_image_view<vxl_byte> loaded_img;
          loaded_img = vil_load( classifier_filename.c_str() );
          if( !loaded_img )
          {
            LOG_ERROR( "Could not load " << classifier_filename );
            return false;
          }
          vil_image_view<bool> mask;
          vil_threshold_above( loaded_img, mask, static_cast<vxl_byte>(1) );
          new_actions->masks[key] = mask;
        }
        else if( action.action_ == osd_action_item::PARSE_TEXT )
        {
          text_instruction_t instruction;
          contains_text_option_ = true;

          if( key2 == "GSD" )
          {
            instruction.type = text_instruction_t::GROUND_SAMPLE_DISTANCE;
          }
          else if( key2 == "FOV_LEVEL" || key2 == "FOV" )
          {
            instruction.type = text_instruction_t::FIELD_OF_VIEW_LEVEL;
          }
          else if( key2 == "TARGET_WIDTH" || key2 == "TWD" )
          {
            instruction.type = text_instruction_t::TARGET_WIDTH;
          }

          if( action.param1_ > 0.5 )
          {
            instruction.select_top = true;
          }

          instruction.region = action.rect_;

          boost::shared_ptr< text_parser_model_group< PixType > > models(
            new text_parser_model_group< PixType >() );

          if( key != "DEFAULT" && !models->load_templates( template_path_ + "/" + key ) )
          {
            return false;
          }

          instruction.models = models;
          new_actions->parse_instructions.push_back( instruction );
        }
#ifdef USE_CAFFE
        else if( key.substr( key.find_last_of( "." ) + 1 ) == "cnn" )
        {
          std::string cnn_options_filename = template_path_ + "/" + key;
          cnn_detector_settings cnn_options;

          if( !cnn_options.load_config_from_file( cnn_options_filename ) )
          {
            LOG_ERROR( "Unable to load cnn settings: " << cnn_options_filename );
            return false;
          }

          if( options.use_gpu_override != osd_recognizer_settings::NO_OVERRIDE )
          {
            switch( options.use_gpu_override )
            {
              case osd_recognizer_settings::YES:
                cnn_options.use_gpu = cnn_detector_settings::YES;
                break;
              case osd_recognizer_settings::NO:
                cnn_options.use_gpu = cnn_detector_settings::NO;
                break;
              default:
                cnn_options.use_gpu = cnn_detector_settings::AUTO;
                break;
            }
          }

          if( !new_actions->cnn_clfrs[ action.key_ ].configure( cnn_options ) )
          {
            return false;
          }
        }
#endif
        else
        {
          // Load classifier
          std::string classifier_filename = template_path_ + "/" + key;
          if( !new_actions->clfrs[ action.key_ ].load_from_file( classifier_filename ) )
          {
            LOG_ERROR( "Unable to load " << classifier_filename );
            return false;
          }

          // Check for a secondary classifier
          if( key2.size() > 0 )
          {
            classifier_filename = template_path_ + "/" + key2;
            if( !new_actions->clfrs[ key2 ].load_from_file( classifier_filename ) )
            {
              LOG_ERROR( "Unable to load " << classifier_filename );
              return false;
            }
          }
        }
      }
    }

    if( !found_template_parse_id )
    {
      new_actions->dyn_instruction_1 = "DISABLE";
      new_actions->dyn_instruction_2 = "DISABLE";
    }
  }

  return true;
}


// Helper functions for unioning/intersecting masks
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


template <typename PixType, typename FeatureType>
boost::shared_ptr< osd_recognizer_output< PixType, FeatureType > >
osd_recognizer<PixType,FeatureType>
::process_frame( const source_image_t& input_image,
                 const feature_array_t& input_features,
                 const image_border& input_border,
                 const classified_image_t& input_detections,
                 const properties_t& input_properties,
                 mask_image_t& output_mask )

{
  // Perform Basic Validation
  LOG_ASSERT( input_detections.nplanes() <= 2, "Classifier input has wrong number of channels!" );
  LOG_ASSERT( input_features.size() > 0, "No input features provided!" );
  LOG_ASSERT( input_features[0].ni() == input_image.ni(), "Input widths do not match." );
  LOG_ASSERT( input_features[0].nj() == input_image.nj(), "Input heights do not match." );

  // Default initialize output
  osd_recognizer_output_sptr_t output;

  // Reset output mask
  output_mask.set_size( input_image.ni(), input_image.nj() );

  // Threshold input channels
  vil_threshold_above( vil_plane( input_detections, 0 ),
                       thresholded_static_image_,
                       options_.classifier_threshold );

  if( options_.fill_color_extremes )
  {
    fill_color_extremes( input_image,
                         thresholded_static_image_,
                         input_properties );
  }

  if( options_.use_initial_approximation )
  {
    vil_threshold_above( vil_plane( input_detections, ( input_detections.nplanes() == 2 ? 1 : 0 ) ),
                         thresholded_full_image_,
                         options_.classifier_threshold );
  }

  // Perform any simple filtering
  if( options_.filter_lone_pixels )
  {
    this->filter_lone_pixels( thresholded_static_image_ );

    if( options_.use_initial_approximation )
    {
      this->filter_lone_pixels( thresholded_full_image_ );
    }
  }

  // Dilate static detections into output_mask_
  if( options_.static_dilation_amount != 0 )
  {
    vil_binary_dilate( thresholded_static_image_, output_mask, static_dilation_el_ );
  }
  else
  {
    output_mask.deep_copy( thresholded_static_image_ );
  }

  // Dilate initial approximation mask
  if( options_.use_initial_approximation )
  {
    morphed_full_image_.set_size( output_mask.ni(), output_mask.nj() );

    if( options_.full_dilation_amount != 0 )
    {
      vil_binary_dilate( thresholded_full_image_, morphed_full_image_, approximation_dilation_el_ );
    }

    vil_transform( output_mask, morphed_full_image_, output_mask, union_functor );
  }

  // Perform connected components on statics, and use them to determine if this
  // mask fits into one of our known (loaded) templates
  int template_id = -1;

  if( options_.use_templates )
  {
    template_id = this->select_template( output_mask, input_properties );

    if( template_id == -1 )
    {
      LOG_INFO( "No known mask detected." );
    }
    else
    {
      LOG_INFO( "Detected Mask: " << templates_[template_id].get_name() );

      output.reset( new osd_recognizer_output_t() );

      output->type = templates_[ template_id ].get_name();
      output->actions = actions_[ template_id ];
      output->source = input_image;
      output->border = input_border;
      output->features = input_features;
      output->classifications = input_detections;
      output->properties = input_properties;
    }
  }

  // Finally, fill in border if known
  if( input_border.area() > 0 )
  {
    for( int i = 0; i < input_border.min_x(); ++i )
    {
      vil_fill_col( output_mask, i, true );
    }
    for( int i = input_border.max_x(); i < static_cast<int>(input_image.ni()); ++i )
    {
      vil_fill_col( output_mask, i, true );
    }
    for( int j = 0; j < input_border.min_y(); ++j )
    {
      vil_fill_row( output_mask, j, true );
    }
    for( int j = input_border.max_y(); j < static_cast<int>(input_image.nj()); ++j )
    {
      vil_fill_row( output_mask, j, true );
    }
  }

  return output;
}


// Returns the index from the template vector for the most likely template
// and -1 if the mask does not seem to be one of our known templates
template <typename PixType, typename FeatureType>
int
osd_recognizer<PixType,FeatureType>
::select_template( const mask_image_t& components,
                   const properties_t& props )
{
  // Step 1. Perform connected components
  std::vector< vil_blob_pixel_list > blobs;
  vil_blob_labels( components, vil_blob_8_conn, labeled_blobs_ );
  vil_blob_labels_to_pixel_lists( labeled_blobs_, blobs );

  // Step 2. Build a simple manual template for the OSD
  osd_template static_template;

  const PixType max_value_int = default_white_point<PixType>::value();
  const double max_value = static_cast<double>( max_value_int );

  vil_rgb<double> color;
  color.r = props.color_.r / max_value;
  color.g = props.color_.g / max_value;
  color.b = props.color_.b / max_value;
  static_template.add_color( color );

  const unsigned ni = components.ni();
  const unsigned nj = components.nj();

  static_template.add_resolution( ni, nj );

  for( unsigned i = 0; i < blobs.size(); ++i )
  {
    if( blobs[i].size() < 15 ) // ignore small blobs (likely noise)
    {
      continue;
    }

    osd_symbol sym;

    if( convert_blob_to_symbol( ni, nj, blobs[i], sym ) )
    {
      static_template.add_feature( sym );
    }
  }

  // Step 3. Generate summary descriptor if required
  std::vector< double > descriptor;

  if( !template_classifiers_.empty() || !options_.descriptor_output_file.empty() )
  {
    this->generate_descriptor( ni, nj, static_template.get_symbols(), descriptor );
  }

  classifier_data_wrapper wrapper( &descriptor[0], descriptor.size() );

  // Step 4. Select best matching template from list if it exists
  int best_id = -1;
  double best_weight = options_.template_threshold;
  bool classifier_match = false;

  for( unsigned i = 0; i < templates_.size(); ++i )
  {
    // Check if force flag is set on the individual template
    if( templates_[i].force_flag_set() )
    {
      best_id = i;
      break;
    }

    // Skip templates where the resolution doesn't match
    if( options_.use_resolution && !templates_[i].check_resolution( ni, nj ) )
    {
      continue;
    }

    // Special case checking for pure green, red. and blue OSD displays
    if( templates_[i].get_name() == "GREEN_OSD" &&
        props.color_.r == 0 && props.color_.g == max_value_int && props.color_.b == 0 )
    {
      best_id = i;
      break;
    }
    if( templates_[i].get_name() == "BLUE_OSD" &&
        props.color_.r == 0 && props.color_.g == 0 && props.color_.b == max_value_int )
    {
      best_id = i;
      break;
    }
    if( templates_[i].get_name() == "RED_OSD" &&
        props.color_.r == max_value_int && props.color_.g == 0 && props.color_.b == 0 )
    {
      best_id = i;
      break;
    }

    // Always prefer matches from the classifier over possible symbology matches
    if( templates_[i].has_classifier() )
    {
      const std::vector< std::string >& clfr_ids = templates_[i].get_classifiers();

      for( unsigned j = 0; j < clfr_ids.size(); ++j )
      {
        double weight = template_classifiers_[ clfr_ids[j] ].classify_raw( wrapper );

        if( weight > best_weight ||
            ( weight > options_.template_threshold && !classifier_match ) )
        {
          classifier_match = true;
          best_weight = weight;
          best_id = i;
        }
      }
    }
    else if( !classifier_match )
    {
      double weight = templates_[i].possibly_contains( static_template );

      if( weight >= options_.template_threshold && weight > best_weight )
      {
        best_weight = weight;
        best_id = i;
      }
    }
  }

  // Set last_template variable for short-term smoothing
  if( best_id != -1 )
  {
    last_template_ = best_id;
  }

  if( best_id == -1 && last_template_ != -1 )
  {
    best_id = last_template_;
  }

  // Optional long term smoothing, select best known OSD template over time
  if( options_.use_most_common_template )
  {
    template_hist_[best_id]++;

    best_id = -1;
    int best_score = 0;

    for( std::map< int, int >::iterator p = template_hist_.begin();
         p != template_hist_.end(); p++ )
    {
      if( p->second > best_score )
      {
        best_id = p->first;
        best_score = p->second;
      }
    }
  }

  // Return identifier of the best OSD type
  return best_id;
}


// Convert a size-normalized region stored in a template into image coordinates
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


// Fill in lone pixels or pixels with just 1 neighbor [unsafe]
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
template <typename PixType, typename FeatureType>
void
osd_recognizer<PixType,FeatureType>
::filter_lone_pixels( mask_image_t& image )
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


template <typename PixType, typename FeatureType>
void
osd_recognizer<PixType,FeatureType>
::fill_color_extremes( const source_image_t& input_image,
                       mask_image_t& output_mask,
                       const properties_t& input_properties )
{
  const unsigned ni = input_image.ni();
  const unsigned nj = input_image.nj();
  const unsigned np = input_image.nplanes();

  if( np != 3 )
  {
    return;
  }

  const PixType high_value = std::numeric_limits< PixType >::max();

  const PixType allowed_variance = 60;
  const PixType high_thresh = high_value - allowed_variance;
  const PixType low_thresh = high_value - allowed_variance;

  const bool is_green = ( input_properties.color_.r == 0 &&
                          input_properties.color_.g == high_value &&
                          input_properties.color_.b == 0 );
  const bool is_blue = ( input_properties.color_.r == 0 &&
                         input_properties.color_.g == 0 &&
                         input_properties.color_.b == high_value );

  const std::ptrdiff_t istep = input_image.istep();
  const std::ptrdiff_t jstep = input_image.jstep();
  const std::ptrdiff_t pstep = input_image.planestep();
  const std::ptrdiff_t p2step = 2 * pstep;

  const PixType* row = input_image.top_left_ptr();

  if( is_green )
  {
    for( unsigned j = 0; j < nj; ++j, row += jstep )
    {
      const PixType* pixel = row;

      for( unsigned i = 0; i < ni; ++i, pixel += istep )
      {
        if( *pixel < low_thresh &&
            *(pixel+pstep) > high_thresh &&
            *(pixel+p2step) < low_thresh )
        {
          output_mask( i, j ) = true;
        }
      }
    }
  }

  if( is_blue )
  {
    for( unsigned j = 0; j < nj; ++j, row += jstep )
    {
      const PixType* pixel = row;

      for( unsigned i = 0; i < ni; ++i, pixel += istep )
      {
        if( *pixel < low_thresh &&
            *(pixel+pstep) < low_thresh &&
            *(pixel+p2step) > high_thresh )
        {
          output_mask( i, j ) = true;
        }
      }
    }
  }
}


// Generate an obstruction template type descriptor
template <typename PixType, typename FeatureType>
void
osd_recognizer<PixType,FeatureType>
::generate_descriptor( const unsigned ni, const unsigned nj,
                       const std::vector< osd_symbol >& symbols,
                       std::vector< double >& descriptor )
{
  const unsigned width_bins = options_.descriptor_width_bins;
  const unsigned height_bins = options_.descriptor_height_bins;
  const unsigned area_bins = options_.descriptor_area_bins;
  const unsigned descriptor_size = width_bins * height_bins * area_bins;

  // Check if we have already pre-computed our area delimiters for this
  // image resolution
  if( size_intervals_.empty() || size_intervals_.back() != ni * nj )
  {
    size_intervals_.resize( area_bins );
    double bin_increment = 0.01 / area_bins;

    for( unsigned i = 0; i < area_bins - 1; ++i )
    {
      size_intervals_[ i ] = i * bin_increment;
    }

    size_intervals_[ area_bins - 1 ] = static_cast< double >( ni * nj );
  }

  // Formulate summary descriptor
  descriptor.clear();
  descriptor.resize( descriptor_size, 0.0 );

  for( unsigned i = 0; i < symbols.size(); ++i )
  {
    const vgl_box_2d< double > region = symbols[i].region_;

    double cx = std::max( std::min( region.centroid_x(), 0.999 ), 0.000 );
    double cy = std::max( std::min( region.centroid_y(), 0.999 ), 0.000 );

    double nsize = region.area();

    unsigned region_bin = static_cast< unsigned >( cx * width_bins ) +
                          static_cast< unsigned >( cy * height_bins ) * width_bins;

    unsigned area_bin = 0;

    for( unsigned j = 0; j < area_bins; ++j )
    {
      if( nsize > size_intervals_[j] )
      {
        area_bin = j;
      }
    }

    descriptor[ region_bin * area_bins + area_bin ] += 1.0;
  }

  // Output descriptor to file if option set
  if( !options_.descriptor_output_file.empty() )
  {
    std::ofstream fout( options_.descriptor_output_file.c_str(), std::ios::app );

    for( unsigned i = 0; i < descriptor.size(); i++ )
    {
      fout << descriptor[i] << " ";
    }

    fout << std::endl;
    fout.close();
  }
}


// Return loaded templates
template <typename PixType, typename FeatureType>
std::vector< osd_template > const&
osd_recognizer<PixType,FeatureType>
::get_templates() const
{
  return templates_;
}


template <typename PixType, typename FeatureType>
bool
osd_recognizer<PixType,FeatureType>
::contains_text_option() const
{
  return contains_text_option_;
}


} // end namespace vidtk
