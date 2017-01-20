cmake_minimum_required( VERSION 2.8.12 )

project( vidtk_all )

# fletch can provide many of the external dependencies
find_package(fletch)
find_package(VXL REQUIRED)
include_directories(SYSTEM ${VXL_VCL_INCLUDE_DIR})
include_directories(SYSTEM ${VXL_CORE_INCLUDE_DIR})
link_directories(${VXL_LIBRARY_DIR})

find_package(Qt4 4.7.0 REQUIRED)
include(${QT_USE_FILE})

#Set CMP0022 to NEW so we error if someone writes older style cmake code
#See http://www.cmake.org/cmake/help/v2.8.12/cmake.html#policy:CMP0022
#for details on CMP0022
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()

#Set CMP0048 to OLD until we can move min cmake to 3.0
#See http://www.cmake.org/cmake/help/v3.0/policy/CMP0048.html for details
#We need OLD to maintain the old approach to versioning ( if we even use it )
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 OLD)
endif()

# Configure the warning and code coverage suppression file
configure_file(
  "${vidtk_all_SOURCE_DIR}/CTestCustom.cmake.in"
  "${vidtk_all_BINARY_DIR}/CTestCustom.cmake"
  COPYONLY
)

set(vidtk_source_dir
  "${CMAKE_CURRENT_SOURCE_DIR}")
