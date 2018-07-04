/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pvohog_track_data.h"

#include <opencv2/imgproc/imgproc.hpp>

#include <vidl/vidl_frame_sptr.h>
#include <vidl/vidl_convert.h>

#include <utilities/vxl_to_cv_converters.h>

#include <vnl/vnl_math.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <cmath>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/foreach.hpp>

//#define TIME_BASED_RESULT

#include <logger/logger.h>

VIDTK_LOGGER("pvohog_track_data");

namespace vidtk
{

pvohog_track_data
::pvohog_track_data(pvohog_parameters const& in_params, track_sptr const track)
  : params(in_params)
  , track_id(track->id())
  , start_time(track->first_state()->time_)
  , highest_p_value(-std::numeric_limits<double>::max())
  , highest_v_value(-std::numeric_limits<double>::max())
  , ended(false)
  , temporal_state(params.settings.frame_start)
{
  LOG_DEBUG("PVOHOG:" << this->track_id << ": Initializing");
}

pvohog_track_data
::~pvohog_track_data()
{
}


// ------------------------------------------------------------------
bool
pvohog_track_data
::update(track_sptr const track, frame_data_sptr const frame)
{
  pvohog_settings const& settings = params.settings;

  if (settings.raw_classifier_output_mode)
  {
    return generate_clfr_desc(track, frame);
  }

  bool result = false;

  if ( this->ended )
  {
    LOG_INFO("PVOHOG:" << this->track_id << ": Updating ended track");
    return result;
  }

  this->current_time = track->last_state()->time_;
  unsigned const current_frame = this->current_time.frame_number();

  this->frames.push_back(pvohog_frame_data());
  pvohog_frame_data& fdata = this->frames.back();
  track_state_sptr const state = track->last_state();
  image_object_sptr image_obj;

  if (!state->image_object(image_obj))
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Skipping frame " << current_frame
        << " which has an invalid image object");
    fdata.valid = false;
    return result;
  }

  if (!frame->was_gsd_set() && params.use_gsd)
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Skipping frame " << current_frame
        << " which does not have a GSD");
    fdata.valid = false;
    return result;
  }

  vgl_box_2d<unsigned> const& img_bbox = image_obj->get_bbox();
  vgl_box_2d<int> bbox;

  if (params.use_sthog)
  {
    vgl_point_2d<unsigned> const orig_tl = img_bbox.min_point();
    unsigned const left = orig_tl.x();
    unsigned const top = orig_tl.y();
    unsigned const right = left + img_bbox.width();
    unsigned const bottom = top + img_bbox.height();
    unsigned const bbox_size = settings.bbox_size;

    int const new_left = (left + right - bbox_size) / 2;
    int const new_top = (top + bottom - bbox_size) / 2;
    bbox.set_min_x(new_left);
    bbox.set_min_y(new_top);
    bbox.set_max_x(new_left + bbox_size);
    bbox.set_max_y(new_top + bbox_size);
  }
  else
  {
    bbox.set_min_x(img_bbox.min_x());
    bbox.set_min_y(img_bbox.min_y());
    bbox.set_max_x(img_bbox.max_x());
    bbox.set_max_y(img_bbox.max_y());
  }

  vil_image_view<vxl_byte> const& image = frame->image_8u_gray();
  vgl_box_2d<int> const frame_bbox(0, image.ni(), 0, image.nj());

  unsigned const iarea = vgl_intersection(bbox, frame_bbox).volume();
  unsigned const oarea = bbox.volume();

  if (iarea != oarea)
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Skipping frame " << current_frame
        << " which has a box off of the frame");
    fdata.valid = false;
    return result;
  }

  fdata.valid = true;

  fdata.ts = this->current_time;

  vgl_point_2d<int> const tl = bbox.min_point();
  unsigned const width = bbox.width();
  unsigned const height = bbox.height();
  fdata.bbox.x = tl.x();
  fdata.bbox.y = tl.y();
  fdata.bbox.width = width;
  fdata.bbox.height = height;

  forced_cv_conversion( image, fdata.image );

  fdata.position.x = tl.x();
  fdata.position.y = tl.y();

  if (frame->was_gsd_set())
  {
    fdata.gsd = frame->average_gsd();
  }
  else
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Using default gsd on frame " << current_frame);
    fdata.gsd = settings.backup_gsd;
  }

  double const dx = state->img_vel_[0];
  double const dy = state->img_vel_[1];

  fdata.velocity = std::sqrt(dx * dx + dy * dy);

  fdata.angle = ( dy == 0 && dx == 0 ? 0 : std::atan2(-dy, dx) );

  fdata.bbox_area = oarea * fdata.gsd * fdata.gsd;

  if (width && height)
  {
    fdata.bbox_aspect = (width > height) ? (1.0 * height / width) : (1.0 * width / height);
  }
  else
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Zero-sized box on frame " << current_frame);
    fdata.bbox_aspect = 1.0;
  }

  fdata.fg_coverage = image_obj->get_area() / oarea;

