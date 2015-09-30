/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ffmt.h"
#include <vcl_limits.h>
#include <vcl_string.h>
#include <vcl_sstream.h>
#include <vcl_cmath.h>
#include <vcl_iomanip.h>
#include <vcl_cstdlib.h>

namespace vidtk
{

// hack the output of the standard library stream insertion operator for
// floating point values so the exponent always has three digits. Seems to
// be the only difference across platforms
vcl_ostream& operator<< (vcl_ostream& out, const ffmt &f)
{
  vcl_stringstream ss;
  ss.setf(vcl_ios::scientific, vcl_ios::floatfield);
  ss.precision(f.precision_);
  ss << f.value_;
  size_t e_pos = ss.str().rfind('e');
  if (vcl_string::npos == e_pos)
  {
    // no exponent found
    return out << ss.str();
  }
  vcl_string mant = ss.str().substr(0, e_pos);
  vcl_string ex = ss.str().substr(e_pos+1);
  int ex_num = vcl_atoi(ex.c_str());
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
  vcl_stringstream ex_fmt;
  ex_fmt << vcl_setfill('0');
  ex_fmt << vcl_setw(3);
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
