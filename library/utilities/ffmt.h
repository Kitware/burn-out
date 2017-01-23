/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ffmt_h_
#define vidtk_ffmt_h_

#include <iostream>

namespace vidtk
{

/// \brief Format floating point output in standard way on all platforms
///
/// ffmt() provides a way to format floating-point values in a portable and
/// standard way across platforms, guaranteeing that the text representations
/// are identical.  This allows direct compares of text files for tests, for
/// instance.  The problem with using the standard C++ routines is that the
/// implementations vary (exponents are 2 *or* 3 digits depending on the
/// system)
///
/// example:
///   LOG_INFO( ffmt(1234567.89, 3));
/// produces: 1.23e6
class ffmt
{
public:
  /// \brief construct from floating point value with optional precision
  ffmt(double value, int precision = 6)
    :value_(value), precision_(precision)
  {
  }
private:
  double value_;
  int precision_;
  friend std::ostream& operator<< (std::ostream& out, const ffmt &f);
};

/// \brief ostream insertion operator for standard formatted floating point
std::ostream& operator<< (std::ostream& out, const ffmt &f);

} // end namespace vidtk

#endif // #ifndef vidtk_ffmt_h_
