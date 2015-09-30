/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */



#ifndef _LOGGER_NS_LOGGER_FACTORY
#define _LOGGER_NS_LOGGER_FACTORY

#include <logger/vidtk_logger.h>

#include <vcl_string.h>
#include <boost/noncopyable.hpp>


namespace vidtk {
namespace logger_ns {

// ----------------------------------------------------------------
/** Factory for underlying logger.
 *
 * This class is the abstract base class that adapts the VIDTK logger
 * to the underlying logging system.
 *
 * An object of this type can be created early in the program
 * execution (i.e. static initializer time), which is before the
 * initialize method is called.
 */
  class logger_factory
    : private boost::noncopyable
{
public:
  logger_factory(const char * name);
  virtual ~logger_factory();

  virtual int initialize(vcl_string const& config_file) = 0;

  virtual vidtk_logger_sptr get_logger( const char * const name ) = 0;
  vidtk_logger_sptr get_logger( vcl_string const& name )
  {  return get_logger (name.c_str()); }

  vcl_string const& get_name() const;


private:
  vcl_string m_name;


}; // end class logger_factory


} // end namespace
} // end namespace

#endif /* _LOGGER_NS_LOGGER_FACTORY */
