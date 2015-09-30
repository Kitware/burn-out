/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_GEOGRAPHIC_COMMON_H_
#define VIDTK_GEOGRAPHIC_COMMON_H_

/*
 * \file common.h
 * \brief This file contains the preprocessor definitions necessary to support
 *        creating a shared library on windows
 */

#ifdef _WIN32
  #ifdef vidtk_geographic_EXPORTS
    #define VIDTK_GEOGRAPHIC_DLL __declspec(dllexport)
  #else
    #define VIDTK_GEOGRAPHIC_DLL __declspec(dllimport)
  #endif
#else
  #define VIDTK_GEOGRAPHIC_DLL
#endif

#endif

