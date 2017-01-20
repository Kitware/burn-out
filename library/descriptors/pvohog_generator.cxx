/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/pvohog_generator.h>

#include "pvohog_track_data.h"
#include "sthog_descriptor.h"

#include <vul/vul_file.h>

#include <numeric>

#include <logger/logger.h>

VIDTK_LOGGER("pvohog_generator_cxx");


namespace vidtk
{

/*
 * Ah, OpenCV. Where ostream operators are formatted debugging output, not
 * easily reversible serialization operations. We store vnl_int_2 in the actual
 * settings object and then use this wonderful mechanism below to convert from
 * the settings object into the sthog_profile class.
 */
namespace
{

template <typename T>
struct profile_conv
{
  profile_conv(T const& t) : t_(t) {}

  T conv() const { return t_; }

  template <typename U>
  U conv() const;

  T t_;
};

template <>
struct profile_conv<vnl_int_2>
{
  profile_conv(vnl_int_2 const& t) : t_(t) {}

  cv::Size conv() const
  {
    cv::Size sz;
    sz.width = t_[0];
    sz.height = t_[1];
    return sz;
  }

  vnl_int_2 t_;
};

}

pvohog_generator
::pvohog_generator()
{
}

typedef std::vector< double > histogram_type;

static bool
normalize_histograms(histogram_type p, histogram_type v, histogram_type o);

bool pvohog_generator
::configure( const external_settings& input_settings )
{
  pvohog_settings const* desc_settings =
    dynamic_cast<pvohog_settings const*>(&input_settings);

  if( !desc_settings )
  {
    return false;
  }

  pvohog_settings const& settings = *desc_settings;

  params_.reset( new pvohog_parameters );
  params_->settings = settings;

  vidtk::sthog_profile profile;

#define copy_to_profile(name, type, def, desc) \
  profile.name##_ = profile_conv<type>(settings.name).conv()

  pvohog_desc_settings(copy_to_profile)

#undef copy_to_profile

  if (settings.frame_start < settings.skip_frames)
  {
    LOG_WARN("Setting skip frames to " << settings.frame_start << " (it must not be larger than frame_start)");
    params_->settings.skip_frames = settings.frame_start;
  }

  if ((!settings.use_linear_svm || !settings.use_rotation_chips) && settings.use_spatial_search)
  {
    LOG_WARN("Cannot use spatial search without a linear svm and rotation chips; disabling");
    params_->settings.use_spatial_search = false;
  }

  if (settings.dbg_write_outputs &&
      (!vul_file::exists(settings.dbg_output_path) ||
       !vul_file::is_directory(settings.dbg_output_path)))
  {
    LOG_ERROR("Debug outputs requested, but " << settings.dbg_output_path
      << " is not suitable (must be a directory)");
    return false;
  }

  params_->desc_vehicle.reset(new sthog_descriptor(profile));
  params_->desc_people.reset(new sthog_descriptor(profile));

  size_t const feature_size = params_->desc_vehicle->get_sthog_descriptor_size();
  bool have_vo = false;
  bool have_po = false;

  if (settings.model_file_vehicle.empty() ||
      !vul_file::exists(settings.model_file_vehicle))
  {
    LOG_WARN("Not using a V/O model file: " << settings.model_file_vehicle);
  }
  else
  {
    params_->svm_vehicle.reset(new KwSVM());

    if (settings.use_linear_svm)
    {
      LOG_INFO("Loading linear V/O model file: " << settings.model_file_vehicle);

      if (params_->svm_vehicle->read_linear_model(settings.model_file_vehicle.c_str(), feature_size + 1))
      {
        have_vo = true;
      }
      else
      {
        LOG_ERROR("Failed to read linear model file: " << settings.model_file_vehicle);
      }

      if (settings.use_spatial_search)
      {
        LOG_INFO("Using a V/O model file");
        params_->desc_vehicle->load_svm_ocv_model(settings.model_file_vehicle);
      }
    }
    else
    {
      LOG_INFO("Loading V/O model file: " << settings.model_file_vehicle);
      params_->svm_vehicle->load(settings.model_file_vehicle.c_str());

      if (params_->svm_vehicle->get_support_vector_count())
      {
        LOG_INFO("Using a V/O model file");
        have_vo = true;
      }
    }
  }

  if (settings.model_file_people.empty() ||
      !vul_file::exists(settings.model_file_people))
  {
    LOG_WARN("Not using a P/O model file: " << settings.model_file_people);
  }
  else
  {
    params_->svm_people.reset(new KwSVM());

    if (settings.use_linear_svm)
    {
      LOG_INFO("Loading linear P/O model file: " << settings.model_file_people);

      if (params_->svm_people->read_linear_model(settings.model_file_people.c_str(), feature_size + 1))
      {
        LOG_INFO("Using a P/O model file");
        have_po = true;
      }
      else
      {
        LOG_ERROR("Failed to read linear model file: " << settings.model_file_people);
      }

      if (settings.use_spatial_search)
      {
        params_->desc_people->load_svm_ocv_model(settings.model_file_people);
      }
    }
    else
    {
      LOG_INFO("Loading P/O model file: " << settings.model_file_people);
      params_->svm_people->load(settings.model_file_people.c_str());

      if (params_->svm_people->get_support_vector_count())
      {
        LOG_INFO("Using a P/O model file");
        have_po = true;
      }
    }
  }

  params_->use_vo_po = have_vo && have_po;
  params_->use_sthog = params_->use_vo_po;

  if (settings.use_histogram_conversion)
  {
    bool ok = true;

    if (!params_->use_vo_po)
    {
      ok = ok &&
        normalize_histograms(params_->settings.histogram_conversion_r0_p,
                             params_->settings.histogram_conversion_r0_v,
                             params_->settings.histogram_conversion_r0_o);
    }
    ok = ok &&
      normalize_histograms(params_->settings.histogram_conversion_r1_p,
                           params_->settings.histogram_conversion_r1_v,
                           params_->settings.histogram_conversion_r1_o);
    ok = ok &&
      normalize_histograms(params_->settings.histogram_conversion_r2_p,
                           params_->settings.histogram_conversion_r2_v,
                           params_->settings.histogram_conversion_r2_o);

    if (!ok)
    {
      LOG_ERROR("Failed to read histogram conversion tables");
      return false;
    }
  }

  if (!params_->use_vo_po)
  {
    if (settings.model_file.empty() ||
        !vul_file::exists(settings.model_file))
    {
      LOG_ERROR("The model file cannot be found: " << settings.model_file);
      return false;
    }

    LOG_INFO("Loading PVO model file...");

    params_->svm.reset(new KwSVM());
    params_->svm->load(settings.model_file.c_str());

    int version;
    cv::FileStorage fs( settings.model_file.c_str(), cv::FileStorage::READ );
    fs["version"] >> version;

    if (version != 2)
    {
      LOG_ERROR("Unsupported PVO model file version: " << version);
      return false;
    }

    int flags;
    fs["feature_flag"] >> flags;

    enum model_flags
    {
      flag_sthog           = 0x0001,
      flag_gsd             = 0x0002,
      flag_velocity        = 0x0004,
      flag_velocity_std    = 0x0008,
      flag_bbox_area       = 0x0010,
      flag_bbox_area_std   = 0x0020,
      flag_bbox_aspect     = 0x0040,
      flag_bbox_aspect_std = 0x0080,
      flag_fg_coverage     = 0x0100,
      flag_fg_coverage_std = 0x0200,
      flag_no_pca          = 0x0400,

      // No such flag, but it makes things consistent.
      flag_gsd_std         = 0x0000
    };

#define check_flag(flag) \
    params_->use_##flag = flags & flag_##flag

    check_flag(sthog);
    check_flag(gsd);
    check_flag(gsd_std);
    check_flag(velocity);
    check_flag(velocity_std);
    check_flag(bbox_area);
    check_flag(bbox_area_std);
    check_flag(bbox_aspect);
    check_flag(bbox_aspect_std);
    check_flag(fg_coverage);
    check_flag(fg_coverage_std);
    check_flag(no_pca);

#undef check_flag

    if (params_->use_sthog && !params_->use_no_pca)
    {
      fs["PCA_eigenvectors"] >> params_->pca.eigenvectors;
      fs["PCA_eigenvalues"] >> params_->pca.eigenvalues;
      fs["PCA_mean"] >> params_->pca.mean;
    }

    // Magic number; "any flag other than sthog"? Was this before or after the
    // PCA flag?
    if (flags & ~0x01)
    {
      fs["trk_feat_mean_std"] >> params_->track_feats;
    }
  }

  // Set internal generator run options
  generator_settings running_options;
  running_options.thread_count = desc_settings->thread_count;
  running_options.sampling_rate = desc_settings->sampling_rate;
  this->set_operating_mode( running_options );

  // Generate sthog profile class
  params_->desc.reset(new sthog_descriptor(profile));

  return true;
}


external_settings* pvohog_generator
::create_settings()
{
  return new pvohog_settings;
}


track_data_sptr pvohog_generator
::new_track_routine( track_sptr const& new_track )
{
  return track_data_sptr(new pvohog_track_data(*params_.get(), new_track));
}

// Called for each track per frame in order to update models and output descriptors
bool pvohog_generator
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  pvohog_track_data* pvohog_data = dynamic_cast<pvohog_track_data*>(track_data.get());

