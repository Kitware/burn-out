/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RESOURCE_POOL_EXCEPTION_H_
#define _VIDTK_RESOURCE_POOL_EXCEPTION_H_

#include <stdexcept>

namespace vidtk {
namespace ns_resource_pool {

// ----------------------------------------------------------------
/*! \brief Resource pool exception
 *
 * This class represents the internal exception used to signal errors
 * and other exceptional conditions in the resource pool.
 */

class resource_pool_exception
  : public std::runtime_error
{
public:
  explicit resource_pool_exception( std::string const& msg )
    : runtime_error( msg ) { }

  explicit resource_pool_exception( char const* msg )
    : runtime_error( msg ) { }

  virtual ~resource_pool_exception() throw() { }


};

} } // end namespace

#endif /* _VIDTK_RESOURCE_POOL_EXCEPTION_H_ */
