/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_metadata_text_parser_process_h_
#define vidtk_metadata_text_parser_process_h_

#include "text_parser.h"

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/image_border.h>

#include <vil/vil_image_view.h>

#include <utilities/video_metadata.h>
#include <utilities/ring_buffer.h>

namespace vidtk
{


/// \brief This process parses text which can appear on different
/// classes of metadata burn-in (a.k.a. on-screen displays).
template <typename PixType>
class metadata_text_parser_process
  : public process
{
public:

  typedef metadata_text_parser_process self_type;

  typedef vil_image_view< PixType > image_t;
  typedef text_parser_instruction< PixType > instruction_t;
  typedef std::vector< instruction_t > instruction_set_t;
  typedef std::vector< unsigned > width_set_t;
  typedef std::vector< std::string > string_set_t;

  metadata_text_parser_process( std::string const& name );
  virtual ~metadata_text_parser_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_image( image_t const& );
  VIDTK_INPUT_PORT( set_image, image_t const& );

  void set_metadata( video_metadata::vector_t const& );
  VIDTK_INPUT_PORT( set_metadata, video_metadata::vector_t const& );

  void set_instructions( instruction_set_t const& );
  VIDTK_INPUT_PORT( set_instructions, instruction_set_t const& );

  void set_target_widths( width_set_t const& );
  VIDTK_INPUT_PORT( set_target_widths, width_set_t const& );

  void set_border( image_border const& border );
  VIDTK_INPUT_PORT( set_border, image_border const& );

  string_set_t parsed_strings() const;
  VIDTK_OUTPUT_PORT( string_set_t, parsed_strings );

  video_metadata::vector_t output_metadata() const;
  VIDTK_OUTPUT_PORT( video_metadata::vector_t, output_metadata );

private:

  // Configuration settings
  bool disabled_;
  config_block config_;

  // Inputs
  image_t image_;
  video_metadata::vector_t meta_in_;
  image_border border_;
  instruction_set_t instructions_;
  width_set_t widths_;

  // Algorithm
  text_parser< PixType > algorithm_;

  // Parameters
  bool adjust_fov_;
  bool remove_existing_metadata_;
  bool parse_on_valid_metadata_only_;
  double forced_target_width_multiplier_;
  double fov_multiplier_;
  unsigned required_full_width_frame_count_;
  unsigned frames_without_brackets_;
  int border_pixel_threshold_;
  unsigned frame_consistency_threshold_;
  double consistency_measure_;
  unsigned max_nonreport_frame_limit_;
  double backup_gsd_;

  // Internal variables
  bool backup_gsd_enabled_;
  std::string last_fov_level_;
  ring_buffer< double > computed_gsds_;
  ring_buffer< double > raw_frame_gsds_;
  unsigned frames_since_gsd_output_;

  // Outputs
  video_metadata::vector_t meta_out_;
  string_set_t parsed_strings_;
};


} // end namespace vidtk


#endif // vidtk_metadata_text_parser_process_h_
