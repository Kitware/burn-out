/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "text_parser.h"

#include <utilities/point_view_to_region.h>
#include <utilities/folder_manipulation.h>

#ifdef USE_TESSERACT
  #include <tesseract/baseapi.h>
  #include <tesseract/strngs.h>
#endif

#ifdef USE_OPENCV
  #include <utilities/vxl_to_cv_converters.h>

  #include <opencv2/core/core.hpp>
  #include <opencv2/highgui/highgui.hpp>
  #include <opencv2/imgproc/imgproc.hpp>
#endif

#include <video_transforms/ssd.h>
#include <video_transforms/invert_image_values.h>

#include <vil/vil_save.h>
#include <vil/vil_load.h>
#include <vil/vil_math.h>
#include <vil/vil_copy.h>
#include <vil/vil_convert.h>
#include <vil/vil_resample_bilin.h>

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <string>
#include <limits>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "text_parser" );


template< typename PixType >
bool
text_parser_model_group< PixType >
::load_templates( const std::string& directory )
{
  std::vector< std::string > files;
  list_files_in_folder( directory, files );

  unsigned last_ni = 0, last_nj = 0, last_np = 0;

  for( unsigned i = 0; i < files.size(); ++i )
  {
    image_t individual_template = vil_load( files[ i ].c_str() );
    std::string::size_type ext_pos = files[ i ].find_last_of( "." );

    if( individual_template.size() == 0 ||
        ext_pos == std::string::npos ||
        ext_pos == 0 )
    {
      LOG_ERROR( "Invalid character template: " << files[ i ] );
      return false;
    }

    if( templates.empty() )
    {
      last_ni = individual_template.ni();
      last_nj = individual_template.nj();
      last_np = individual_template.nplanes();
    }
    else if( individual_template.nplanes() != last_np )
    {
      LOG_ERROR( "Individual template plane counts do not match" );
      return false;
    }
    else if( individual_template.ni() != last_ni ||
             individual_template.nj() != last_nj )
    {
      // We can still function with non-equal templates, just at a slightly
      // lower performance level.
      LOG_WARN( "Individual template dimensions do not match" );
    }

    boost::filesystem::path p( files[ i ] );
    std::string fn_wo_ext = p.stem().string();
    templates.push_back( std::make_pair( fn_wo_ext, individual_template ) );
  }

  if( files.empty() )
  {
    LOG_ERROR( "No character templates found in " << directory );
    return false;
  }

  return !files.empty();
}


template< typename PixType >
class
text_parser< PixType >
::external_data
{
public:

  external_data() {}
  ~external_data() {}

#ifdef USE_TESSERACT
  tesseract::TessBaseAPI tess_parser;
#endif

};


template< typename PixType >
text_parser< PixType >
::text_parser()
{
}


template< typename PixType >
text_parser< PixType >
::~text_parser()
{
}


template< typename PixType >
bool
text_parser< PixType >
::configure( const text_parser_settings& settings )
{
  settings_ = settings;

#ifdef USE_TESSERACT
  if( settings_.algorithm == text_parser_settings::TESSERACT_OCR )
  {
    ext_.reset( new external_data() );

    ext_->tess_parser.Init( settings_.model_directory.c_str(),
                            settings_.language.c_str(),
                            tesseract::OEM_DEFAULT );

    ext_->tess_parser.SetPageSegMode( tesseract::PSM_SINGLE_LINE );
  }
#else
  if( settings_.algorithm == text_parser_settings::TESSERACT_OCR )
  {
    LOG_ERROR( "Cannot use tesseract option (VidTK not built with tesseract)" );
    return false;
  }
#endif

  if( !settings.model_directory.empty() )
  {
    return default_models_.load_templates( settings.model_directory );
  }

  return true;
}


// Intermediate structure used for template matching step
struct position_result
{
  position_result()
   : position(0),
     template_id(0),
     weight(std::numeric_limits<double>::max()) {}

  ~position_result() {}

  // Detection column value
  unsigned position;

  // Template identifier
  unsigned template_id;

  // Detection magnitude
  double weight;
};


bool
sort_by_position( const position_result& r1, const position_result& r2 )
{
  return ( r1.position < r2.position );
}


bool
sort_by_weight( const position_result& r1, const position_result& r2 )
{
  return ( r1.weight > r2.weight );
}


