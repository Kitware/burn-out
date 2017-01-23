/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ffmt.h"
#include <limits>
#include <string>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <cstdlib>

namespace vidtk
{

// hack the output of the standard library stream insertion operator for
// floating point values so the exponent always has three digits. Seems to
// be the only difference across platforms
std::ostream& operator<< (std::ostream& out, const ffmt &f)
{
  std::stringstream ss;
  ss.setf(std::ios::scientific, std::ios::floatfield);
  ss.precision(f.precision_);
  ss << f.value_;
  size_t e_pos = ss.str().rfind('e');
  if (std::string::npos == e_pos)
  {
    // no exponent found
    return out << ss.str();
  }
  std::string mant = ss.str().substr(0, e_pos);
  std::string ex = ss.str().substr(e_pos+1);
  int ex_num = std::atoi(ex.c_str());
  bool neg;
  if (ex_num >= 0)
  {
    neg = false;
  }
  else
  {
    neg = true;
    ex_num = - ex_num;
  }
  std::stringstream ex_fmt;
  ex_fmt << std::setfill('0');
  ex_fmt << std::setw(3);
  ex_fmt << ex_num;
  out << mant << "e";
  if (neg)
  {
    out << "-";
  }
  else
  {
    out << "+";
  }
  out << ex_fmt.str();
  return out;
}

} // end namespace vidtk