#ifdef TIME_BASED_RESULT
  if (2 <= (this->current_time.time() - this->start_time.time() / 100000))
  {
    LOG_INFO("PVOHOG:" << this->track_id << ": Signaling on frame " << current_frame
        << " because 2 seconds have elapsed");
    result = true;
  }
#else

  unsigned const start_frame = this->start_time.frame_number();
  unsigned const nframes = current_frame - start_frame;

  LOG_INFO("PVOHOG:" << this->track_id << ": "
      "frame: " << current_frame << " "
      "track start: " << start_frame << " "
      "end: " << settings.frame_end << " "
      "start: " << settings.frame_start);

  if (this->temporal_state <= nframes)
  {
    result = true;

    LOG_INFO("PVOHOG:" << this->track_id << ": Signaling on frame " << current_frame
        << " because we have enough frames to compute a PVO on");

    while (this->temporal_state <= nframes)
    {
      this->temporal_state += settings.frame_step;
    }
  }
#endif

  return result;
}


// ------------------------------------------------------------------
descriptor_sptr_t
pvohog_track_data
::make_descriptor()
{
  pvohog_settings const& settings = params.settings;

  std::vector< double > descriptor_values;

  if (!settings.raw_classifier_output_mode)
  {
    descriptor_values = pvo_scores();

    if (descriptor_values.empty())
    {
      LOG_WARN("PVOHOG:" << this->track_id << ": No PVO score made; not outputting a descriptor");
      return descriptor_sptr();
    }
  }
  else
  {
    descriptor_values.resize( 6 );

    descriptor_values[0] = this->frames.back().v_classification;
    descriptor_values[1] = this->frames.back().p_classification;
    descriptor_values[2] = ( descriptor_values[0] > descriptor_values[1] ? 1.0 : 0.0 );

    if( descriptor_values[0] > this->highest_v_value )
    {
      this->highest_v_value = descriptor_values[0];
    }

    if( descriptor_values[1] > this->highest_p_value )
    {
      this->highest_p_value = descriptor_values[1];
    }

    descriptor_values[3] = this->highest_v_value;
    descriptor_values[4] = this->highest_p_value;
    descriptor_values[5] = ( this->highest_v_value > this->highest_p_value ? 1.0 : 0.0 );
  }

  descriptor_sptr_t const descriptor = create_descriptor( settings.descriptor_name, this->track_id );

  descriptor->set_pvo_flag( true );
  descriptor->set_features( descriptor_values );

  BOOST_FOREACH( pvohog_frame_data const& fdata, this->frames )
  {
    if ( ! fdata.valid )
    {
      LOG_WARN("PVOHOG:" << this->track_id << ": Skipping an invalid frame for the descriptor history");
      continue;
    }

    vgl_box_2d<unsigned> bbox;
    cv::Rect const& rect = fdata.bbox;

    bbox.set_min_x(rect.x);
    bbox.set_min_y(rect.y);
    bbox.set_max_x(rect.x + rect.width);
    bbox.set_max_y(rect.y + rect.height);

    descriptor_history_entry const hist_entry( fdata.ts, bbox );

    descriptor->add_history_entry( hist_entry );
  }

  return descriptor;
}


// ------------------------------------------------------------------
std::vector<double>
pvohog_track_data
::pvo_scores()
{
  // We're computing a score, we're done with a track.
  ended = true;

  if (params.use_vo_po)
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Computing PVO using a PO/VO model");
    return compute_po_vo_scores();
  }
  else
  {
    LOG_WARN("PVOHOG:" << this->track_id << ": Computing PVO using a full model");
    return compute_pvo_scores();
  }
}

typedef std::vector< double > histogram_type;

static double value_from_histogram(histogram_type const hist, double min, double max, double score);
static std::vector<double> make_pvo(double p, double v, double o);


