/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_online_descriptor_generator_h_
#define vidtk_online_descriptor_generator_h_

#include <tracking_data/raw_descriptor.h>
#include <tracking_data/frame_data.h>

#include <descriptors/online_descriptor_buffer.h>
#include <descriptors/online_descriptor_thread_system.h>
#include <descriptors/online_descriptor_helper_functions.h>

#include <utilities/config_block.h>
#include <utilities/external_settings.h>

#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

namespace vidtk
{


/**
 * @brief Settings which all descriptor generators share.
 *
 * These options should be set by implementers of descriptor generators,
 * and should only be made public to the users of these processes at the
 * implementers discretion.
 */
struct generator_settings
{
  /// How many threads we want to utilize?
  unsigned thread_count;

  /// Sampling rate at which all step functions will be called (all step
  /// functions defined by the implementer will be called every 1 in x
  /// frames).
  unsigned sampling_rate;

  /// How many input frames do we want to internally buffer? (can be 0)
  unsigned frame_buffer_length;

  /// [Advanced] Do we want to run in safe mode? Safe mode spents a little
  /// bit of extra time checking descriptor outputs. NaN values will be
  /// checked for, in addition to making sure that the history vector of
  /// each descriptor is continuous and that the descriptor start/end
  /// timestamps are set.
  bool run_in_safe_mode;

  /// [Advanced] Do we want to keep a record of which tracks we've seen?
  /// If this is disabled, the user defined track step functions will not
  /// be called.
  bool process_tracks;

  /// [Advanced] Do we want to only buffer the inputted smart points to
  /// frames or the actual frame contents if buffering is enabled?
  bool buffer_smart_ptrs_only;

  /// [Advanced] Should we append a modality string to the end of each
  /// descriptor identifier representing the modality that the descriptor
  /// was computed in?
  bool append_modality_to_id;

  /// Default constructor
  generator_settings()
   : thread_count( 1 ),
     sampling_rate( 1 ),
     frame_buffer_length( 1 ),
     run_in_safe_mode( true ),
     process_tracks( true ),
     buffer_smart_ptrs_only( true ),
     append_modality_to_id( false )
  {}
};



/**
 * @brief A helper struct used for online descriptor generators.
 *
 * When defining a descriptor generator, we can create a derived version of
 * this struct for containing any data which we want to internally store for
 * every active track individually (i.e. there will be one instances of this
 * struct allocated per track).
 */
struct track_data_storage
{
  track_data_storage() {}
  virtual ~track_data_storage() {}
};

typedef boost::shared_ptr< track_data_storage > track_data_sptr;



/**
 * @brief Forward declaration of the multi-descriptor generator class.
 *
 * This is a special-case of a descriptor generator which computes multiple
 * other descriptor modules in parallel, more efficiently then they would
 * if run individually.
 */
class multi_descriptor_generator;



/**
 * @brief Abstract base class for online descriptors that use time-series data to
 * compute measurements around tracks, which are formatted into some
 * raw descriptor output.
 *
 * Raw descriptors are typically used as intermediate values before
 * either database storage or classification for higher level
 * recognition-based activities. These descriptor generators can
 * optionally be configured to use and/or buffer input frame
 * contents. See other descriptors within this folder for example
 * implementations.
 *
 * Inherited processes must define:
 *
 *   - A class constructor/destructor
 *   - Any data that should be allocated for each track
 *   - A frame_step function, called once per frame before any track updates
 *   - A new track routine, called whenever we receive a new track
 *   - A track step function, called once per frame for each active track
 *   - A track termination routine, called when a track terminates
 *   - Optionally: A configure function to set any internal settings
 *
 * The external usage of descriptor generators derived from this class is very
 * similar to that of a standard vidtk process. But for build and other
 * reasons it was decide to leave the implementation seperate from a regular
 * process for now.
 *
 * Example Typical usage:
 *
 *   - set_input_frame( verbose_frame )
 *   - call step()
 *   - get_descriptors() will return any descriptors terminating on
 *     the current frame, or since the call to the last step function
 *   - get_events() will return any managed events terminating on the
 *     current frame, or since the call to the last step function
 *
 * \msc
 process, descriptor_generator, example_descriptor;

 process=>descriptor_generator [ label = "set_input_frame()" ];
 process=>descriptor_generator [ label = "step()" ];

 descriptor_generator=>example_descriptor [ label = "frame_update_routine()", URL="\ref frame_update_routine()" ];
 descriptor_generator=>example_descriptor [ label = "new_track_routine()", URL="\ref new_track_routine()" ];
 descriptor_generator=>example_descriptor [ label = "terminated_track_routine()", URL="\ref terminated_track_routine()" ];
 descriptor_generator=>descriptor_generator [ label = "final_update_routine()", URL="\ref final_update_routine()" ];

 process=>descriptor_generator [ label = "get_descriptors()", URL="\ref get_descriptors()" ];
 process<<descriptor_generator [ label = "list of descriptors" ];
 process=>descriptor_generator [ label = "get_events()", URL="\ref get_events()" ];
 process<<descriptor_generator [ label = "list of events" ];

 * \endmsc
 */
class online_descriptor_generator
{

public:

