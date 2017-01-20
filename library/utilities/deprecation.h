/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_UTILITIES_DEPRECATION_H
#define VIDTK_UTILITIES_DEPRECATION_H

#ifdef _WIN32
#define VIDTK_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__clang__)
#if __has_extension(attribute_deprecated_with_message)
#define VIDTK_DEPRECATED(msg) __attribute__((__deprecated__(msg)))
#else
#define VIDTK_DEPRECATED(msg) __attribute__((__deprecated__))
#endif
#elif defined(__GNUC__)
#if (__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))
#define VIDTK_DEPRECATED(msg) __attribute__((__deprecated__(msg)))
#else
#define VIDTK_DEPRECATED(msg) __attribute__((__deprecated__))
#endif
#else
#define VIDTK_DEPRECATED(msg)
#endif

#endif
