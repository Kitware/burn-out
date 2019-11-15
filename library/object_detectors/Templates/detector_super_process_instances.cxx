/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/detector_factory.txx>
#include <object_detectors/detector_implementation_base.txx>
#include <object_detectors/detector_super_process.txx>

// 1) add include for detector impl
#include <object_detectors/motion_detector_pipeline.txx>
#include <object_detectors/motion_and_saliency_detector_pipeline.txx>
#include <object_detectors/detection_reader_pipeline.txx>

#ifdef USE_CAFFE
#include <object_detectors/cnn_detector_pipeline.txx>
#include <object_detectors/motion_and_cnn_detector_pipeline.txx>
#endif

// ================================================================
// 2) add detector variant class to list of detectors
#define DETECTOR_LIST_VXL(T)                                             \
template class vidtk::motion_detector_pipeline< T >;                     \
template class vidtk::motion_and_saliency_detector_pipeline< T >;        \
template class vidtk::detection_reader_pipeline< T >;                    \

#define DETECTOR_LIST_CFE(T)                                             \
template class vidtk::cnn_detector_pipeline< T >;                        \
template class vidtk::motion_and_cnn_detector_pipeline< T >;             \


#ifndef USE_CAFFE
  #undef DETECTOR_LIST_CFE
  #define DETECTOR_LIST_CFE(T)
#endif

#define DETECTOR_LIST(T)                                                 \
  DETECTOR_LIST_VXL(T)                                                   \
  DETECTOR_LIST_CFE(T)


// ================================================================
// 3) list of support classes
#define SUPPORT_CLASSES(T)                                              \
  template class vidtk::detector_factory< T >;                          \
template class vidtk::detector_implementation_base< T >;                \
template void vidtk::detector_implementation_base< T >::                \
setup_base_pipeline< vidtk::async_pipeline >( vidtk::async_pipeline* ); \
template void vidtk::detector_implementation_base< T >::                \
setup_base_pipeline< vidtk::sync_pipeline >( vidtk::sync_pipeline* );   \
template class vidtk::detector_super_process< T >;

// ================================================================
// 4) Combine lists
#define INSTANTIATE_DETECTORS(T)                                         \
SUPPORT_CLASSES(T)                                                       \
DETECTOR_LIST(T)

// ================================================================
// 4) Instantiate for different pixel types
// Could use define from build to enable 16 bit pixels because
// applications using them are rare
INSTANTIATE_DETECTORS(vxl_byte)
INSTANTIATE_DETECTORS(vxl_uint_16)

#undef DETECTOR_LIST
#undef INSTANTIATE_DETECTORS
