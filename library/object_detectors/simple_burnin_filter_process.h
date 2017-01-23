/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_simple_burnin_filter_process_h_
#define vidtk_simple_burnin_filter_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <object_detectors/osd_recognizer.h>

namespace vidtk
{


/// \brief A simple filter class to highlight possible burnin symbols.
///
/// This process produces a 3-plane image where planes 1 and 2 are
/// responses to horizontally and vertically oriented filters and
/// plane 3 is the response to a bidirectional filter. The filter
/// is defined as a high-pass intensity filter multiplied by a
/// color distance function.
///
/// If no color information is supplied, the process tries to detect
/// black or white metadata and returns the result with the highest
/// response. The metadata_is_black port is true if the metadata
/// is near black and false otherwise.
template< typename PixType >
class simple_burnin_filter_process
  : public process
{
public:
  typedef simple_burnin_filter_process self_type;
  typedef osd_recognizer_output< PixType, vxl_byte > osd_properties_t;
  typedef boost::shared_ptr< const osd_properties_t > osd_properties_sptr_t;

  simple_burnin_filter_process( std::string const& name );
  virtual ~simple_burnin_filter_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view< PixType > const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  /// \brief The input burnin properties, if known.
  void set_burnin_properties( osd_properties_sptr_t const& properties );
  VIDTK_INPUT_PORT( set_burnin_properties, osd_properties_sptr_t const& );

  /// \brief The output image.
  vil_image_view< PixType > metadata_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, metadata_image );

  /// \brief The the output is true if the burnin is black.
  bool metadata_is_black() const;
  VIDTK_OUTPUT_PORT( bool, metadata_is_black );

private:

  // Run simple filtering algorithm
  unsigned long detect_burnin(
    const vil_image_view< PixType >& image,
    const vil_image_view< PixType >& smooth,
    vil_image_view< PixType >& weight,
    int burnin_intensity );

  // Parameters
  config_block config_;

  int kernel_width_;
  int kernel_height_;
  bool interlaced_;
  int adapt_thresh_high_;
  int abs_thresh_low_;
  int abs_thresh_high_;
  bool use_target_intensity_;
  PixType target_intensity_;

  // Inputs
  vil_image_view< PixType > image_;
  osd_properties_sptr_t osd_info_;

  // Outputs
  vil_image_view< PixType > filtered_image_;
  bool is_black_;
};


} // end namespace vidtk


#endif // vidtk_simple_burnin_filter_process_h_
