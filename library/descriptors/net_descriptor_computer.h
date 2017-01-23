/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_net_descriptor_computer_h_
#define vidtk_net_descriptor_computer_h_

#include <descriptors/online_descriptor_generator.h>

#include <utilities/track_demultiplexer.h>

#include <vector>
#include <map>
#include <set>

namespace vidtk
{

/// \brief Settings for the net_descriptor_computer class
///
/// These settings are declared in macro form to prevent code duplication
/// in multiple places (e.g. initialization, file parsing, etc...).
#define settings_macro( add_param, add_array, add_enumr ) \
  add_array( \
    to_merge, \
    descriptor_id_t, 0, \
    "", \
    "List of descriptor IDs to merge into a single new descriptor." ); \
  add_array( \
    merge_weights, \
    double, 0, \
    "", \
    "List of descriptor weights to use when merging descriptors." ); \
  add_param( \
    normalize_merge, \
    bool, \
    false, \
    "Whether or not to re-normalize merged descriptors." ); \
  add_param( \
    sampling_rate, \
    unsigned, \
    0, \
    "Sampling rate to attempt to create net descriptors. Will only " \
    "check at end of track if set to 0." ); \
  add_param( \
    merged_id, \
    descriptor_id_t, \
    "joint_descriptor", \
    "New descriptor ID for net descriptors." ); \

init_external_settings3( net_descriptor_computer_settings, settings_macro )

#undef settings_macro

/// \brief Raw descriptor filter.
///
/// This class can perform different filtering operations on descriptors
/// including merging, removal, and duplication based on descriptor IDs.
class net_descriptor_computer
{

public:

  typedef raw_descriptor descriptor_t;
  typedef descriptor_sptr descriptor_sptr_t;
  typedef raw_descriptor::vector_t descriptor_vec_t;

  /// Constructor.
  net_descriptor_computer();

  /// Destructor.
  virtual ~net_descriptor_computer();

  /// Set any settings for this descriptor filter.
  virtual bool configure( const net_descriptor_computer_settings& settings );

  /// Filter input descriptors.
  virtual void compute( const track::vector_t& tracks, descriptor_vec_t& descriptors );

private:

  typedef std::map< descriptor_id_t, descriptor_vec_t > descriptor_map_t;
  typedef std::pair< unsigned, descriptor_map_t > descriptor_data_t;
  typedef std::map< track::track_id_t, descriptor_data_t > track_descriptor_map_t;

  // Precomputed flags
  bool enabled_;

  // Stored track demuxer for repeated operation
  track_demultiplexer track_demuxer_;

  // Generated map of IDs to duplicate
  track_descriptor_map_t descriptors_;

  // Internal settings copy
  net_descriptor_computer_settings settings_;

  // Generate a joint descriptor if possible
  descriptor_sptr_t generate_net_descriptor( const descriptor_map_t& descriptors );
};

} // end namespace vidtk

#endif // net_descriptor_computer_h_
