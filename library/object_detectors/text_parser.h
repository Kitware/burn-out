/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_text_parser_h_
#define vidtk_text_parser_h_

#include <utilities/external_settings.h>

#include <vil/vil_image_view.h>

#include <vgl/vgl_box_2d.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>
#include <vector>
#include <map>

namespace vidtk
{


/// \brief Settings for the text_parser class.
#define settings_macro( add_param, add_array, add_enumr ) \
  add_enumr( \
    algorithm, \
    (TEMPLATE_MATCHING) (TESSERACT_OCR), \
    TEMPLATE_MATCHING, \
    "Algorithm used to perform OCR. Can either be template_matching " \
    "(to use a simple matcher with a template for every character) " \
    "or tesseract_ocr (to use the method provided by the tesseract " \
    "external package)." ); \
  add_param( \
    model_directory, \
    std::string, \
    "", \
    "Directory storing default character models for either method." ); \
  add_param( \
    language, \
    std::string, \
    "eng", \
    "Three character language abbreviation string, specifying the " \
    "language of the text we're parsing." ); \
  add_param( \
    detection_threshold, \
    double, \
    0.1, \
    "Character detection threshold in range [0.0, 1.0]. A higher value " \
    "will lead to more detections (ie, greater sensitivity)." ); \
  add_param( \
    overlap_threshold, \
    double, \
    0.5, \
    "Overlap (non-maximum suppression) setting. If detected characters " \
    "have a percentage of overlap less than this threshold, only the " \
    "higher will be reported." ); \
  add_param( \
    vertical_jitter, \
    unsigned, \
    1, \
    "Number of pixels to jitter models vertically." ); \
  add_param( \
    invert_image, \
    bool, \
    false, \
    "Should all input pixel values be inverted before running OCR?" ); \
  add_array( \
    model_resolution, \
    unsigned, \
    2, \
    "0 0", \
    "The image resolution (width height) at which the models were generated " \
    "at, set to 0 if unknown. If set and input resolutions do not match these " \
    "values, the template models will be scaled accordingly." ); \
  add_param( \
    output_directory, \
    std::string, \
    "", \
    "If set, output a copy of the image region we are scanning for " \
    "text in to this directory." ); \
  add_param( \
    painted_text_size, \
    unsigned, \
    0, \
    "If non-zero, paint detected text of the given size on the output " \
    "debug images." ); \

init_external_settings3( text_parser_settings, settings_macro );

#undef settings_macro


/// \brief Model group for storing a template for each character.
template< typename PixType = vxl_byte >
class text_parser_model_group
{
public:

  typedef vil_image_view< PixType > image_t;
  typedef std::vector< std::pair< std::string, image_t > > templates_t;

  text_parser_model_group() {}
  virtual ~text_parser_model_group() {}

  /// Load all valid character templates from some directory.
  bool load_templates( const std::string& directory );

  /// Are any models loaded?
  bool has_templates() const { return !templates.empty(); }

  /// Collection of models (one per character).
  templates_t templates;
};


/// \brief An instruction detailing which region of the image to parse and how.
template< typename PixType = vxl_byte >
class text_parser_instruction
{
public:

  typedef text_parser_model_group< PixType > model_t;
  typedef boost::shared_ptr< model_t > model_sptr_t;

  typedef vgl_box_2d< double > region_t;

  enum text_t
  {
    UNKNOWN = 0,
    GROUND_SAMPLE_DISTANCE,
    TARGET_WIDTH,
    FIELD_OF_VIEW_LEVEL,
    CENTER_WORLD_LOCATION
  };

  text_parser_instruction() : type( UNKNOWN ), select_top( false ) {}
  virtual ~text_parser_instruction() {}

  /// The type of text, if known.
  text_t type;

  /// Normalized region in the image to scan for text.
  region_t region;

  /// Any optional, custom models to use with this instruction.
  model_sptr_t models;

  /// Selects the top model no matter what, ignoring any settings.
  bool select_top;
};


/// \brief Algorithm to perform text parsing based on character templates.
template< typename PixType = vxl_byte >
class text_parser
{
public:

  typedef vil_image_view< PixType > image_t;
  typedef text_parser_instruction< PixType > instruction_t;
  typedef text_parser_model_group< PixType > model_t;
  typedef boost::shared_ptr< model_t > model_sptr_t;
  typedef typename model_t::templates_t templates_t;

  text_parser();
  virtual ~text_parser();

  /// Set new text parser settings.
  virtual bool configure( const text_parser_settings& settings );

  /// Parse text from the supplied image.
  virtual std::string parse_text( const image_t& image,
                                  const instruction_t& instruction );

protected:

  // Default models to use if the instruction set contains no models.
  model_t default_models_;

  // Internal operating settings.
  text_parser_settings settings_;

private:

  // Pointer to hide data types from other optional libraries
  class external_data;
  boost::scoped_ptr< external_data > ext_;
};


} // end namespace vidtk

#endif // vidtk_text_parser_h_
