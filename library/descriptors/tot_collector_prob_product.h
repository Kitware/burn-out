/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_collector_prob_product_h_
#define vidtk_tot_collector_prob_product_h_

#include <descriptors/online_track_type_classifier.h>

namespace vidtk
{

/// Consolidate multiple descriptors into a net TOT output for a track
/// via multiplying multiple PVO values together.
class tot_collector_prob_product : public online_track_type_classifier
{

public:

  tot_collector_prob_product() {}
  ~tot_collector_prob_product() {}

  /// Given a tack object and a list of corresponding descriptors for that
  /// track for the current state, derive an object class for the given track.
  object_class_sptr_t classify( const track_sptr_t& track_object,
                                const raw_descriptor::vector_t& descriptors );

  /// Given a list of descriptors of equivalent length, multiply their
  /// scores together to formulate a new descriptor.
  descriptor_sptr compute_product( const raw_descriptor::vector_t& descriptors );
};

} // end namespace vidtk

#endif // vidtk_tot_collector_prob_product_h_
