set( vidtk_learning_sources
  adaboost.h                         adaboost.cxx
  gaussian_weak_learners.h           gaussian_weak_learners.cxx
  histogram_weak_learners.h          histogram_weak_learners.cxx
  learner_data.h                     learner_data.cxx
  linear_weak_learners.h             linear_weak_learners.cxx
  stump_weak_learners.h              stump_weak_learners.cxx
  weak_learner.h                     weak_learner.cxx
  learner_base.h
  learner_data_class_single_vector.h
  training_feature_set.h             training_feature_set.cxx
  learner_data_class_vector.h
)

add_library( vidtk_learning ${vidtk_learning_sources} )
set_target_properties( vidtk_learning PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_learning vcl vbl vnl vnl_algo )
install( TARGETS vidtk_learning EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_learning APPEND FILE "${VIDTK_EXPORT_FILE}")
