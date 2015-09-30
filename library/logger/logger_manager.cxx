/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <logger/logger_manager.h>

#ifdef USE_LOG4CXX
#include <logger/logger_factory_log4cxx.h>
#endif

#include <logger/logger_factory_mini_logger.h>

#include <vul/vul_file.h>
#include <vul/vul_arg.h>
#include <vcl_cstdlib.h>
#include <vcl_cstring.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>


/*
 * Note: This must be thread safe.
 *
 * Also: In order to make this work, it must be possible to create
 * loggers before the manager has been initialized. This means that
 * the initialization is flexible, adaptive and has a reasonable
 * default.
 */

namespace vidtk {

//
// Pointer to our single instance.
//
logger_manager * logger_manager::s_instance = 0;



// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
logger_manager
::logger_manager()
  : m_logFactory(0),
    m_initialized(false)
{
  // Need to create a factory class at this point because loggers
  // are created by static initializers. we can wait no longer until
  // we provide a method for creating these loggers.
  //
  // This implies that we need to read a config or something so we
  // can determine which one to create.

#ifdef USE_LOG4CXX

  char * factory = vcl_getenv("VIDTK_LOGGER_FACTORY");
  if ((0 != factory) && (vcl_strcmp(factory, "MINI_LOGGER") == 0) )
  {
    m_logFactory = new ::vidtk::logger_ns::logger_factory_mini_logger();
  }
  else // default factory
  {
    m_logFactory = new ::vidtk::logger_ns::logger_factory_log4cxx();
  }

#else

  m_logFactory = new ::vidtk::logger_ns::logger_factory_mini_logger();

#endif

  /*
   * An alternate approach to managing the list of factories is to
   * have each factory register with the manager at load time (static
   * constructor). This would give the manager a more dynamic approach
   * to logger factories. The problem arises when we allow loggers to
   * be created at static constructor time (which we do). There is no
   * portable and reliable way to make sure the factories register
   * before the first logger is created.  :-(
   *
   *
   */

}


logger_manager
::~logger_manager()
{

}


// ----------------------------------------------------------------
/** Get singleton instance.
 *
 *
 */
logger_manager * logger_manager
::instance()
{
  static boost::mutex local_lock;          // synchronization lock

  if (0 != s_instance)
  {
    return s_instance;
  }

  boost::lock_guard<boost::mutex> lock(local_lock);
  if (0 == s_instance)
  {
    // create new object
    s_instance = new logger_manager();
  }

  return s_instance;
}


// ----------------------------------------------------------------
/* Initialize.
 *
 * Initialize the logging subsystem using the commane line
 * arguments.
 *
 * The following items are initialized:
 * - application name - from argv[0] (override by option --logger-app xxx)
 * - application instance name - from option --logger-app-instance xxx
 * - system name - from gethostname()
 * - config file name - from option --logger-config xxx
 *
 * @param[in] argc - number of elements in argv,
 * @param[in] argv - vector of erguments.
 *
 * @retval 0 - initialization completed o.k.
 * @retval -1 - error in initialization.
 */
int logger_manager
::initialize(int argc, char ** argv)
{
  vcl_string config_file;

  if (argc > 0)
  {
    // Get the name of the application program from the executable file name
    vcl_string app = vul_file::strip_directory(argv[0]);
    m_applicationName = app;

    // parse argv for allowable options.
    vul_arg < vcl_string > app_name_arg("--logger-app", "Application name. overrides argv[0].");
    vul_arg < vcl_string > app_instance_arg( "--logger-app-instance", "Application instance name.");
    vul_arg < vcl_string > config_file_arg( "--logger-config", "Configuration file name.");

    vul_arg_parse( argc, argv );

    if ( ! app_name_arg().empty())
    {
      m_applicationName = app_name_arg();
    }

    m_applicationInstanceName = app_instance_arg();
    config_file = config_file_arg();
  }

#ifdef HAVE_GETHOSTNAME

  // Get name of the system we are running on
  ///@todo use portable call for gethostname()
  char host_buffer[1024];
  gethostname (host_buffer, sizeof host_buffer);
  m_systemName = host_buffer;

#endif

  // initialise adapter to do real logging.
  m_logFactory->initialize (config_file);

  return (0);
}


// ----------------------------------------------------------------
/** Get address of logger object.
 *
 *
 */
vidtk_logger_sptr logger_manager
::get_logger( char const* name )
{
  return ::vidtk::logger_manager::instance()->m_logFactory->get_logger(name);
}


vidtk_logger_sptr logger_manager
::get_logger( vcl_string const& name )
{
  return get_logger(name.c_str() );
}


vcl_string const& logger_manager
::get_application_name() const
{
  return m_applicationName;
}


vcl_string const& logger_manager
::get_application_instance_name() const
{
  return m_applicationInstanceName;
}


vcl_string const& logger_manager
::get_system_name() const
{
  return m_systemName;
}


vcl_string const& logger_manager
::get_factory_name() const
{
  return m_logFactory->get_name();
}


// ----------------------------------------------------------------
/* Set location strings.
 *
 *
 */
void logger_manager
::set_application_name (vcl_string const& name)
{
  m_applicationName = name;
}


void logger_manager
::set_application_instance_name (vcl_string const& name)
{
  m_applicationInstanceName = name;
}


void logger_manager
::set_system_name (vcl_string const& name)
{
  m_systemName = name;
}


} // end namespace
