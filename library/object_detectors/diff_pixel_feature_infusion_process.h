/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_diff_pixel_feature_infusion_process_h_
#define vidtk_diff_pixel_feature_infusion_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <resource_pool/resource_user.h>

#include <tracking_data/pixel_feature_array.h>
#include <tracking_data/image_border.h>
#include <tracking_data/gui_frame_info.h>

#include <utilities/timestamp.h>

#include <object_detectors/pixel_feature_writer.h>
#include <object_detectors/obj_specific_salient_region_classifier.h>

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>

namespace vidtk
{

template <class PixType>
class diff_pixel_feature_infusion_process
  : public process,
    public resource_user
{
public:
  typedef diff_pixel_feature_infusion_process self_type;

  typedef float weight_t;
  typedef PixType pixel_t;

  typedef vil_image_view< pixel_t > input_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef vil_image_view< pixel_t > motion_image_t;
  typedef vil_image_view< weight_t > classified_image_t;
  typedef std::vector< input_image_t > feature_array_t;

  typedef vgl_box_2d< int > image_border_t;
  typedef pixel_feature_writer< pixel_t > feature_writer_t;
  typedef boost::scoped_ptr< feature_writer_t > feature_writer_sptr_t;
  typedef typename pixel_feature_array< pixel_t >::sptr_t features_sptr_t;
  typedef obj_specific_salient_region_classifier< pixel_t, weight_t > classifier_t;
  typedef boost::shared_ptr< classifier_t > classifier_sptr_t;

  diff_pixel_feature_infusion_process( std::string const& );
  virtual ~diff_pixel_feature_infusion_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  // Input Ports
  void set_diff_mask( mask_image_t const& src );
  VIDTK_INPUT_PORT( set_diff_mask, mask_image_t const& );

  void set_diff_image( motion_image_t const& src );
  VIDTK_INPUT_PORT( set_diff_image, motion_image_t const& );

  void set_border( image_border_t const& src );
  VIDTK_INPUT_PORT( set_border, image_border_t const& );

  void set_input_features( feature_array_t const& src );
  VIDTK_INPUT_PORT( set_input_features, feature_array_t const& );

  void set_gui_feedback( gui_frame_info const& src );
  VIDTK_INPUT_PORT( set_gui_feedback, gui_frame_info const& );

  // Output Ports
  classified_image_t classified_image() const;
  VIDTK_OUTPUT_PORT( classified_image_t, classified_image );

  features_sptr_t feature_array() const;
  VIDTK_OUTPUT_PORT( features_sptr_t, feature_array );

  // Non-Port Output
  classifier_sptr_t get_classifier(){ return classifier_; }

protected:

  // Configuration
  config_block config_;
  int motion_image_crop_[2];
  weight_t border_fill_value_;

  // Inputs
  const feature_array_t* input_features_;
  const mask_image_t* diff_mask_;
  const motion_image_t* diff_image_;
  const image_border_t* border_;
  gui_frame_info gfi_;

  // Outputs
  classified_image_t output_;
  features_sptr_t features_sptr_;

  // Classifiers
  classifier_sptr_t classifier_;

  // Helper Functions
  void reset_inputs();

  // Save re-alloc
  input_image_t new_feature_;

  // Training mode parameters
  bool is_training_mode_;
  bool output_feature_images_;
  std::string feature_output_folder_;
  unsigned frame_counter_;
  feature_writer_sptr_t feature_writer_;
};

} // end namespace vidtk

#endif // vidtk_diff_pixel_feature_infusion_process_h_
