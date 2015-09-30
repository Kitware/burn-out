/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _KLT_MUTEX_included_H_
#define _KLT_MUTEX_included_H_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

namespace vidtk {

class klt_mutex;
typedef boost::shared_ptr<klt_mutex> klt_mutex_t;


// ----------------------------------------------------------------
/** Singleton mutex for KLT interactions
 *
 *
 */
class klt_mutex
{
public:
  static klt_mutex_t instance();
  virtual ~klt_mutex();

  void lock() { lock_.lock(); }
  void unlock() { lock_.unlock(); }
  boost::mutex & get_lock() { return lock_; }


protected:
  klt_mutex();


private:
  boost::mutex lock_;          // synchronization lock


  static klt_mutex_t s_instance;
}; // end class klt_mutex

} // end namespace

#endif /* _KLT_MUTEX_included_H_ */

