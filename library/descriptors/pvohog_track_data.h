/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pvohog_track_data_h_
#define vidtk_pvohog_track_data_h_

#include <descriptors/pvohog_generator.h>
#include <descriptors/sthog_descriptor.h>

#include <tracking_data/frame_data.h>
#include <learning/kwsvm.h>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

struct pvohog_parameters
{
  pvohog_parameters() {}
  ~pvohog_parameters() {}

  pvohog_settings settings;
  bool use_vo_po;
  boost::shared_ptr< sthog_descriptor > desc_vehicle;
  boost::shared_ptr< sthog_descriptor > desc_people;
  boost::shared_ptr< KwSVM > svm_vehicle;
  boost::shared_ptr< KwSVM > svm_people;

  boost::shared_ptr< sthog_descriptor > desc;
  boost::shared_ptr< KwSVM > svm;

  bool use_sthog;
  bool use_gsd;
  bool use_gsd_std;
  bool use_velocity;
  bool use_velocity_std;
  bool use_bbox_area;
  bool use_bbox_area_std;
  bool use_bbox_aspect;
  bool use_bbox_aspect_std;
  bool use_fg_coverage;
  bool use_fg_coverage_std;
  bool use_no_pca;

  cv::PCA pca;
  cv::Mat track_feats;
};

class pvohog_track_data : public track_data_storage
{

public:

  pvohog_track_data(pvohog_parameters const& in_params, track_sptr const track);
  ~pvohog_track_data();

  bool update(track_sptr const track, frame_data_sptr const frame);
  descriptor_sptr_t make_descriptor();

private:

  std::vector< double > pvo_scores();
  std::vector< double > compute_pvo_scores();
  std::vector< double > compute_po_vo_scores();

  struct pvohog_frame_data
  {
    bool valid;

    // Frame information.
    timestamp ts;

    cv::Rect bbox;
    cv::Mat image;
    cv::Point position;

    double gsd;
    double velocity;
    double angle;
    double bbox_area;
    double bbox_aspect;
    double fg_coverage;

    // Raw classification values.
    double p_classification;
    double v_classification;
  };

  std::vector< pvohog_frame_data >::const_reverse_iterator find_frame_span(unsigned* nframes) const;

  pvohog_parameters const params;
  unsigned const track_id;
  timestamp const start_time;

  // Per-frame scores.
  std::vector< std::vector< double > > scores;

  // GSD-modulated scores which determine which PVO score to use at the end.
  std::vector< double > scores_gsd;

  std::vector< pvohog_frame_data > frames;

  timestamp current_time;

  double highest_p_value, highest_v_value;

  bool ended;

  unsigned temporal_state;

  // Generate the raw classifier variant of this descriptor.
  bool generate_clfr_desc(track_sptr const track, frame_data_sptr const frame);
};


} // end namespace vidtk

#endif // vidtk_pvohog_track_data_h_