  /// Functions available to external users of derived generator classes:
  ///                  [ Should not be overridden ]

  typedef boost::shared_ptr< frame_data > frame_sptr;
  typedef frame_sptr frame_sptr_t;
  typedef online_descriptor_generator generator_t;
  typedef track::vector_t track_vector_t;

  /// Constructor
  online_descriptor_generator();

  /// Destructor
  virtual ~online_descriptor_generator();

  /// Set verbose input frame
  void set_input_frame( const frame_sptr_t& frame );

  /// The step function, triggers ingestation of data and descriptor generation
  /// for the supplied round of inputs.
  bool step();

  /// Retrieve a list of any descriptors generated since the last time
  /// step was called.
  raw_descriptor::vector_t get_descriptors();

  /// Set any configuration settings for this generator. Returns status.
  virtual bool configure( external_settings const& settings )
  {
    (void) settings;
    return true;
  }

  /// Attempts to restart a descriptor. This will clear all internal
  /// settings, and start the descriptor generator as if it was new.
  bool reset();

  /// Manually terminate all currently active tracks, calling the track termination
  /// function for the descriptor. \sa terminated_track_routine()
  virtual bool terminate_all_tracks();

  /**
   * @brief Create descriptor specific settings object.
   *
   * This method returns a pointer to the descriptor specific settings
   * object. The object is allocated from the heap and must be deleted
   * by the caller.
   *
   * The derived class must implement this method to create the
   * settings object specific to the concrete descriptor generator.
   *
   * @return Pointer to new settings object.
   */
  virtual external_settings* create_settings() = 0;

protected:

  // Functions which should be defined by descriptor generator writers:

  /**
   * @brief Handle new frame.
   *
   * Called at the start of every step function call (i.e. every-time
   * we receive a new frame). Useful when we want to use some global
   * properties to aid in descriptor formation, or for dealing with
   * descriptors involving multiple tracks or scene contents alone. Is
   * not required to be implemented by derived classes.
   *
   * Returning \b false will terminate the controlling process and
   * cause the pipeline to fail.
   *
   * @return \b true if processing completed o.k., \b false otherwise.
   */
  virtual bool frame_update_routine()
  {
    return true;
  }

  /**
   * @brief Handle new track.
   *
   * Called every time we receive a track for the first time. Must
   * return a pointer to any data we might want to store for this
   * track over time, for its entire duration. This data will be
   * passed back to the descriptor through the track_update_routine()
   * with its associated track.
   *
   * An uninitialized pointer may be returned (pointing to nothing)
   * if nothing is needed for this track.
   *
   * @param new_track        new track on this frame.
   * @return A pointer must be returned.
   */
  virtual track_data_sptr new_track_routine( track_sptr const& new_track )
  {
    (void) new_track; // not used
    return track_data_sptr(); // uninitialized pointer - default return
  }