bool
is_in_bounds( const position_result& result,
              const unsigned lower_bound,
              const unsigned upper_bound )
{
  return ( result.position >= lower_bound && result.position <= upper_bound );
}


template< typename PixType >
void
format_region( const vil_image_view< PixType >& roi,
               vil_image_view< PixType >& scaled_roi,
               const double scale_factor,
               const bool invert_pixel_values )
{
  if( scale_factor != 1.0 )
  {
    vil_resample_bilin( roi, scaled_roi,
                        roi.ni() * scale_factor,
                        roi.nj() * scale_factor );

    if( invert_pixel_values )
    {
      invert_image( scaled_roi );
    }
  }
  else if( invert_pixel_values )
  {
    vil_copy_deep( roi, scaled_roi );
    invert_image( scaled_roi );
  }
  else
  {
    scaled_roi = roi;
  }
}


template< typename PixType >
std::string
text_parser< PixType >
::parse_text( const image_t& image,
              const instruction_t& instruction )
{
  std::string parsed_string;

  if( !image )
  {
    LOG_WARN( "Text parser received empty image" );
    return parsed_string;
  }

  // Extract image region for convolution
  image_t unfiltered_roi, filtered_roi;

  const unsigned ni = image.ni();
  const unsigned nj = image.nj();

  vgl_box_2d< unsigned > scaled_region(
    static_cast<unsigned>( instruction.region.min_x() * ni ),
    static_cast<unsigned>( instruction.region.max_x() * ni ),
    static_cast<unsigned>( instruction.region.min_y() * nj ),
    static_cast<unsigned>( instruction.region.max_y() * nj ) );

  vgl_box_2d< unsigned > adjusted_region;

  if( settings_.vertical_jitter < scaled_region.min_x() &&
      settings_.vertical_jitter < scaled_region.min_y() )
  {
    adjusted_region = vgl_box_2d< unsigned > (
      scaled_region.min_x() - settings_.vertical_jitter,
      scaled_region.max_x() + settings_.vertical_jitter,
      scaled_region.min_y() - settings_.vertical_jitter,
      scaled_region.max_y() + settings_.vertical_jitter );
  }
  else
  {
    adjusted_region = scaled_region;
  }

  unfiltered_roi = point_view_to_region( image, adjusted_region );

  if( unfiltered_roi.size() == 0 )
  {
    LOG_WARN( "Text parser received invalid region to scan" );
    return parsed_string;
  }

  // Perform template matching
  if( settings_.algorithm == text_parser_settings::TEMPLATE_MATCHING )
  {
    // Use default models if no custom models in the instruction set are provided
    model_t& models = ( instruction.models ? *instruction.models : default_models_ );

    if( !models.has_templates() && !settings_.output_directory.empty() )
    {
      LOG_ERROR( "No valid models provided to text parser" );
      return parsed_string;
    }

    if( image.nplanes() != models.templates[0].second.nplanes() )
    {
      LOG_ERROR( "Template channel count does not match input image" );
      return parsed_string;
    }

    // Rescale input image if required
    double scale_factor = 1.0;

    if( settings_.model_resolution[1] > 0 )
    {
      scale_factor = static_cast<double>( settings_.model_resolution[1] ) / image.nj();
    }

    format_region( unfiltered_roi, filtered_roi,
                   scale_factor,
                   settings_.invert_image );

    // Perform convolution with each template
    const templates_t& templates = models.templates;
    const unsigned template_count = templates.size();

    std::vector< vil_image_view< double > > ssd_results( templates.size() );
    unsigned min_col_count = std::numeric_limits< unsigned >::max();

    for( unsigned t = 0; t < template_count; ++t )
    {
      get_ssd_surf( filtered_roi,
                    ssd_results[t],
                    templates[t].second );

      min_col_count = std::min( min_col_count, ssd_results[t].ni() );
    }

    // Compute template normalization and overlap factors
    double thresh_norm_factor = static_cast<double>( std::numeric_limits< PixType >::max() );
    thresh_norm_factor = thresh_norm_factor * thresh_norm_factor;

    double normalized_threshold = thresh_norm_factor * settings_.detection_threshold;

    unsigned step_threshold = static_cast< unsigned >( settings_.overlap_threshold *
                                                       templates[0].second.ni() );

    // Compress results to one dimension via thresholding
    std::vector< position_result > detections;
    position_result top_result;

    // For every column in the line...
    for( unsigned i = 0; i < min_col_count; ++i )
    {
      // Take the maximum response from all templates at all vertical locs...
      position_result top_col_result;

      for( unsigned t = 0; t < template_count; ++t )
      {
        for( unsigned j = 0; j < ssd_results[t].nj(); ++j )
        {
          if( ssd_results[ t ]( i, j ) < top_col_result.weight )
          {
            top_col_result.weight = ssd_results[ t ]( i, j );
            top_col_result.position = i;
            top_col_result.template_id = t;
          }
        }
      }

      // Record overall top result
      if( top_col_result.weight < top_result.weight )
      {
        top_result = top_col_result;
      }

      // Record all thresholded detections
      if( top_col_result.weight < normalized_threshold )
      {
        detections.push_back( top_col_result );
      }
    }

    // Perform non-maximum suppression
    std::vector< position_result > final_selections;

    if( instruction.select_top )
    {
      final_selections.push_back( top_result );
    }
    else
    {
      std::sort( detections.begin(), detections.end(), sort_by_weight );

      while( !detections.empty() )
      {
        final_selections.push_back( detections.front() );

        const unsigned position = final_selections.back().position;
        const unsigned lower_bound = ( position > step_threshold ? position - step_threshold : 0 );
        const unsigned upper_bound = position + step_threshold;

        std::vector< position_result >::iterator new_end =
          std::remove_if( detections.begin(), detections.end(),
                          boost::bind( is_in_bounds, _1, lower_bound, upper_bound ) );

        detections.erase( new_end, detections.end() );
      }
    }

    // Formulate output string
    std::sort( final_selections.begin(), final_selections.end(), sort_by_position );

    for( unsigned i = 0; i < final_selections.size(); ++i )
    {
      parsed_string += templates[ final_selections[ i ].template_id ].first;
    }
  }
  else
  {
#ifdef USE_TESSERACT

    // Format input image
    vil_image_view<vxl_byte> single_plane_byte_roi, tess_roi;
    vil_convert_stretch_range( unfiltered_roi, tess_roi );
    vil_math_mean_over_planes( tess_roi, single_plane_byte_roi, unsigned() );

    double scale_factor = 1.0;

    if( settings_.model_resolution[1] > 0 )
    {
      scale_factor = static_cast<double>( settings_.model_resolution[1] ) / image.nj();
    }

    format_region( single_plane_byte_roi, tess_roi,
                   scale_factor,
                   settings_.invert_image );

    // Use tesseract library to parse text
    ext_->tess_parser.SetImage( tess_roi.top_left_ptr(),
                                tess_roi.ni(),
                                tess_roi.nj(),
                                1, tess_roi.jstep() );

    // Retrieve text
    char* tess_output = ext_->tess_parser.GetUTF8Text();
    parsed_string = tess_output;
    delete[] tess_output;

    // If writing out image, cast it to the original format
    if( !settings_.output_directory.empty() )
    {
      vil_convert_cast( tess_roi, filtered_roi );
    }

#endif
  }

  // Optionally output images to directory
  if( !settings_.output_directory.empty() )
  {
    static unsigned counter = 1;
    std::string output_fn = settings_.output_directory + "/region";
    output_fn += boost::lexical_cast<std::string>( counter ) + ".png";
    image_t original_region = point_view_to_region( image, scaled_region );
    counter++;

    if( settings_.painted_text_size > 0 )
    {
#ifdef USE_OPENCV

      cv::Mat output_image;
      deep_cv_conversion( filtered_roi, output_image );

      cv::putText( output_image,
                   parsed_string,
                   cv::Point( 0, filtered_roi.nj()-1 ),
                   cv::FONT_HERSHEY_COMPLEX_SMALL,
                   settings_.painted_text_size,
                   cv::Scalar( 0, 255, 0 ) );

      cv::imwrite( output_fn, output_image );

#else
      LOG_WARN( "Draw text option requires a build with OpenCV" );
#endif
    }
    else
    {
      vil_save( filtered_roi, output_fn.c_str() );
    }
  }

  return parsed_string;
}


} // end namespace vidtk