  if (!pvohog_data)
  {
    LOG_ERROR("pvohog: Failed to get track data");
    return false;
  }

  bool const updated = pvohog_data->update(active_track, current_frame());

  if (updated)
  {
    push_results(pvohog_data);
  }

  return true;
}


bool pvohog_generator
::terminated_track_routine( track_sptr const& finished_track,
                            track_data_sptr track_data )
{
  pvohog_track_data* pvohog_data = dynamic_cast<pvohog_track_data*>(track_data.get());

  if (!pvohog_data)
  {
    LOG_ERROR("pvohog: Failed to get track data");
    return false;
  }

  bool const updated = pvohog_data->update(finished_track, current_frame());

  if (updated)
  {
    push_results(pvohog_data);
  }

  return true;
}

void
pvohog_generator
::push_results(pvohog_track_data* data)
{
  descriptor_sptr const descriptor = data->make_descriptor();

  if (!descriptor)
  {
    return;
  }

  add_outgoing_descriptor(descriptor);
}

static bool
normalize_histogram(histogram_type hist);
static void
normalize_pvo(double& p, double& v, double& o);

bool
normalize_histograms(histogram_type p, histogram_type v, histogram_type o)
{
  bool ok = true;

  ok = ok && normalize_histogram(p);
  ok = ok && normalize_histogram(v);
  ok = ok && normalize_histogram(o);

  for (unsigned i = 0; ok && (i < PVOHOG_HIST_SIZE); ++i)
  {
    normalize_pvo(p[i], v[i], o[i]);
  }

  return ok;
}

bool
normalize_histogram(histogram_type hist)
{
  double const sum = std::accumulate(hist.begin(), hist.end(), 0.);
  if (sum <= 0)
  {
    return false;
  }

  for (size_t i = 0; i < PVOHOG_HIST_SIZE; ++i)
  {
    hist[i] /= sum;
  }

  return true;
}

void
normalize_pvo(double& p, double& v, double& o)
{
  double const sum = p + v + o;
  if (sum <= 0)
  {
    p = v = o = 1.0 / 3.0;
  }
  else
  {
    p /= sum;
    v /= sum;
    o /= sum;
  }
}

} // end namespace vidtk