  /**
   * @brief Handle existing track update.
   *
   * Called for every active track per frame. As arguments, a smart
   * pointer to the track and the track-specific data set by the
   * new_track_routine is supplied.
   *
   * Note that there may not be a state on the active track for this call.
   *
   * @param active_track   current state of active track
   * @param track_data     data associated with the track
   *
   * @return \b true for successful completion; \b false if error encountered.
   */
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data )
  {
    (void) active_track;
    (void) track_data;
    return true;
  }

  /**
   * @brief Handle terminated track.
   *
   * Called for every track that is terminated in case we want to
   * output any descriptors near the end of a track, or do any
   * cleanup. The track specific data supplied with this call was
   * created in the new_track_routine() call.
   *
   * @param finished_track    track that has just terminated
   * @param track_data        data associated with the track
   *
   * @return \b true for successful completion; \b false if error encountered.
   */
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data )
  {
    (void) finished_track;
    (void) track_data;
    return true;
  }

  /**
   * @brief Called after processing all tracks.
   *
   * This method is called after all tracks for a frame have been
   * processed. The descriptor can do any processing needed at then
   * end of a frame.
   *
   * @return \b true for successful completion; \b false if error encountered.
   */
  virtual bool final_update_routine()
  {
    return true;
  }

  /// All for resetting interals.
  virtual bool reset_generator()
  {
    return true;
  }


  // Common helper functions which can be used by derived classes:


  /// Get a pointer to most recent (the latest) frame provided as input.
  frame_sptr_t const& current_frame();

  /// Get the ith frame in our buffer, where 0 is the oldest frame.
  frame_sptr_t const& get_frame( unsigned i );

  /// How many frames are currently in our buffer.
  unsigned frame_buffer_length();

  /// Attach a descriptor to the outgoing descriptors list. The list
  /// of descriptors will be cleared before the next frame is
  /// presented.
  bool add_outgoing_descriptor( const descriptor_sptr& descriptor );

  /// Configure the internal descriptor operating settings.
  bool set_operating_mode( const generator_settings& settings );

  /// Get stored operating mode configuration for this generator.
  generator_settings const& operating_mode();

  /// Throw an exception and report a critical error.
  void report_error( const std::string& error );

private:

  typedef unsigned track_id_t;
  typedef online_descriptor_thread_task task_t;
  typedef online_descriptor_thread_system thread_sys_t;
  typedef online_descriptor_buffer buffer_t;
  typedef std::map< track_id_t, track_data_sptr > data_storage_t;
  typedef data_storage_t::iterator data_storage_itr_t;
  typedef std::vector< task_t > task_vector_t;

  /// Descriptor operating mode settings.
  generator_settings running_options_;

  /// Stored internal track data [Track ID->Smart Ptr]
  data_storage_t track_data_;

  /// Data pretaining to track and frame level inputs and outputs
  class io_data;
  boost::scoped_ptr< io_data > io_data_;

  /// Previously stored frames, if enabled
  boost::scoped_ptr< buffer_t > frame_buffer_;

  /// Data pretraining to threading, if enabled
  boost::scoped_ptr< thread_sys_t > thread_sys_;

  /// Create track update tasks - should only be overriden by specialized processes
  /// which manage multiple descriptors
  virtual void formulate_tasks( const track_vector_t& active_tracks,
                                const track_vector_t& terminated_tracks,
                                task_vector_t& task_list );

  // Friend the multi-descriptor generator, a special case of a descriptor generator
  // which can compute multiple other descriptor generators in parallel
  friend class multi_descriptor_generator;
  friend class online_descriptor_thread_task;
};


} // end namespace vidtk

#endif
