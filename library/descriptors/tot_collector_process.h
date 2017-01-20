/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_collector_process_h_
#define vidtk_tot_collector_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/pvo_probability.h>
#include <tracking_data/raw_descriptor.h>
#include <tracking_data/track.h>

#include <descriptors/online_track_type_classifier.h>

#include <vector>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

/// The tot_collector_process consumes multiple raw descriptors, and
/// transforms their values into an object classification or other
/// quanity based on a variety of methods.
class tot_collector_process
  : public process
{
public:
  typedef vidtk::pvo_probability tot_probability;
  typedef vidtk::pvo_probability_sptr tot_probability_sptr;
  typedef tot_collector_process self_type;

  tot_collector_process( std::string const& name );
  ~tot_collector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief Set input tracks.
  void set_descriptors( raw_descriptor::vector_t const& descriptors);
  VIDTK_INPUT_PORT( set_descriptors, raw_descriptor::vector_t const& );

  /// \brief Set input descriptors.
  void set_source_tracks( vidtk::track::vector_t const& trks );
  VIDTK_INPUT_PORT( set_source_tracks, vidtk::track::vector_t const& );

  /// \brief Set input descriptors.
  void set_source_gsd( double gsd );
  VIDTK_INPUT_PORT( set_source_gsd, double );

  /// \brief The output tracks with tot probabilities attached.
  vidtk::track::vector_t output_tracks() const;
  VIDTK_OUTPUT_PORT( vidtk::track::vector_t, output_tracks );

private:

  // Parameters
  config_block config_;
  enum { DISABLED, PROB_PRODUCT, ADABOOST } mode_;

  // Inputs
  raw_descriptor::vector_t input_des_;
  vidtk::track::vector_t input_trks_;
  double input_gsd_;

  // Output
  vidtk::track::vector_t output_trks_;

  // Internal classifier class
  boost::scoped_ptr< online_track_type_classifier > classifier_;

};


} // end namespace vidtk


#endif // vidtk_tot_collector_process_h_
