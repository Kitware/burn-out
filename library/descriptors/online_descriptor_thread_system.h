/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_online_descriptor_thread_system_h_
#define vidtk_online_descriptor_thread_system_h_

#include <tracking_data/track.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <queue>
#include <set>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace vidtk
{

// Forward declarations of required classes
class online_descriptor_generator;
struct track_data_storage;

/// \brief A task given to the online_descriptor_thread_system to process.
///
/// While potentially unsafe if used incorrectly in the future, in the current
/// threading update stage of online descriptor generators, the locations of each
/// of these components will be unchanged and guaranteed to still be in scope for
/// the duration of the existance of each object, which is why all data within it is
/// saved as just a simple pointer. This saves a needless extra O( tracks*descriptor )
/// extra shared_ptr copies per frame by just passing regular ptrs to these data
/// contents along.
class online_descriptor_thread_task
{
public:

  typedef boost::shared_ptr< track_data_storage > track_data_sptr;

  enum task_activity
  {
    UPDATE_ACTIVE_TRACK = 0,
    TERMINATE_TRACK = 1
  };

  explicit online_descriptor_thread_task(
    const task_activity act,
    const track_sptr& trk,
    online_descriptor_generator& gen,
    track_data_sptr& trk_data );

  virtual ~online_descriptor_thread_task() {}

  virtual bool execute();

private:

  // Type of task to perform
  task_activity action;

  // A pointer to the track (if used)
  const track_sptr* track_ref;

  // The descriptor generator to use
  online_descriptor_generator* generator_ref;

  // The track data to use for the task (if required)
  track_data_sptr* data_ref;

};

/// Variables used for the online_descriptor_generator threading sub-system.
class online_descriptor_thread_system
{
public:

  typedef online_descriptor_thread_task task_t;

  explicit online_descriptor_thread_system( unsigned thread_count );
  virtual ~online_descriptor_thread_system();

  // Process a list of tasks
  bool process_tasks( const std::vector< task_t >& tasks );

  // Cancel all running threads and deallocate all utilized memory
  void shutdown();

private:

  // Thread status definition
  enum thread_status
  {
    THREAD_WAITING = 0,
    THREAD_TASKED = 1,
    THREAD_RUNNING = 2,
    THREAD_FINISHED_W_SUCCESS = 3,
    THREAD_FINISHED_W_FAILURE = 4,
    THREAD_TERMINATED = 5
  };

  // Number of threads
  unsigned thread_count_;

  // Each of these will have thread_count instances
  boost::thread_group threads_;
  boost::condition_variable *thread_cond_var_;
  boost::mutex *thread_muti_;
  thread_status *thread_status_;
  std::queue< task_t > *thread_worker_queues_;

  // Worker thread main execution definition
  void worker_thread( unsigned thread_id );
};

} // end namespace vidtk

#endif
