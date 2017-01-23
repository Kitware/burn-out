/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detector_factory.h"

#include <vxl_config.h>
#include "detector_implementation_factory.h"

// 1) add Include for detector variants
#include "motion_detector_pipeline.h"
#include "motion_and_saliency_detector_pipeline.h"
#include "detection_reader_pipeline.h"

#ifdef USE_OPENCV
#include "ocv_hog_detector_pipeline.h"
#endif

#ifdef USE_CAFFE
#include "cnn_detector_pipeline.h"
#include "motion_and_cnn_detector_pipeline.h"
#endif

namespace vidtk
{

// Instructions on adding detector variants:
//
// A) add detector to appropriate group below
// B) Go to templates/detector_super_process_instances.cxx and follow instructions
//

// ================================================================
// 2) Make a list of all variants to be supported
//
// The entry specifies the name of the variant that is to appear in
// the config, followed by the class name.
//
//  <config name>,        <class name>
#define VXL_VARIANTS( MACRO )                                           \
  MACRO( motion_detector,     motion_detector_pipeline );               \
  MACRO( motion_and_saliency, motion_and_saliency_detector_pipeline );  \
  MACRO( detection_reader,    detection_reader_pipeline );

#define OPENCV_VARIANTS( MACRO )                                        \
  MACRO( ocv_hog_detector,    ocv_hog_detector_pipeline );

#define CAFFE_VARIANTS( MACRO )                                         \
  MACRO( cnn_detector,        cnn_detector_pipeline );                  \
  MACRO( motion_and_cnn,      motion_and_cnn_detector_pipeline );       \


// ==================================================================
// Should not need to change anything below here
// ==================================================================
// Compile List of All Variants
#ifndef USE_OPENCV
  #undef OPENCV_VARIANTS
  #define OPENCV_VARIANTS( MACRO )
#endif

#ifndef USE_CAFFE
  #undef CAFFE_VARIANTS
  #define CAFFE_VARIANTS( MACRO )
#endif

#define VARIANTS( MACRO )                                               \
  VXL_VARIANTS( MACRO )                                                 \
  OPENCV_VARIANTS( MACRO )                                              \
  CAFFE_VARIANTS( MACRO )


/**
 * Instantiate concrete factory class to create pipeline.  This is
 * needed to allow factories to be kept in a collection. Collections
 * can only hold a single type, in this case, the base class. Ideally,
 * a factory class could be templated on the class it is to create,
 * but then they will not go easily into a collection.
 *
 * This may not look like the cleanest implementation, but it keeps
 * the individual object detector factories hidden from the interface,
 * while only exposing the implementation factory interface.
 *
 * To add a new object detector variant, add the variant specification
 * to the list above.
 *
 */
#define DETECTOR_FACTORY(N, TYPE)                                       \
template < class PixType >                                              \
class TYPE ## _factory                                                  \
  : public detector_implementation_factory < PixType >                  \
{                                                                       \
public:                                                                 \
  TYPE ## _factory( std::string const& name )                           \
  : detector_implementation_factory < PixType >( name )                 \
  { }                                                                   \
                                                                        \
  virtual vidtk::config_block params()                                  \
  {                                                                     \
    config_block blk;                                                   \
                                                                        \
    TYPE< PixType >* factory = new TYPE< PixType >( this->m_name + "/params"  ); \
    blk = factory->params();                                            \
    delete factory;                                                     \
                                                                        \
    return blk;                                                         \
  }                                                                     \
                                                                        \
  virtual typename detector_implementation_base< PixType >::sptr        \
  create_detector_implementation( std::string const& n )                \
  {                                                                     \
    TYPE< PixType >* detector_impl = new TYPE< PixType >( n );          \
                                                                        \
    return typename detector_implementation_base< PixType >::sptr( detector_impl ); \
                                                                        \
  }                                                                     \
}

// ----------------------------------------------------------------
// Instantiate factories
  VARIANTS( DETECTOR_FACTORY );

// ----------------------------------------------------------------
// Instantiate list of varmints for each image pixel type.
//
template<>
void
detector_factory< vxl_byte >
::add_variants_for_type()
{
#define add_variant( N, T )   this->m_variants[ # N ].reset( new T ##_factory < vxl_byte >( # N ) )
  VARIANTS( add_variant );
#undef add_variant
}


template<>
void
detector_factory< vxl_uint_16 >
::add_variants_for_type()
{
#define add_variant( N, T )   this->m_variants[ # N ].reset( new T ##_factory < vxl_uint_16 >( # N ) )
  VARIANTS( add_variant );
#undef add_variant
}

} // end namespace vidtk
