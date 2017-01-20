/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "klt_mutex.h"
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{

//
// pointer to our instance
//
klt_mutex_t klt_mutex::s_instance = klt_mutex_t();



// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
klt_mutex::klt_mutex()
{
}


klt_mutex::~klt_mutex()
{
}


// ----------------------------------------------------------------
/** Get the instance
 *
 *
 */
klt_mutex_t klt_mutex
::instance()
{
  static boost::mutex local_lock;          // synchronization lock

  if (s_instance)
  {
    return s_instance;
  }

  boost::lock_guard<boost::mutex> lock(local_lock);
  if (!s_instance)
  {
    // create new object
    s_instance = klt_mutex_t(new klt_mutex);
  }

  return s_instance;
}


} // end namespace