set(vidtk_binary_dir
  "${CMAKE_CURRENT_BINARY_DIR}")

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
  # Enable /MP flag for Visual Studio 2008 and greater
  if(MSVC_VERSION GREATER 1400)
    # Allow the MP flag to get set externally. If not set, default to OFF.
    if(NOT vidtk_ENABLE_MULTI_PROCESS_BUILDS)
      set(vidtk_ENABLE_MULTI_PROCESS_BUILDS OFF)
    endif()
    set(ENABLE_MULTI_PROCESS_BUILDS ${vidtk_ENABLE_MULTI_PROCESS_BUILDS} CACHE BOOL "Enable multi-process builds")
    set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
    set(CMAKE_CXX_MP_NUM_PROCESSORS ${PROCESSOR_COUNT} CACHE STRING "The maximum number of processes for the /MP flag")
    if (ENABLE_MULTI_PROCESS_BUILDS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
    endif ()
  endif()
endif()

# add the CMake directory for CMake modules
set(CMAKE_MODULE_PATH "${vidtk_all_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
include(vidtk_find_package)

set(EXECUTABLE_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/bin)
if (WIN32)
  set(LIBRARY_OUTPUT_PATH       ${CMAKE_BINARY_DIR}/bin)
else (WIN32)
  set(LIBRARY_OUTPUT_PATH       ${CMAKE_BINARY_DIR}/lib)
endif (WIN32)
set(PYTHON_OUTPUT_PATH        ${LIBRARY_OUTPUT_PATH}/python)

if (WIN32)
  set(EXECUTABLE_INSTALL_PATH    ${CMAKE_INSTALL_PREFIX}/${PREFIX}/bin)
  set(LIBRARY_INSTALL_PATH       ${CMAKE_INSTALL_PREFIX}/${PREFIX}/bin)
else (WIN32)
  set(EXECUTABLE_INSTALL_PATH    ${CMAKE_INSTALL_PREFIX}/${PREFIX}/bin)
  set(LIBRARY_INSTALL_PATH       ${CMAKE_INSTALL_PREFIX}/${PREFIX}/lib${LIB_SUFFIX})
endif (WIN32)
set(INCLUDE_INSTALL_PATH       ${CMAKE_INSTALL_PREFIX}/${PREFIX}/include)

# check for hardware support for sse2 with the current compiler flags
TRY_COMPILE(VIDTK_HAS_SSE2_HARDWARE_SUPPORT
  ${CMAKE_BINARY_DIR}
  ${vidtk_all_SOURCE_DIR}/CMake/test_sse.cxx
  OUTPUT_VARIABLE OUTPUT)
IF( VIDTK_HAS_SSE2_HARDWARE_SUPPORT )
  OPTION(VIDTK_CONFIG_ENABLE_SSE2 "Enable Streaming SIMD Extensions 2 optimisations (hardware dependant)." ON)
ELSE ( VIDTK_HAS_SSE2_HARDWARE_SUPPORT )
  message("NO SSE SUPPORT")
  message(${OUTPUT})
endif()

# we can't build shared libraries on Windows so we leave it off by default;
# we haven't set up DLL exports or anything like that
if (WIN32)
  option(BUILD_SHARED_LIBS "Build with shared libraries." OFF)
else()
  option(BUILD_SHARED_LIBS "Build with shared libraries." ON)
endif()

if (UNIX AND NOT APPLE AND NOT BUILD_SHARED_LIBS)
  set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

# Protobuf support. Controls protobuf library and support processes.
option(VIDTK_ENABLE_PROTOBUF "Build protobuf serialization support" OFF)

option(USE_ANGEL_FIRE "Build with Angel Fire Support" OFF)
if(USE_ANGEL_FIRE)
  find_package(AFReader REQUIRED)
  include_directories(SYSTEM ${AFREADER_INCLUDE_DIRS})
endif()

option (USE_CONDOR "Build with CONDOR support" OFF)
if(USE_CONDOR)
  find_package(Condor REQUIRED)
  include_directories(SYSTEM ${CONDOR_INCLUDE_DIR})
  option (USE_APIX_INTERLINK "Build with apix interlink support" OFF)
  if(USE_APIX_INTERLINK)
    find_package(ApixInterlink REQUIRED)
    include_directories(SYSTEM ${INTERLINK_INCLUDE_DIR})
  endif(USE_APIX_INTERLINK)
endif(USE_CONDOR)

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
  message(WARNING "Export control directory has been FOUND")
  option( ENABLE_EXPORT_CONTROLLED_TESTS
          "Enable tests that use export-controlled data" off )
  message(WARNING "The export controlled dashboard is ENABLED")
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
    message(WARNING "Export controlled tests are ENABLED")
    set( DROP_METHOD
         "https"
         CACHE INTERNAL "Protocol for submitting to dashboards"
         FORCE )
    set( DROP_SITE
         "ecdash.kitware.com"
         CACHE INTERNAL "Site for EC dashboards"
         FORCE )
    set( DROP_LOCATION
         "/CDash/submit.php?project=vidtk-ec"
         CACHE INTERNAL "Location on the dashboard to submit to"
         FORCE )
    set( CTEST_CURL_OPTIONS CURLOPT_SSL_VERIFYHOST_OFF;CURLOPT_SSL_VERIFYPEER_OFF )
  else()
    message(WARNING "Export controlled tests are DISABLED")
    # Don't override the default CTestConfig.cmake file
    unset( DROP_METHOD CACHE )
    unset( DROP_SITE CACHE )
    unset( DROP_LOCATION CACHE )
  endif()

  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
  #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
else()
  set( ENABLE_EXPORT_CONTROLLED_TESTS OFF CACHE BOOL "Enable tests that use export-controlled data" FORCE )
endif()

# Force disabling of sdk installs until they are removed
set( VIDTK_ACTIVITY_SDK_INSTALL OFF CACHE BOOL "Install the activity SDK" FORCE )
set( VIDTK_EVENT_SDK_INSTALL OFF CACHE BOOL "Install the event SDK" FORCE )
set( VIDTK_COMMON_SDK_INSTALL OFF CACHE BOOL "Install the common SDK" FORCE )
set( VIDTK_HTML_INSTALL OFF CACHE BOOL "Install the html doc" FORCE )

option( USE_CPPDB_BACKEND "Build database backend support with CPPDB" "OFF")

option( VIDTK_ENABLE_OPENCV "Enable OpenCV-dependent code" "OFF" )
option( VIDTK_ENABLE_VLFEAT "Enable VLFeat-dependent code" "OFF" )
option( VIDTK_ENABLE_CAFFE "Enable Caffe-dependent code" "OFF" )
option( VIDTK_ENABLE_VISCL "Enable VisCL dependent code (for OpenCL on GPU)" "OFF" )
option( VIDTK_ENABLE_SHAPELIB "Enable Shapelib dependent code" OFF)
option( VIDTK_ENABLE_LIBSVM "Enable LibSVM-dependent code" "OFF" )
option( VIDTK_ENABLE_LIBJSON "Enable LibJSON dependent code" ON)
option( VIDTK_ENABLE_TRACK_ORACLE "Enable track_oracle dependent code" ON)
option( VIDTK_ENABLE_TinyXML "Enable TinXML dependent code" ON)

if (USE_CPPDB_BACKEND)
  find_package(CppDB REQUIRED)
  add_definitions(-DMODULE_PATH="${CppDB_LIB_DIR}")
  add_definitions(-DHAS_CPPDB)
  include_directories( SYSTEM ${CppDB_INCLUDE_DIR} )
endif()

if (VIDTK_ENABLE_TRACK_ORACLE)
  add_definitions( -DUSE_TRACK_ORACLE )
  if (NOT VIDTK_ENABLE_TinyXML)
    message("Turning on TinyXML since it's required for Track Oracle. If you want to remove TinyXML, please turn off Track Oracle too.")
    set(VIDTK_ENABLE_TinyXML ON CACHE BOOL "" FORCE)
  endif()
endif()

if (VIDTK_ENABLE_TinyXML)
  find_package( TinyXML REQUIRED )
  include_directories( SYSTEM ${TinyXML_INCLUDE_DIR} )
  add_definitions( -DUSE_TinyXML )
else()
  set(VIDTK_ENABLE_TinyXML OFF CACHE BOOL "" FORCE)
endif()

if( VIDTK_ENABLE_OPENCV )
  find_package( OpenCV REQUIRED )
  if( OpenCV_VERSION VERSION_LESS "2.4" )
    message( FATAL_ERROR "OpenCV version must be at least 2.4" )
  endif()

  file( APPEND ${VIDTK_EXPORT_FILE} "find_package( OpenCV REQUIRED PATHS ${OpenCV_ROOT} ${OpenCV_DIR} NO_DEFAULT_PATH)" )
  add_definitions(-DUSE_OPENCV)
  include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS})
