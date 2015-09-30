project( vidtk_all )

cmake_minimum_required( VERSION 2.8 )
# Configure the warning and code coverage suppression file
configure_file(
  "${vidtk_all_SOURCE_DIR}/CTestCustom.cmake.in"
  "${vidtk_all_BINARY_DIR}/CTestCustom.cmake"
  COPYONLY
)

set(vidtk_VERSION_MAJOR 0)
set(vidtk_VERSION_MINOR 9)
set(vidtk_VERSION_PATCH 2)
set(vidtk_VERSION "${vidtk_VERSION_MAJOR}.${vidtk_VERSION_MINOR}.${vidtk_VERSION_PATCH}")

# -----------------------------------------------------------------------------
# Disable deprecation warnings for standard C and STL functions in VS2005 and
# later
# -----------------------------------------------------------------------------
if( MSVC_VERSION EQUAL 1400 OR MSVC_VERSION GREATER 1400)
  add_definitions( -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS )
  add_definitions( -D_SCL_SECURE_NO_DEPRECATE )
endif( MSVC_VERSION EQUAL 1400 OR MSVC_VERSION GREATER 1400)

# The following are required to uses Dart and the Cdash dashboard
enable_testing()
include(CTest)

# add the CMake directory for CMake modules
set(CMAKE_MODULE_PATH "${vidtk_all_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

# we can't build shared libraries on Windows so we leave it off by default;
# we haven't set up DLL exports or anything like that
option(BUILD_SHARED_LIBS "Build with shared libraries." OFF)

if( NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY )
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${vidtk_all_BINARY_DIR}/lib )
endif()
if( NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${vidtk_all_BINARY_DIR}/lib )
endif()
if( NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${vidtk_all_BINARY_DIR}/bin )
endif()

# Set up the build export configuration
set( VIDTK_EXPORT_FILE "${vidtk_all_BINARY_DIR}/vidtkConfig.cmake" )
configure_file(
  "${vidtk_all_SOURCE_DIR}/vidtkConfig.cmake.in"
  "${VIDTK_EXPORT_FILE}"
)

# Set up the install export
set( VIDTK_EXPORT_INSTALL_FILE "${vidtk_all_BINARY_DIR}/CMakeFiles/vidtkConfig.cmake" )
configure_file(
  "${vidtk_all_SOURCE_DIR}/vidtkConfig-install.cmake.in"
  "${VIDTK_EXPORT_INSTALL_FILE}"
  @ONLY
)

if( EXISTS ${CMAKE_SOURCE_DIR}/ec )
  option( ENABLE_EXPORT_CONTROLLED_TESTS
          "Enable tests that use export-controlled data" off )
  # CDash settings
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #
  #   These settings MUST NOT BE CHANGED!
  #
  # Submitting the results of these tests
  # to a non-EC dashboard would be a very
  # bad thing; let's hardcode where the
  # results get sent...

  if( ENABLE_EXPORT_CONTROLLED_TESTS )
    set( DROP_METHOD
         "https"
         CACHE INTERNAL "Protocol for submitting to dashboards"
         FORCE )
    set( DROP_SITE
         "ecdash.kitwarein.com"
         CACHE INTERNAL "Site for EC dashboards"
         FORCE )
    set( DROP_LOCATION
         "/CDash/submit.php?project=vidtk-ec"
         CACHE INTERNAL "Location on the dashboard to submit to"
         FORCE )
  else()
    # Don't override the default CTestConfig.cmake file
    unset( DROP_METHOD CACHE )
    unset( DROP_SITE CACHE )
    unset( DROP_LOCATION CACHE )
  endif()

  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
else()
  set( ENABLE_EXPORT_CONTROLLED_TESTS OFF FORCE )
endif()

# Force disabling of sdk installs until they are removed
set( VIDTK_ACTIVITY_SDK_INSTALL OFF CACHE BOOL "Install the activity SDK" FORCE )
set( VIDTK_EVENT_SDK_INSTALL OFF CACHE BOOL "Install the event SDK" FORCE )
set( VIDTK_COMMON_SDK_INSTALL OFF CACHE BOOL "Install the common SDK" FORCE )
set( VIDTK_HTML_INSTALL OFF CACHE BOOL "Install the html doc" FORCE )

option( USE_CPPDB_BACKEND "Build database backend support with CPPDB" "OFF")
option( VIDTK_ENABLE_OPENCV "Enable OpenCV-dependent code" "OFF" )

add_subdirectory( vidtk3p )
add_subdirectory( wrappers )
include_directories( wrappers )
add_subdirectory( library )

option( VIDTK_BUILD_TOOLS "Build tools. (Supercedes any sub-tool options)" "ON" )
if( VIDTK_BUILD_TOOLS )
  add_subdirectory( tools )
endif( VIDTK_BUILD_TOOLS )

option( VIDTK_BUILD_TESTS "Build tests" "ON" )
if( VIDTK_BUILD_TESTS )
  enable_testing()
  add_subdirectory( tests )
endif( VIDTK_BUILD_TESTS )

configure_file(
  ${vidtk_all_SOURCE_DIR}/CMake/event-sdk-config-install.cmake.in
  ${vidtk_all_BINARY_DIR}/CMakeFiles/event-sdk-config.cmake
  @ONLY
)
configure_file(
  ${vidtk_all_SOURCE_DIR}/CMake/activity-sdk-config-install.cmake.in
  ${vidtk_all_BINARY_DIR}/CMakeFiles/activity-sdk-config.cmake
  @ONLY
)
configure_file(
  ${vidtk_all_SOURCE_DIR}/CMake/common-sdk-config-install.cmake.in
  ${vidtk_all_BINARY_DIR}/CMakeFiles/common-sdk-config.cmake
  @ONLY
)

# Configure the CMake EXPORT file during installation
install( FILES ${vidtk_all_BINARY_DIR}/CMakeFiles/vidtkConfig.cmake
  DESTINATION lib/cmake
)
install( EXPORT vidtk DESTINATION lib/cmake FILE vidtkConfigTargets.cmake )

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install(FILES ${vidtk_all_BINARY_DIR}/CMakeFiles/activity-sdk-config.cmake
    DESTINATION lib/cmake
  )
  install(EXPORT activity-sdk-targets DESTINATION lib/cmake)
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
  install(FILES ${vidtk_all_BINARY_DIR}/CMakeFiles/event-sdk-config.cmake
    DESTINATION lib/cmake
  )
  install(EXPORT event-sdk-targets DESTINATION lib/cmake)
endif()

if( VIDTK_COMMON_SDK_INSTALL )
  install(FILES ${vidtk_all_BINARY_DIR}/CMakeFiles/common-sdk-config.cmake
    DESTINATION lib/cmake
  )
  install(EXPORT common-sdk-targets DESTINATION lib/cmake)
endif()

# build a CPack driven installer package
set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_NAME          "vidtk")
set(CPACK_PACKAGE_VENDOR        "Kitware, Inc.")
set(CPACK_PACKAGE_CONTACT       "kitware@kitware.com")
set(CPACK_MONOLITHIC_INSTALL    true)
set(CPACK_PACKAGE_VERSION_MAJOR "${vidtk_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${vidtk_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${vidtk_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION       "${vidtk_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
include (CPack)
