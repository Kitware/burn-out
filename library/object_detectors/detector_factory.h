/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _vidtk_detector_factory_h_
#define _vidtk_detector_factory_h_


#include <object_detectors/detector_implementation_factory.h>
#include <object_detectors/detector_super_process.h>

#include <utilities/config_block.h>

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Detector factory interface class.
 *
 * This class manages a set of factory classes that create object
 * detectors. The individual factory classes are registered by the
 * constructor, but could be registered at run time using a dynamic
 * plugin approach.
 *
 * The purpose of this class is to create and return an object
 * detector super process that encapsulates a single object detector
 * pipeline.
 *
 \msc
DnT_sp, det_factory, impl_factory, det_impl, det_pipeline

DnT_sp->det_factory [ label = "<<CREATE>>" ];
det_factory=>det_factory [ label = "add_variants_for_type<T>( )" ];
--- [ label = "Repeat for each pixel type" ];
DnT_sp<-det_factory;

--- [ label = "get parameters for all variants" ];
DnT_sp=>det_factory [ label = "params()" ];
det_factory=>impl_factory [ label = "get_params()" ];
impl_factory=>det_impl [ label = "<<CREATE>>" ];
impl_factory=>det_impl [ label = "get_params()" ];
impl_factory=>det_impl [ label = "<<DESTROY>>" ];
--- [ label = "for all pipeline factories registered" ];
DnT_sp<=det_factory [ label = "config block" ];

--- [ label = "instantiating a detector" ];
DnT_sp=>det_factory [ label = "create_detector( config_block )" ];
det_factory=>det_implementation_factory [ label = "create_detector_implementation()" ];
det_implementation_factory=>det_impl [ label = "<<CREATE>>" ];
det_factory<=det_implementation_factory [ label = "det_impl" ];
det_factory=>det_impl [ label = "set_params(config_block)" ];
det_factory=>det_impl [ label = "setup_pipeline(pipeline)" ];
--- xxx set_params() on pipeline xxx ---
DnT_sp<=det_factory [ label = "det_super_process" ];

 \endmsc
 *
 */
  template< class PixType >
class detector_factory
{
public:

  detector_factory( std::string const& n );
  ~detector_factory();

  /*! \brief Return name of this factory.
   *
   * This method returns the name of this factory. This name is used in
   * a similar manner as a process name.
   *
   * @return Name of this factory.
   */
  std::string const& name() const;

  /**
   * \brief Get parameters for all detector variants.
   *
   * This method returns the config block for all the detectors that
   * can have been registered.
   *
   * @return Config block for the detector to be created.
   */
  config_block params() const;

  /**
   * \brief Create detector process object.
   *
   * This method creates and configures the specified detector super
   * process. The configuration block contains the tag specifying
   * which detector is to be created and all the other config values.
   * The configuration block must contain the following entries:
   *
   * variant = foo    # where foo is the name of the detector to create
   * foo:* = <stuff>  # configuration entries for that detector.
   * bar:* = <stuff>  # configuration entries for other detecectors not selected.
   *
   * @param blk Configuration block
   *
   * @throw config_block_parse_error If there was an error configuring
   * the detector pipeline.
   *
   * @return Pointer to new detector process object.
   */
  process_smart_pointer< detector_super_process< PixType > >
  create_detector(vidtk::config_block const& blk);

private:
  /*! \brief Add variants for a single image template type.
   *
   * This method adds all variants for a specific image pixel
   * type. All template instantiations are explicitly defined.
   */
  void add_variants_for_type();

  std::string m_name;

  /// List of available detector variants
  typedef vidtk::detector_implementation_factory < PixType > factory_t;
  typedef boost::shared_ptr< factory_t > factory_sptr_t;
  typedef std::map < std::string, factory_sptr_t > map_t;
  map_t m_variants;

  /// The pipeline factory for the selected detector variant.
  typename detector_implementation_base< PixType >::sptr m_pipeline_factory;
  pipeline_sptr m_pipeline;

};

} // namespace vidtk

#endif // _vidtk_detector_factory_h_
