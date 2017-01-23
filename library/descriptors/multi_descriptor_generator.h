/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_multi_descriptor_generator
#define vidtk_multi_descriptor_generator

#include <tracking_data/raw_descriptor.h>
#include <tracking_data/frame_data.h>

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/online_descriptor_helper_functions.h>

#include <vector>

namespace vidtk
{


#define settings_macro( add_param ) \
  add_param( \
    thread_count, \
    unsigned, \
    10, \
    "How many threads to use across all descriptors." ); \
  add_param( \
    run_in_safe_mode, \
    bool, \
    true, \
    "Run a sanity check over descriptor values (NaN, continuity, " \
    "timestamps, etc.)" ); \
  add_param( \
    buffer_smart_ptrs_only, \
    bool, \
    true, \
    "Buffer smart pointers or frame contents" );

init_external_settings( multi_descriptor_settings, settings_macro )

#undef settings_macro

/// \brief Group of online descriptor generators.
///
/// A class which runs multiple online_descriptor_generators in an
/// optimal configuration (for efficiency reasons). Ensures that no
/// actions are repeated for multiple descriptors that don't have to
/// be (track management), that descriptor tasks are spread out
/// amongst threads efficiently, and that the maximum thread pool size
/// is never exceeded.
///
/// Example Typical usage:
///
///   First, all descriptors should be added to this module
///
///   - configure( options )
///   - add_descriptor( descriptor_sptr_1 )
///   - add_descriptor( descriptor_sptr_2 )
///
///   Next, when running online:
///
///   - set_input_frame( verbose_frame )
///   - call step()
///   - get_descriptors() will return any descriptors terminating on
///     the current frame, or since the call to the last step function
///
class multi_descriptor_generator : public online_descriptor_generator
{

public:

  typedef boost::shared_ptr< online_descriptor_generator > generator_sptr;

  /// Constructor
  multi_descriptor_generator() {}

  /// Destructor
  virtual ~multi_descriptor_generator() {}

  /// Configure multi-generator settings
  bool configure( const external_settings& options );
  virtual external_settings* create_settings();

  /// Add another online descriptor module. Don't add yourself. Only add
  /// new descriptors before we start processing.
  virtual bool add_generator( generator_sptr& descriptor_module );

  /// Flush all partially completely descriptors, and remove all attached descriptor
  /// modules (restoring this class to it's defaults).
  virtual bool clear();

  /// Attempts to restart all attached descriptor modules to their defaults.
  virtual bool reset_all();

protected:

  /// Called once per frame, call step on all descriptors.
  ///
  /// The return code determined similarly to the original vidtk
  /// "step" function, returning false indicated that an error
  /// occurred somewhere in the function call, though it should
  /// probably be replaced by an exception eventually since it is
  /// mostly unused.
  ///
  /// If this occurs when running a descriptor, the program will
  /// either halt in debug mode, or attempt to restart the descriptor
  /// in release mode (after performing as much work as possible on
  /// all other descriptors).
  virtual bool frame_update_routine();

  /// Call new_track_routine for all descriptors
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  /// Terminate all descriptors for a certain track
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

  /// Called after processing all tracks
  virtual bool final_update_routine();

private:

  /// Externally set options
  multi_descriptor_settings settings_;

  /// Data we want to store for each descriptor
  struct generator_data
  {
    // Operating mode of this descriptor
    generator_settings run_time_options;

    // Sampling rate for this descriptor
    unsigned int sample_rate;

    // Frame counter for calling descriptor with the correct samp rate
    unsigned int counter;

    // Should we process tracks with this descriptor?
    bool process_tracks;
  };

  /// Data we want to store for each track
  struct mdg_track_data : track_data_storage
  {
    // Data for each descriptor
    std::vector< track_data_sptr > descriptor_track_data;
  };

  /// Information stored about each descriptor
  std::vector< generator_data > descriptor_info_;

  /// Descriptors we are using
  std::vector< generator_sptr > descriptors_;

  /// Distribute update tasks amongst worker threads as required for all descriptors
  virtual void formulate_tasks( const track_vector_t& active_tracks,
                                const track_vector_t& terminated_tracks,
                                task_vector_t& task_list );
};

} // end namespace vidtk

#endif