// ------------------------------------------------------------------
std::vector<double>
pvohog_track_data
::compute_pvo_scores()
{
  unsigned nframes;
  std::vector<pvohog_frame_data>::const_reverse_iterator start = find_frame_span(&nframes);

  pvohog_settings const& settings = params.settings;

  if ((start == this->frames.rend()) || (nframes < settings.samples))
  {
    LOG_WARN("PVOHOG.PVO:" << this->track_id << ": Failed to "
        "find enough contiguous frames to compute a score: "
        "contiguous: " << nframes << " "
        "samples: " << settings.samples);
    return std::vector<double>();
  }

  if (settings.dbg_write_outputs)
  {
    // TODO: Write out collage image for training.
  }

  std::vector<cv::Mat> images;
  std::vector<cv::Point> points;

#define features(call) \
  call(gsd)            \
  call(velocity)       \
  call(bbox_area)      \
  call(bbox_aspect)    \
  call(fg_coverage)

#define declare_vecs(feature) \
  std::vector<double> feature##_vec;

  features(declare_vecs)

#undef declare_vecs

  for (unsigned i = 0; i < nframes; ++i, ++start)
  {
    pvohog_frame_data const& frame = *start;

    if (params.use_sthog)
    {
      images.push_back(frame.image);
      points.push_back(frame.position);
    }

#define fill_features(feature)              \
  if (params.use_##feature ||               \
      params.use_##feature##_std)           \
  {                                         \
    feature##_vec.push_back(frame.feature); \
  }

    features(fill_features)

#undef fill_features
  }

#define declare_feature_values(feature) \
  double feature##_mean = 0.;           \
  double feature##_std = 0.;

  features(declare_feature_values)

#undef declare_feature_values

  namespace ba = boost::accumulators;
  typedef ba::accumulator_set<double, ba::stats
    < ba::tag::mean
    , ba::tag::variance(ba::lazy)
    > > stats_t;

#define feature_stats(feature)                     \
  if (params.use_##feature ||                      \
      params.use_##feature##_std)                  \
  {                                                \
    std::vector<double> const& vec = feature##_vec; \
    stats_t stats =                                \
      std::for_each(vec.begin(), vec.end(),         \
                   stats_t());                     \
                                                   \
    feature##_mean = ba::mean(stats);              \
    feature##_std = std::sqrt(ba::variance(stats)); \
  }                                                \

  features(feature_stats)

#undef feature_stats

  cv::Mat projected_feature_raw;

  if (params.use_sthog)
  {
    std::vector<float> feature;

    params.desc->compute(images, points);
    params.desc->get_sthog_descriptor(feature, points[0]);

    cv::Mat feature_mat(1, feature.size(), CV_32FC1);

    for (size_t i = 0; i < feature.size(); ++i)
    {
      feature_mat.at<float>(0, i) = feature[i];
    }

    if (!params.use_no_pca)
    {
      params.pca.project(feature_mat, projected_feature_raw);
    }
    else
    {
      projected_feature_raw = feature_mat;
    }
  }

#define count_feature(feature) \
  + params.use_##feature + params.use_##feature##_std

  size_t const nfeatures = projected_feature_raw.cols
                           features(count_feature);

#undef count_feature

  size_t const base_feature_col = projected_feature_raw.cols;

  cv::Mat projected_feature(1, nfeatures, CV_32FC1);
  cv::Mat projected_feature_view = projected_feature(cv::Range(0, 1), cv::Range(0, base_feature_col));
  projected_feature_raw.copyTo(projected_feature_view);

  size_t feature_col = 0;

#define load_feature(feature)                                           \
  if (params.use_##feature)                                             \
  {                                                                     \
    float& norm_val =                                                   \
      projected_feature.at<float>(0, base_feature_col + feature_col);   \
    norm_val =                                                          \
      (feature##_mean - params.track_feats.at<float>(0, feature_col)) / \
      params.track_feats.at<float>(1, feature_col);                     \
    LOG_DEBUG("PVOHOG.PVO:" << this->track_id << ": " #feature " "            \
        "raw: " << feature##_mean << " "                                \
        "norm: " << norm_val);                                          \
    ++feature_col;                                                      \
  }                                                                     \
                                                                        \
  if (params.use_##feature##_std)                                       \
  {                                                                     \
    float& norm_val =                                                   \
      projected_feature.at<float>(0, base_feature_col + feature_col);   \
    norm_val =                                                          \
      (feature##_std - params.track_feats.at<float>(0, feature_col)) /  \
      params.track_feats.at<float>(1, feature_col);                     \
    LOG_DEBUG("PVOHOG.PVO:" << this->track_id << ": " #feature "_std "        \
        "raw: " << feature##_std << " "                                 \
        "norm: " << norm_val);                                          \
    ++feature_col;                                                      \
  }

  features(load_feature)

#undef load_feature

#undef features

  std::vector<float> pvo;

  params.svm->predict(projected_feature, pvo);

  for (std::vector<float>::const_iterator itr = pvo.begin(); itr != pvo.end(); itr++)
  {
    if (vnl_math::isnan(*itr))
    {
      LOG_WARN("PVOHOG.PVO:" << this->track_id << ": Found NaN values in the raw scores; skipping");
      return std::vector<double>();
    }
  }

  LOG_INFO("PVOHOG.PVO:" << this->track_id << ": Raw scores: "
      "0: " << pvo[0] << " "
      "1: " << pvo[1] << " "
      "2: " << pvo[2]);

  double p;
  double v;
  double o;

  if (settings.use_histogram_conversion)
  {
#define score_from_histogram(hist, cls, score) \
  value_from_histogram(settings.histogram_conversion_r##hist##_##cls, \
                       settings.histogram_conversion_r##hist##_min, \
                       settings.histogram_conversion_r##hist##_max, \
                       score)

    double const r0_p = score_from_histogram(0, p, pvo[0]);
    double const r0_v = score_from_histogram(0, v, pvo[0]);
    double const r0_o = score_from_histogram(0, o, pvo[0]);

    double const r1_p = score_from_histogram(1, p, pvo[1]);
    double const r1_v = score_from_histogram(1, v, pvo[1]);
    double const r1_o = score_from_histogram(1, o, pvo[1]);

    double const r2_p = score_from_histogram(2, p, pvo[2]);
    double const r2_v = score_from_histogram(2, v, pvo[2]);
    double const r2_o = score_from_histogram(2, o, pvo[2]);

    p = r0_p * r1_p * r2_p;
    v = r0_v * r1_v * r2_v;
    o = r0_o * r1_o * r2_o;
  }
  else
  {
    p = pvo[2];
    v = pvo[1];
    o = pvo[0];
  }

  LOG_INFO("PVOHOG.PVO:" << this->track_id << ": Raw PVO: "
      "P: " << p << " "
      "V: " << v << " "
      "O: " << o);

  return make_pvo(p, v, o);
}

static cv::Mat
extract_chip(cv::Mat const& image, cv::Point2d const& position, cv::Size const& size, double angle, double scale, bool flip);
static void
scale_images(std::vector<cv::Mat> const& images, std::vector<cv::Point> const& points, std::vector<double> const& gsds,
             double scale_factor, unsigned bbox_size, std::vector<cv::Mat>& scaled_images, std::vector<cv::Point>& scaled_points);
static double
spatial_search(boost::shared_ptr<sthog_descriptor> const& desc, std::vector<cv::Mat>& images, double threshold);
static double
compute_sthog(boost::shared_ptr<sthog_descriptor> const& desc, boost::shared_ptr<KwSVM> const& svm, std::vector<cv::Mat>& images, std::vector<cv::Point> const& points, bool use_linear_svm);
static double
adjust_value(double value, double sigma, double gsd, double threshold);


// ------------------------------------------------------------------
std::vector<double>
pvohog_track_data
::compute_po_vo_scores()
{
  unsigned nframes;
  std::vector<pvohog_frame_data>::const_reverse_iterator start = find_frame_span(&nframes);

  pvohog_settings const& settings = params.settings;

  if ((start == this->frames.rend()) || (nframes < settings.samples))
  {
    LOG_WARN("PVOHOG.PO/VO:" << this->track_id << ": Failed to "
        "find enough contiguous frames to compute a score: "
        "contiguous: " << nframes << " "
        "samples: " << settings.samples);
    return std::vector<double>();
  }

  std::vector<cv::Mat> images;
  std::vector<cv::Point> points;

#define features(call) \
  call(gsd)            \
  call(angle)

#define declare_vecs(feature) \
  std::vector<double> feature##_vec;

  features(declare_vecs)

#undef declare_vecs

  for (unsigned i = 0; i < nframes; ++i, ++start)
  {
    pvohog_frame_data const& frame = *start;

    images.push_back(frame.image);
    points.push_back(frame.position);

#define fill_features(feature) \
  feature##_vec.push_back(frame.feature);

    features(fill_features)

#undef fill_features
  }

#undef features

  std::vector<cv::Mat> far_images;
  std::vector<cv::Point> far_points;
  std::vector<cv::Mat> near_images;
  std::vector<cv::Point> near_points;

  if (settings.use_rotation_chips)
  {
    unsigned const chip_width = settings.bbox_size + 2 * settings.bbox_border;
    cv::Size const chip_size(chip_width, chip_width);
    cv::Point const ul_chip_pt(settings.bbox_border, settings.bbox_border);

    cv::Point2d const to_center(settings.bbox_size / 2, settings.bbox_size / 2);
    for (unsigned i = 0; i < nframes; ++i)
    {
      double const gsd = gsd_vec[i];
      double const vehicle_scale = gsd / settings.gsd_scale_vehicle;
      double const people_scale = gsd / settings.gsd_scale_people;

      cv::Point2d center = points[i];
      center += to_center;
      double const angle = angle_vec[i];
      bool const flip = (angle < -vnl_math::pi_over_2) || (vnl_math::pi_over_2 < angle);

      cv::Mat const vehicle_chip = extract_chip(images[i], center, chip_size, angle, vehicle_scale, flip);
      cv::Mat const people_chip = extract_chip(images[i], center, chip_size, 0., people_scale, flip);

      far_images.push_back(vehicle_chip);
      near_images.push_back(people_chip);

      far_points.push_back(ul_chip_pt);
      near_points.push_back(ul_chip_pt);
    }

    if (settings.dbg_write_outputs)
    {
      // TODO: Write out rotated images for training.
    }
  }
  else
  {
    scale_images(images, points, gsd_vec,
                 settings.gsd_scale_vehicle, settings.bbox_size,
                 far_images, far_points);
    scale_images(images, points, gsd_vec,
                 settings.gsd_scale_people, settings.bbox_size,
                 near_images, near_points);
  }

  double far_value;
  double near_value;

  if (settings.use_spatial_search)
  {
    far_value = spatial_search(params.desc_vehicle, far_images, settings.dt_threshold_vehicle);
    near_value = spatial_search(params.desc_people, near_images, settings.dt_threshold_people);
  }
  else
  {
    LOG_DEBUG("PVOHOG.PO/VO:" << this->track_id << ": Computing far feature");
    far_value = compute_sthog(params.desc, params.svm_vehicle, far_images, far_points, settings.use_linear_svm);
    LOG_DEBUG("PVOHOG.PO/VO:" << this->track_id << ": Computing near feature");
    near_value = compute_sthog(params.desc, params.svm_people, near_images, near_points, settings.use_linear_svm);
  }

  std::vector<float> pvo;
  pvo.push_back(0.); // Initial "other" score.

  double const cur_gsd = gsd_vec[0];
  if (cur_gsd < settings.gsd_threshold)
  {
    far_value = adjust_value(far_value, settings.score_sigma_vehicle, cur_gsd, settings.gsd_threshold);
  }
  else
  {
    near_value = adjust_value(near_value, settings.score_sigma_people, cur_gsd, settings.gsd_threshold);
  }

  if (settings.dbg_write_outputs)
  {
    // TODO: Write out image chip collage for training.
  }

  pvo.push_back(far_value);
  pvo.push_back(near_value);

  for (std::vector<float>::const_iterator itr = pvo.begin(); itr != pvo.end(); itr++)
  {
    if (vnl_math::isnan(*itr))
    {
      LOG_WARN("PVOHOG.PVO:" << this->track_id << ": Found NaN values in the raw scores; skipping");
      return std::vector<double>();
    }
  }

  LOG_INFO("PVOHOG.PO/VO:" << this->track_id << ": Input values: "
      "P: " << pvo[2] << " "
      "V: " << pvo[1] << " "
      "O: " << pvo[0]);

  double p;
  double v;
  double o;

  if (settings.use_histogram_conversion)
  {
    double const r1_p = score_from_histogram(1, p, pvo[1]);
    double const r1_v = score_from_histogram(1, v, pvo[1]);
    double const r1_o = score_from_histogram(1, o, pvo[1]);

    double const r2_p = score_from_histogram(2, p, pvo[2]);
    double const r2_v = score_from_histogram(2, v, pvo[2]);
    double const r2_o = score_from_histogram(2, o, pvo[2]);

    p = r1_p * r2_p;
    v = r1_v * r2_v;
    o = r1_o * r2_o;
  }
  else
  {
    p = pvo[2];
    v = pvo[1];
    o = pvo[0];
  }

  LOG_INFO("PVOHOG.PO/VO:" << this->track_id << ": Raw PVO: "
      "P: " << p << " "
      "V: " << v << " "
      "O: " << o);

  if (settings.frame_end <= settings.frame_start)
  {
    // Single point.

    return make_pvo(p, v, o);
  }
  else
  {
    // Temporal filtering.
    std::vector<double> temporal;
    temporal.push_back(p);
    temporal.push_back(v);
    temporal.push_back(o);

    this->scores.push_back(temporal);

    double const score = (cur_gsd < settings.gsd_threshold) ? p : v;
    this->scores_gsd.push_back(score);

    // Are we on the last frame?
    unsigned const current_frame = this->current_time.frame_number();
    unsigned const start_frame = this->start_time.frame_number();

    if (settings.frame_end < (current_frame - start_frame + settings.frame_step))
    {
      size_t median_idx = 0;
      size_t min_diff = std::numeric_limits<size_t>::max();

      for (size_t i = 0; i < this->scores_gsd.size(); ++i)
      {
        double const cur_value = this->scores_gsd[i];
        size_t gt_count = 0;
        size_t lt_count = 0;

        BOOST_FOREACH (double const& value, this->scores_gsd)
        {
          gt_count += (value >= cur_value);
          lt_count += (value <= cur_value);
        }

        size_t const diff = abs(static_cast<int>(gt_count) - static_cast<int>(lt_count));
        if (diff < min_diff)
        {
          min_diff = diff;
          median_idx = i;
        }
      }

      std::vector<double> const& pvo_score = this->scores[median_idx];

      LOG_INFO("PVOHOG.PO/VO:" << this->track_id << ": Temporally filtered PVO: "
          "P: " << pvo_score[0] << " "
          "V: " << pvo_score[1] << " "
          "O: " << pvo_score[2]);

      return make_pvo(pvo_score[0], pvo_score[1], pvo_score[2]);
    }
  }

  return std::vector<double>();
}


// ------------------------------------------------------------------
std::vector<pvohog_track_data::pvohog_frame_data>::const_reverse_iterator
pvohog_track_data
::find_frame_span(unsigned* nframes) const
{
  unsigned total_frames = 0;
  *nframes = 0;
  std::vector<pvohog_frame_data>::const_reverse_iterator start;
  std::vector<pvohog_frame_data>::const_reverse_iterator i;
  std::vector<pvohog_frame_data>::const_reverse_iterator const i_end = this->frames.rend();

  pvohog_settings const& settings = params.settings;

  for (i = this->frames.rbegin(), start = i_end;
       (i != i_end) && (*nframes < settings.samples);
       ++i, ++total_frames)
  {
    if (total_frames < settings.skip_frames)
    {
      continue;
    }
    if (!i->valid)
    {
      *nframes = 0;
      start = i_end;
    }
    else
    {
      if (start == i_end)
      {
        start = i;
      }
      ++*nframes;
    }
  }

  return start;
}


// ------------------------------------------------------------------
bool
pvohog_track_data
::generate_clfr_desc(track_sptr const track, frame_data_sptr const frame)
{
  bool result = false;

  this->current_time = track->last_state()->time_;

  this->frames.push_back(pvohog_frame_data());
  pvohog_frame_data& fdata = this->frames.back();
  track_state_sptr const state = track->last_state();
  image_object_sptr image_obj;

  if (!state->image_object(image_obj))
  {
    fdata.valid = false;
    return result;
  }

  if (!frame->was_gsd_set() && params.use_gsd)
  {
    fdata.valid = false;
    return result;
  }

  const vgl_box_2d<unsigned>& bbox = image_obj->get_bbox();

  vil_image_view<vxl_byte> const& image = frame->image_8u_gray();
  vgl_box_2d<unsigned> const frame_bbox(0, image.ni(), 0, image.nj());
  vgl_box_2d<int> const frame_bbox_int(0, image.ni(), 0, image.nj());
  vgl_box_2d<int> const bbox_int( bbox.min_x(), bbox.max_x(), bbox.min_y(), bbox.max_y() );

  unsigned const iarea = vgl_intersection(bbox_int, frame_bbox_int).volume();
  unsigned const oarea = bbox.volume();

  pvohog_settings const& settings = params.settings;

  if (iarea != oarea)
  {
    fdata.valid = false;
    return result;
  }

  fdata.valid = true;
  fdata.ts = this->current_time;

  unsigned const width = bbox.width();
  unsigned const height = bbox.height();

  cv::Point p1( ( bbox.min_x() + bbox.max_x() - settings.bbox_size ) / 2,
                ( bbox.min_y() + bbox.max_y() - settings.bbox_size ) / 2 );

  cv::Point p2( p1.x + settings.bbox_size,
                p1.y + settings.bbox_size );

  cv::Mat gray_img;
  forced_cv_conversion(image, gray_img);

  fdata.bbox = cv::Rect( p1, p2 );
  fdata.image = gray_img;
  fdata.position.x = fdata.bbox.x;
  fdata.position.y = fdata.bbox.y;

  if (frame->was_gsd_set())
  {
    fdata.gsd = frame->average_gsd();
  }
  else
  {
    fdata.gsd = settings.backup_gsd;
  }

  double const dx = state->vel_[0];
  double const dy = state->vel_[1];

  fdata.velocity = std::sqrt(dx * dx + dy * dy);
  fdata.angle = ( dy == 0 && dx == 0 ? 0 : std::atan2(-dy, dx) );
  fdata.bbox_area = oarea;

  if (width && height)
  {
    fdata.bbox_aspect = (width > height) ? (height / width) : (width / height);
  }
  else
  {
    fdata.bbox_aspect = 1.0;
  }

  fdata.fg_coverage = image_obj->get_area() / oarea;

  std::vector<cv::Mat> far_images;
  std::vector<cv::Point> far_points;
  std::vector<cv::Mat> near_images;
  std::vector<cv::Point> near_points;

  unsigned const chip_width = settings.bbox_size + 2 * settings.bbox_border;
  cv::Size const chip_size(chip_width, chip_width);
  cv::Point const ul_chip_pt(settings.bbox_border, settings.bbox_border);

  cv::Point2d const to_center(settings.bbox_size/2, settings.bbox_size/2);

  double const gsd = fdata.gsd;
  double const vehicle_scale = gsd / settings.gsd_scale_vehicle;
  double const people_scale = gsd / settings.gsd_scale_people;

  cv::Point2d center = fdata.position;
  center += to_center;
  double const angle = fdata.angle;
  bool const flip = (angle < vnl_math::pi_over_2) || (vnl_math::pi_over_2 < angle);

  cv::Mat const vehicle_chip = extract_chip(fdata.image, center, chip_size, angle, vehicle_scale, flip);
  cv::Mat const people_chip = extract_chip(fdata.image, center, chip_size, angle, people_scale, flip);

  far_images.push_back(vehicle_chip);
  near_images.push_back(people_chip);

  far_points.push_back(ul_chip_pt);
  near_points.push_back(ul_chip_pt);

  double v_value;
  double p_value;

  if (settings.use_spatial_search)
  {
    v_value = spatial_search(params.desc_vehicle, far_images, settings.dt_threshold_vehicle);
    p_value = spatial_search(params.desc_people, near_images, settings.dt_threshold_people);
  }
  else
  {
    v_value = compute_sthog(params.desc, params.svm_vehicle, far_images, far_points, settings.use_linear_svm);
    p_value = compute_sthog(params.desc, params.svm_people, near_images, near_points, settings.use_linear_svm);
  }

  fdata.v_classification = v_value * -1.;
  fdata.p_classification = p_value * -1.;
  return true;
}


// ------------------------------------------------------------------
double
value_from_histogram(histogram_type const hist, double min, double max, double score)
{
  if (score < min)
  {
    return hist[0];
  }
  if (score > max)
  {
    return hist[PVOHOG_HIST_SIZE - 1];
  }
  double const range = max - min;
  double const bucket_size = range / PVOHOG_HIST_SIZE;
  double const offset = score - min;
  return hist[size_t(offset / bucket_size)];
}

static void normalize_pvo(double* p, double* v, double* o);

std::vector<double>
make_pvo(double p, double v, double o)
{
  normalize_pvo(&p, &v, &o);

  std::vector<double> pvo_score;

  pvo_score.push_back(p);
  pvo_score.push_back(v);
  pvo_score.push_back(o);

  return pvo_score;
}


// ------------------------------------------------------------------
cv::Mat
extract_chip(cv::Mat const& image, cv::Point2d const& position, cv::Size const& size, double angle, double scale, bool flip)
{
  static double const deg_per_rad = 180. / vnl_math::pi;

  cv::Mat transform = cv::getRotationMatrix2D(position, -angle * deg_per_rad, scale);

  // Translate to the center of the chip.
  transform.at<double>(0, 2) += size.width / 2.0 - position.x;
  transform.at<double>(1, 2) += size.height / 2.0 - position.y;

  if (flip)
  {
    transform.row(1) *= -1.;
    transform.at<double>(1, 2) += size.height;
  }

  cv::Mat output(size, CV_8UC1);
  cv::warpAffine(image, output, transform, size, cv::INTER_LINEAR, cv::BORDER_REFLECT);

  return output;
}


// ------------------------------------------------------------------
void
scale_images(std::vector<cv::Mat> const& images, std::vector<cv::Point> const& points, std::vector<double> const& gsds,
             double scale_factor, unsigned bbox_size, std::vector<cv::Mat>& scaled_images, std::vector<cv::Point>& scaled_points)
{
  unsigned const nframes = images.size();
  for (unsigned i = 0; i < nframes; ++i)
  {
    double scale = gsds[i] / scale_factor;
    if ((scale < 0.1) || (10. < scale))
    {
      scale = 1.0;
    }

    cv::Mat image;
    cv::resize(images[i], image, cv::Size(0, 0), scale, scale);
    scaled_images.push_back(image);

    double const v = bbox_size * (scale - 1) / 2.0;
    cv::Point const& cur_pt = points[i];
    cv::Point pt(cur_pt.x * scale + v,
                 cur_pt.y * scale + v);

    int const max_size = image.cols - bbox_size - 1;
    pt.x = std::max(0, std::min(max_size, pt.x));
    pt.y = std::max(0, std::min(max_size, pt.y));

    scaled_points.push_back(pt);
  }
}


// ------------------------------------------------------------------
double
spatial_search(boost::shared_ptr<sthog_descriptor> const& desc, std::vector<cv::Mat>& images, double threshold)
{
  std::vector<cv::Rect> found;

  desc->sthog_descriptor::detectMultiScale(images, found, threshold,
    cv::Size(8, 8), cv::Size(8, 8), 1, 1, 2); // XXX: Some magic constants.

  std::vector<double> const hit_scores = desc->get_hit_scores();
  std::vector<double>::const_iterator const min = std::min_element(hit_scores.begin(), hit_scores.end());

  if (min == hit_scores.end())
  {
    return threshold;
  }
  else
  {
    return *min;
  }
}


// ------------------------------------------------------------------
double
compute_sthog(boost::shared_ptr<sthog_descriptor> const& desc, boost::shared_ptr<KwSVM> const& svm,
              std::vector<cv::Mat>& images, std::vector<cv::Point> const& points, bool use_linear_svm)
{
  desc->compute(images, points);
  std::vector<float> feature;
  desc->get_sthog_descriptor(feature, points[0]);

  if (use_linear_svm)
  {
    return svm->predict_linear_svm(feature);
  }
  else
  {
    cv::Mat feature_mat = cv::Mat(1, feature.size(), CV_32FC1);

    for (size_t i = 0; i < feature.size(); ++i)
    {
      feature_mat.at<float>(0, i) = feature[i];
    }

    std::vector<float> scores;
    return svm->predict(feature_mat, scores);
  }
}


// ------------------------------------------------------------------
double
adjust_value(double value, double sigma, double gsd, double threshold)
{
  if ((0 < value) && (1e-6 < fabs(sigma)))
  {
    double d = gsd - threshold;
    d *= d;
    d = exp(-d / (2. * sigma * sigma));

    return value * d;
  }

  return value;
}


// ------------------------------------------------------------------
void
normalize_pvo(double* p, double* v, double* o)
{
  double const sum = *p + *v + *o;
  if (sum <= 0)
  {
    *p = *v = *o = 1.0 / 3.0;
  }
  else
  {
    *p /= sum;
    *v /= sum;
    *o /= sum;
  }
}

}
