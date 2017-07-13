set( vidtk_learning_sources
  adaboost.h                         adaboost.cxx
  histogram_weak_learners.h          histogram_weak_learners.cxx
  learner_data.h                     learner_data.cxx
  linear_weak_learners.h             linear_weak_learners.cxx
  stump_weak_learners.h              stump_weak_learners.cxx
  tree_weak_learners.h               tree_weak_learners.cxx
  weak_learner.h                     weak_learner.cxx
  learner_base.h
  learner_data_class_single_vector.h
  learner_data_class_simple_wrapper.h
  training_feature_set.h             training_feature_set.cxx
  learner_data_class_vector.h
  )

if( VIDTK_ENABLE_OPENCV )
  set( vidtk_learning_sources
    ${vidtk_learning_sources}
    kwsvm.h
    kwsvm.cxx
    kwsvm_ocv.cxx
    )
endif()

if( VIDTK_ENABLE_LIBSVM )
  set( vidtk_learning_sources
    ${vidtk_learning_sources}
    svm_learner.h                      svm_learner.cxx
    )
  add_definitions(-DUSE_LIBSVM)
endif()

add_library( vidtk_learning ${vidtk_learning_sources} )
set_target_properties( vidtk_learning PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  )

if( VIDTK_ENABLE_OPENCV )
  target_link_libraries( vidtk_learning ${OpenCV_LIBS} vidtk_utilities ${vidtk_utilites_plugin_lib} )
endif()

if( VIDTK_ENABLE_LIBSVM )
  target_link_libraries( vidtk_learning ${LIBSVM_LIBRARY} )
endif()

target_link_libraries( vidtk_learning vidtk_logger vbl vnl vnl_algo )

install( TARGETS vidtk_learning EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
  )
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
  )
export( TARGETS vidtk_learning APPEND FILE "${VIDTK_EXPORT_FILE}")
