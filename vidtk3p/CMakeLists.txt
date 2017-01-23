
# KWSys borrowed from CMake
#---------------------------------------------------------------------
# Create the kwsys library for vidtk.
set(KWSYS_NAMESPACE vidtksys)
set(KWSYS_USE_DynamicLoader 1)
set(KWSYS_USE_SystemTools 1)
set(KWSYS_USE_RegularExpression 1)
set(KWSYS_USE_MD5 1)
set(KWSYS_INSTALL_EXPORT_NAME vidtk)
set(KWSYS_INSTALL_LIB_DIR lib)
set(KWSYS_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/KWSys CACHE INTERNAL "")
add_subdirectory(KWSys)
export( TARGETS vidtksys APPEND FILE "${VIDTK_EXPORT_FILE}" )
