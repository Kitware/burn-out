/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_training_thread_h_
#define vidtk_training_thread_h_

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <queue>
#include <list>
#include <set>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace vidtk
{

/// A task given to the training_thread class to perform.
class training_thread_task
{
public:

  training_thread_task() : is_valid_(true) {}
  virtual ~training_thread_task() {}

  /// Execute this user-defined task.
  virtual bool execute() = 0;

protected:

  /// Is this thread still valid? Do we even still care about it's outputs?
  ///
  /// This is a function meant to be used internally by the execute function,
  /// to determine if the task should even continue processing. It is set by
  /// the training_thread class, when forced_reset is called. This function
  /// can also be used to check whether or not any external outputs should
  /// still be set at the implementers discretion.
  bool is_still_valid() { return is_valid_; }

private:

  // Internal validity flag.
  bool is_valid_;

  // Mark thread as invalid. Only meant to be called by training_thread class.
  void mark_as_invalid() { is_valid_ = false; }

  // Friend training thread so it can use the above function.
  friend class training_thread;

};


/// A smart pointer to a training_thread_task.
typedef boost::shared_ptr< training_thread_task > training_thread_task_sptr;


/// \brief A class which contains an external thread for the purpose of training
/// a classifier (or performing some other related task) outside of a standard
/// sequential processing pipeline.
///
/// If it is able to, this class will always keep this thread busy with the most
/// recently added task. When a hard reset is called, this class has the possibility
/// of allocating more than 1 thread for a short period of time (as old tasks
/// are marked as invalid and finish off).
class training_thread
{
public:

  training_thread();
  virtual ~training_thread();

  /// Non-blocking call to execute the task, but only if the internal thread
  /// is not occupied. If it is occupied, and a new task comes along, this
  /// task will be dropped and not called unless block_until_finished is called.
  ///
  /// To force deterministic behavior, block_until_finished can be called directly
  /// after this function to complete all outstanding tasks and to ensure that
  /// the next task inputted to this function will be called.
  void execute_if_able( training_thread_task_sptr& task );

  /// Blocks until all thread tasks have finished, and the thread is unoccupied.
  void block_until_finished();

  /// Non-blocking call to immediately make a new thread available which will
  /// process all future tasks. If the current internal thread is busy, this
  /// reset function will not wait for the old thread to finish, but instead
  /// make a new thread for all future tasks. It assumed that all previously
  /// entered tasks are invalid, and we don't care about any of their outputs
  /// any more.
  void forced_reset();

private:

  typedef unsigned index_t;
  typedef boost::thread thread_t;
  typedef boost::shared_ptr< thread_t > thread_sptr_t;
  typedef boost::condition_variable cond_var_t;
  typedef boost::mutex mutex_t;
  typedef boost::unique_lock< mutex_t > lock_t;
  typedef std::list< training_thread_task_sptr > task_queue_t;

  enum thread_status_t
  {
    THREAD_RUNNING_TASKED = 0,
    THREAD_RUNNING_UNTASKED = 1,
    THREAD_FAILED = 2,
    THREAD_ABANDONED = 3,
    THREAD_TERMINATED = 4
  };

  struct thread_info_t
  {
    index_t index;
    thread_sptr_t thread;
    cond_var_t in_cond_var;
    cond_var_t out_cond_var;
    thread_status_t status;
    mutex_t mutex;
    task_queue_t task_queue;
  };

  typedef boost::shared_ptr< thread_info_t > thread_info_sptr_t;

  // Current (active) thread executing inputted tasks
  thread_info_sptr_t current_thread_;

  // Latest launched thread ID
  index_t current_thread_index_;

  // Past threads which we no longer care anything about, and simply want
  // to forget about as soon as possible
  std::queue< thread_info_sptr_t > old_threads_;

  // Internal thread job
  void thread_job( thread_info_sptr_t info );

  // Launch a new current thread
  void launch_new_thread( index_t id );
};


} // end namespace vidtk

#endif // vidtk_training_thread_h_
