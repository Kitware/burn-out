# Set up warning flags
set(vidtk_warnings "")

include(CheckCXXCompilerFlag)
function (check_warning_flag variable flag)
  string(REPLACE "+" "plus" __safeflag "${flag}")

  if (MSVC)
    set(full_flag "/w${flag}")
    set(full_error_flag "/w${flag}")
  else ()
    set(full_flag "-W${flag}")
    set(full_error_flag "-Werror=${flag}")
  endif ()

  check_cxx_compiler_flag("${full_flag}" "have_warning_flag-${__safeflag}")

  if (have_targeted_error_flags AND ARGN)
    set(__realflag "${full_error_flag}")
  else ()
    set(__realflag "${full_flag}")
  endif ()

  if (have_warning_flag-${__safeflag})
    if (${variable})
      set(${variable}
        "${${variable}} ${__realflag}"
        PARENT_SCOPE)
    else ()
      set(${variable}
        "${__realflag}"
        PARENT_SCOPE)
    endif ()
  endif ()
endfunction ()

if (MSVC)
  option(VIDTK_ENABLE_DLLHELL_WARNINGS "Enables warnings about DLL visibility" OFF)
  if (NOT VIDTK_ENABLE_DLLHELL_WARNINGS)
    # C4251: STL interface warnings
    check_warning_flag(vidtk_warnings d4251)
    # C4275: Inheritance warnings
    check_warning_flag(vidtk_warnings d4275)
  endif ()

  # -----------------------------------------------------------------------------
  # Disable deprecation warnings for standard C and STL functions in VS2005 and
  # later
  # -----------------------------------------------------------------------------
  if (MSVC_VERSION GREATER 1400 OR
      MSVC_VERSION EQUAL 1400)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_DEPRECATE)
  endif ()
# Assume GCC-compatible
else ()
  set(abi_warnings_default ON)

  # Check for clang
  if (CMAKE_CXX_COMPILER MATCHES "clang\\+\\+")
    set(vidtk_using_clang TRUE)
  else ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.5")
      set(abi_warnings_default OFF)
    endif ()
  endif ()

  option( SHOW_ABI_WARNINGS "Show -Wabi warnings which are numerous on gcc < 4.5" "${abi_warnings_default}" )
  mark_as_advanced( SHOW_ABI_WARNINGS )

  check_cxx_compiler_flag("-Werror=all" have_targeted_error_flags)

  # General warnings
  check_warning_flag(vidtk_warnings all)
  check_warning_flag(vidtk_warnings extra)
  # Class warnings
  if ( SHOW_ABI_WARNINGS )
    check_warning_flag(vidtk_warnings abi)
  endif()
  check_warning_flag(vidtk_warnings ctor-dtor-privacy)
  check_warning_flag(vidtk_warnings init-self ON)
  check_warning_flag(vidtk_warnings overloaded-virtual)
  # Pointer warnings
  check_warning_flag(vidtk_warnings pointer-arith)
  check_warning_flag(vidtk_warnings strict-null-sentinel)
  # Formatting warnings
  check_warning_flag(vidtk_warnings format-security)
  check_warning_flag(vidtk_warnings format=2)
  # Casting warnings
  check_warning_flag(vidtk_warnings cast-align)
  check_warning_flag(vidtk_warnings cast-qual)
  #check_warning_flag(vidtk_warnings double-promotion)
  #check_warning_flag(vidtk_warnings float-equal ON)
  #check_warning_flag(vidtk_warnings strict-overflow=5)
  check_warning_flag(vidtk_warnings old-style-cast)
  # Variable naming warnings
  check_warning_flag(vidtk_warnings shadow)
  # C++ 11 compatibility warnings
  check_warning_flag(vidtk_warnings narrowing)
  # Exception warnings
  check_warning_flag(vidtk_warnings noexcept)
  # Miscellaneous warnings
  check_warning_flag(vidtk_warnings logical-op)
  check_warning_flag(vidtk_warnings missing-braces ON)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${vidtk_warnings}")
