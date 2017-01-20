/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef fuzzy_compare_h_
#define fuzzy_compare_h_

/**\file
   \brief
   Methods for comparing floating point numbers
*/

#include <cmath>
#include <limits>

namespace vidtk
{

template<typename T> class fuzzy_compare
{
public:

  static T epsilon()
  {
    return epsilon_;
  };
  static bool compare(T a, T b, T tolerance)
  {
    // must be <= in the case of 0 tolerance.
    return fabs(a - b) <= tolerance;
  };
  static bool compare(T a, T b)
  {
    return fuzzy_compare<T>::compare(a, b, fuzzy_compare<T>::epsilon());
  };

private:
  static T epsilon_;
};

template<typename T> T fuzzy_compare<T>::epsilon_ =
  static_cast<T>(1.0 / std::pow(10.0, std::numeric_limits<T>::digits10));

} // end namespace vidtk

#endif // fuzzy_compare_h_
