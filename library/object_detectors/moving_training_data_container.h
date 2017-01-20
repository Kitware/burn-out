/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_moving_training_data_container_h_
#define vidtk_moving_training_data_container_h_

#include <utilities/external_settings.h>

#include <vector>
#include <iostream>

namespace vidtk
{


#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    max_entries, \
    unsigned, \
    0, \
    "Max number of entries (capacity) of the container." ) \
  add_param( \
    features_per_entry, \
    unsigned, \
    0, \
    "Number of features per each entry in the container." ) \
  add_enumr( \
    insert_mode, \
    (FIFO) (RANDOM), \
    RANDOM, \
    "How are new entries added to the buffer?" )

init_external_settings3( moving_training_data_settings, settings_macro );

#undef settings_macro


/// \brief A data structure for storing a fixed amount of feature vectors,
/// where new feature vectors are added in an online fashion.
///
/// When the capacity of the buffer is reached, new data can either be inserted
/// into random positions, or it can replace the oldest entries stored within
/// the buffer. Note, this buffer class is designed for cases where we do not
/// care about the order of feature vectors in the buffer, therefore any function
/// call which accesses internal raw data is not guaranteed to be in any particular
/// order (ie, entry 0 in the entries function may or may not be the last input).
template< typename FeatureType >
class moving_training_data_container
{
public:

  typedef FeatureType feature_t;
  typedef std::vector< feature_t > data_block_t;

  moving_training_data_container();
  ~moving_training_data_container() {}

  /// Set any settings for this buffer.
  void configure( const moving_training_data_settings& settings );

  /// Keep the container capacity the same, but remove all stored entries.
  void reset();

  /// The number of entries inserted into the container.
  unsigned size() const;

  /// Is the container empty?
  bool empty() const;

  /// The capacity of the container.
  unsigned capacity() const;

  /// The pre-set feature count per entry in the container.
  unsigned feature_count() const;

  /// A pointer to the start of a specified entry in the container.
  feature_t const *entry( unsigned id ) const;

  /// A pointer to the full data array stored in the container.
  data_block_t const& raw_data() const;

  /// Insert a single entry into the array via a pointer to the start of the
  /// input data.
  void insert( const feature_t* new_entry );

  /// Insert a vector of data entries into the array, via a list of pointers
  /// to the start of of each entry.
  void insert( const std::vector< const feature_t* >& new_entries );

  /// Insert a sequential block of data into the array. The size of the
  /// input block must be a multiple of the size of each entry.
  void insert( const data_block_t& new_data_block );

  /// Insert a vector of data entries into the array. The size of each
  /// vector must be the same as the specified feature count per entry.
  void insert( const std::vector< data_block_t >& new_entries );

private:

  // The number of entries stored in the container
  unsigned stored_entry_count_;

  // End of the last inserted block of data
  unsigned current_position_;

  // Max number of entries (capacity) of the container.
  unsigned max_entries_;

  // Number of features per each entry in the container.
  unsigned features_per_entry_;

  // How are new entries added to the buffer?
  moving_training_data_settings::insert_mode_t insert_mode_;

  // A vector of all training data stored in this buffer. All data is
  // stored in a sequential array.
  data_block_t data_;

};


/// Output operator for moving_training_data_container class.
template< typename FeatureType >
std::ostream& operator<<( std::ostream& out,
  const moving_training_data_container< FeatureType >& data );


} // end namespace vidtk

#endif // vidtk_moving_training_data_container_h_
