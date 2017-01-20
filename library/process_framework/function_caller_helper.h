/*ckwg +5
 * Copyright 2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef function_caller_helper_h_
#define function_caller_helper_h_

namespace vidtk
{

namespace helper
{

template<class T>
struct holder
{
  holder() { }
  holder( T p ) : val_( p ) { }
  T value() const { return val_; }
  T val_;
};

template<class T>
struct holder<T&>
{
  holder() : ptr_( NULL ) { }
  holder( T& p ) : ptr_( &p ) { }
  T& value() const { return *ptr_; }
  T* ptr_;
};

}

}

#endif // function_caller_helper_h_
