/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_osd_recognizer_process_h_
#define vidtk_osd_recognizer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <video_properties/border_detection_process.h>

#include <object_detectors/osd_template.h>
#include <object_detectors/osd_recognizer.h>
#include <object_detectors/scene_obstruction_detector.h>
#include <object_detectors/metadata_text_parser_process.h>

#include <boost/shared_ptr.hpp>

#include <vector>


namespace vidtk
{


/// \brief A process which recognizes the type of OSD displayed on video.
///
/// This process outputs several notable properties of the OSD, as well
/// as any recommended actions to be performed to refine the output mask
/// covering the OSD.
template <typename PixType>
class osd_recognizer_process
  : public process
{

public:

  typedef osd_recognizer_process self_type;
  typedef vxl_byte feature_type;
  typedef std::vector< vil_image_view<feature_type> > feature_array;
  typedef scene_obstruction_properties<PixType> mask_properties_type;
  typedef osd_recognizer<PixType,vxl_byte> recognizer_type;
  typedef text_parser_instruction< PixType > text_instruction_type;
  typedef std::vector< text_instruction_type > text_instructions_type;
  typedef osd_recognizer_output< PixType, vxl_byte > osd_properties_type;
  typedef boost::shared_ptr< osd_properties_type > osd_properties_sptr_type;

  osd_recognizer_process( std::string const& _name );
  virtual ~osd_recognizer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();
  virtual bool text_parsing_enabled() const;

  void set_classified_image( vil_image_view<double> const& src );
  VIDTK_INPUT_PORT( set_classified_image, vil_image_view<double> const& );

  void set_mask_properties( mask_properties_type const& props );
  VIDTK_INPUT_PORT( set_mask_properties, mask_properties_type const& );

  void set_source_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_border( image_border const& rect );
  VIDTK_INPUT_PORT( set_border, image_border const& );

  void set_input_features( feature_array const& array );
  VIDTK_INPUT_PORT( set_input_features, feature_array const& );

  osd_properties_sptr_type output() const;
  VIDTK_OUTPUT_PORT( osd_properties_sptr_type, output );

  std::string detected_type() const;
  VIDTK_OUTPUT_PORT( std::string, detected_type );

  vil_image_view<bool> mask_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, mask_image );

  text_instructions_type text_instructions() const;
  VIDTK_OUTPUT_PORT( text_instructions_type, text_instructions );

  std::string dynamic_instruction_set_1() const;
  VIDTK_OUTPUT_PORT( std::string, dynamic_instruction_set_1 );

  std::string dynamic_instruction_set_2() const;
  VIDTK_OUTPUT_PORT( std::string, dynamic_instruction_set_2 );

private:

  // Primary Inputs
  vil_image_view<double> classified_image_;
  mask_properties_type properties_;

  // Secondary Inputs
  vil_image_view<PixType> source_image_;
  image_border border_;
  feature_array features_;

  // Outputs
  std::string detected_type_;
  osd_properties_sptr_type osd_properties_;
  vil_image_view<bool> output_mask_;
  text_instructions_type text_instructions_;
  std::string dynamic_instruction_set_1_;
  std::string dynamic_instruction_set_2_;

  // Internal parameters/settings
  config_block config_;

  // Recognizer algorithm
  recognizer_type recognizer_;
};


} // end namespace vidtk


#endif // vidtk_osd_recognizer_process_h_
