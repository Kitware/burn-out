/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <logger/logger_factory.h>


namespace vidtk {
namespace logger_ns {


// ----------------------------------------------------------------
/**
 *
 *
 */
logger_factory
::logger_factory(const char * name)
  :m_name(name)
{

}


logger_factory
:: ~logger_factory()
{

}


vcl_string const & logger_factory
::get_name() const
{
  return m_name;
}


} // end namespace
} // end namespace