endif()

if( VIDTK_ENABLE_VLFEAT )
  find_package( VLFeat REQUIRED )
  add_definitions(-DUSE_VLFEAT)
  include_directories(SYSTEM ${VLFeat_INCLUDE_DIRS})
endif()

if( VIDTK_ENABLE_CAFFE )
  find_package( Caffe REQUIRED )
  add_definitions(-DUSE_CAFFE)
  include_directories(SYSTEM ${Caffe_INCLUDE_DIRS})
endif()

if (VIDTK_ENABLE_LIBJSON)
  find_package(LIBJSON REQUIRED)
  include_directories ( SYSTEM ${LIBJSON_INCLUDE_DIR} )
endif()

option( VIDTK_ENABLE_TESSERACT "Enable Tesseract-dependent code" "OFF" )

if( VIDTK_ENABLE_VISCL )
  find_package(viscl REQUIRED)
  include_directories(SYSTEM ${viscl_INCLUDE_DIR})
endif()

if( VIDTK_ENABLE_SHAPELIB )
  find_package(SHAPELIB REQUIRED)
  include_directories(SYSTEM ${SHAPELIB_INCLUDE_DIR})
  add_definitions("-DSHAPELIB_ENABLED")
endif()

set(VIDTK_HAS_GDAL FALSE)
find_package(GDAL)
if( GDAL_FOUND )
  # We need to build the file in a line-by-line fashon because of
  # portability with end of line markers.
  file( WRITE  ${CMAKE_BINARY_DIR}/test_gdal_version.cxx "#include <gdal_version.h>\n" )
  file( APPEND ${CMAKE_BINARY_DIR}/test_gdal_version.cxx "#if ( GDAL_COMPUTE_VERSION( 1, 11, 0  ) != GDAL_VERSION_NUM )\n")
  file( APPEND ${CMAKE_BINARY_DIR}/test_gdal_version.cxx "#error \"GDAL Not required version: $GDAL_VERSION_NUM\"\n" )
  file( APPEND ${CMAKE_BINARY_DIR}/test_gdal_version.cxx "#endif\n")
  file( APPEND ${CMAKE_BINARY_DIR}/test_gdal_version.cxx "int main() { } // just need some code\n")

  TRY_COMPILE( GDAL_VERSION_MATCH
             ${CMAKE_BINARY_DIR}
             ${CMAKE_BINARY_DIR}/test_gdal_version.cxx
             COMPILE_DEFINITIONS "-I${GDAL_INCLUDE_DIR}"
             OUTPUT_VARIABLE OUTPUT)

  if( GDAL_VERSION_MATCH )
    set(VIDTK_HAS_GDAL TRUE)
    include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
  else()
    message( WARNING "GDAL found, but not needed version. Need version: 1.11" )
    message(${OUTPUT})
  endif()
endif()

find_package( Boost 1.42 REQUIRED COMPONENTS thread filesystem system date_time regex)
add_definitions( -DBOOST_ALL_NO_LIB )
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

add_subdirectory( vidtk3p )

option( VIDTK_ENABLE_MAPTK "Enable MAP-Tk-dependent code" OFF )
if( VIDTK_ENABLE_MAPTK )
  vidtk_find_package(MAPTK REQUIRED)

  vidtk_find_package(VITAL REQUIRED)
  include_directories(SYSTEM ${MAPTK_INCLUDE_DIRS})
  include_directories(SYSTEM ${VITAL_INCLUDE_DIRS})
  add_definitions(-DWITH_MAPTK_ENABLED)

  # MAP-Tk requires cxx11
#  Note, when we can update the minimum CMake version to 3.1,
#  we can use the single line below rather than the large block that's enabled
#  set (CMAKE_CXX_STANDARD 11)
  include(CheckCXXCompilerFlag)
  #CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
  #CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
endif()

# Required VXL git version
include(GitVersionTest)
check_reachable_git_hash(VXL ${VXL_CMAKE_DIR} "e03c56a32e5f52c80d86b503a778210437bb861d")

add_subdirectory( wrappers )
include_directories( SYSTEM wrappers )
include_directories( SYSTEM ${CMAKE_CURRENT_BINARY_DIR}/wrappers )

include("${vidtk_all_SOURCE_DIR}/CMake/flags.cmake")

add_subdirectory( library )

# Prefix added to all tool binaries
set( VIDTK_BINARY_PREFIX "" CACHE STRING "Filename prefix for all compiled tools" )

option( VIDTK_BUILD_TOOLS "Build tools. (Supercedes any sub-tool options)" ON )
if( VIDTK_BUILD_TOOLS )
  add_subdirectory( tools )
endif( VIDTK_BUILD_TOOLS )

option( VIDTK_BUILD_TESTS "Build tests" "ON" )
if( VIDTK_BUILD_TESTS )
  # The following are required to uses Dart and the Cdash dashboard
  enable_testing()
  include(CTest)
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
