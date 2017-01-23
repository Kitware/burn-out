/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_descriptor_filter_h_
#define vidtk_descriptor_filter_h_

#include <descriptors/online_descriptor_generator.h>

#include <vector>
#include <map>
#include <set>

namespace vidtk
{

/// \brief Settings for the descriptor_filter class
///
/// These settings are declared in macro form to prevent code duplication
/// in multiple places (e.g. initialization, file parsing, etc...).
#define settings_macro( add_param, add_array, add_enumr ) \
  add_array( \
    to_duplicate, \
    descriptor_id_t, 0, \
    "", \
    "List of descriptors to duplicate. Can contain the same ID more than " \
    "once. Must be the same size as the duplicate_ids parameter." ); \
  add_array( \
    duplicate_ids, \
    descriptor_id_t, 0, \
    "", \
    "New descriptor IDs for the descriptors we want to duplicate." ); \
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
    merged_id, \
    descriptor_id_t, \
    "joint_descriptor", \
    "New descriptor ID for merged descriptors." ); \
  add_array( \
    to_remove, \
    descriptor_id_t, 0, \
    "", \
    "List of descriptor IDs to remove after all other operations." ); \

init_external_settings3( descriptor_filter_settings, settings_macro )

#undef settings_macro

/// \brief Raw descriptor filter.
///
/// This class can perform different filtering operations on descriptors
/// including merging, removal, and duplication based on descriptor IDs.
class descriptor_filter
{

public:

  typedef raw_descriptor descriptor_t;
  typedef descriptor_sptr descriptor_sptr_t;
  typedef raw_descriptor::vector_t descriptor_vec_t;

  /// Constructor.
  descriptor_filter();

  /// Destructor.
  virtual ~descriptor_filter();

  /// Set any settings for this descriptor filter.
  virtual bool configure( const descriptor_filter_settings& settings );

  /// Filter input descriptors.
  virtual void filter( descriptor_vec_t& descriptors );

private:

  // Precomputed flags
  bool any_filter_enabled_;
  bool merge_filter_enabled_;

  // Generated map of IDs to duplicate
  std::map< descriptor_id_t, std::vector< descriptor_id_t > > dup_map_;

  // Generated map of IDs to remove
  std::set< descriptor_id_t > rem_set_;

  // Copy of user settings
  descriptor_filter_settings settings_;
};

} // end namespace vidtk

#endif // descriptor_filter_h_
