#
# Convenience macro to take advantage of VisionTPL find_package wrapper if
# VisionTPL was found. Else, fall back on base CMake find_package.
#
macro(vidtk_find_package)
  if( fletch_FOUND )
    find_package(${ARGN})
  else()
    find_package(${ARGN})
  endif()
endmacro()
