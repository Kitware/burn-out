/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_DETECTOR_IMPLEMENTATION_FACTORY_H_
#define _VIDTK_DETECTOR_IMPLEMENTATION_FACTORY_H_

#include <object_detectors/detector_implementation_base.h>

#include <utilities/config_block.h>

#include <boost/shared_ptr.hpp>


namespace vidtk {


// ----------------------------------------------------------------
/*! \brief  Base class for pipeline factories.
 *
 * This is a templated base class for pipeline factories. Factories
 * are light weight class that creates a specific pipeline. The type
 * of pipeline created is specified by the template parameter.
 *
 * @tparam PixType Pixel type for images being passed.
 */
template < class PixType >
class detector_implementation_factory
{
public:
  detector_implementation_factory( std::string const& name )
    : m_name(name)
  {
  }


  virtual ~detector_implementation_factory()
  {
  }


  /*! \brief  Get parameters for this detector implementation.
   *
   * This method returns the config block containing all config
   * process parameters for this detector implementation.
   *
   * @return Pipeline config.
   */
  virtual vidtk::config_block params() = 0;


  /*! \brief Create detector pipeline.
   *
   * This method creates a detector implementation. Concrete classes
   * create a specific detector and return the base class pointer.
   *
   * @return A pointer to the detector implementation is returned.
   */
  virtual typename detector_implementation_base< PixType >::sptr
  create_detector_implementation( std::string const& name ) = 0;

protected:
  std::string m_name;

}; // end class detector_implementation_factory

} // end namespace vidtk

#endif /* _VIDTK_DETECTOR_IMPLEMENTATION_FACTORY_H_ */
