/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_checked_bool_h_
#define vidtk_checked_bool_h_

#include <string>
#include <sstream>

/** \file
    \brief
    Method and field definitions for a bool-like value.
*/

/**
 \namespace vidtk
 \brief Video Toolkit.
*/
namespace vidtk
{

/// \brief A bool-like value.
///
/// If this object is not "checked" before it destructs, and the value
/// is \c false, it will throw an unchecked_return_value exception.
/// The idea is that a given function's boolean return values can be
/// returned as checked_bool, and if the user doesn't actually check
/// them, an unchecked_return_value exception will be thrown when the
/// function fails.
///
/// \note Since the exception is thrown in the destructor of the
/// class, users should generally not create named objects of this
/// type unless they check it immediate afterwards. Example of BAD code:
/// \code
/// checked_bool f() {
///    checked_bool res = false;
///    g(); // if g throws, then when the destructor of res
///         // also throws, things go bad.
///    return res;
/// }
/// \endcode
/// A better version is
/// \code
/// checked_bool f() {
///    bool res = false;
///    g();
///    return res; // or return checked_bool( "f() failed" );
/// }
/// \endcode
///
/// \note
/// It is not good to use checked_bool in logical expressions such as
/// \code
///   checked_bool A = ...;
///   checked_bool B = ...;
///   checked_bool C = A && B;
/// \code
/// because if either A or B is false, the failure message will not
/// propagate to C.  If we do this, and either A or B is "false", when
/// the failure triggers an exception, the message is not helpful.  If
/// you really do want this behavior, explicitly cast to bool:
/// \code
///   checked_bool C = bool(A) && bool(B);
/// \endcode
/// (It is not possible to overload operator&& to maintain the message
/// because user-defined operator&& does not short-circuit, and does
/// not create a sequence point.)
///
/// A more verbose, but more readable, way is to explicitly encode the
/// message you want to return on failure:
/// \code
///   checked_bool C( true );
///   checked_bool A = ...;
///   checked_bool B = ...;
///   if( ! A )
///     C = A;
///   else
///     C = B;
/// \endcode
class checked_bool
{
public:
  ///Constructor: pass a bool value.
  checked_bool( bool v );
  ///Constructor: pass a failure message.
  checked_bool( std::string const& failure_msg );
  ///An overload to avoid char const* being converted to bool.
  checked_bool( char const* failure_msg );

  ///\brief Copy Constructor:
  ///
  /// When the object is copied, the source is marked as "checked" and
  /// the result (new copy) is marked as "unchecked", regardless of
  /// what the source used to be.  This is to allow the most common
  /// case of copying, which is to pass the "failure message" around.
  ///
  /// checked_bool f() {
  ///   checked_bool b = helper();
  ///   if( ! b ) {
  ///     return b; // b has already been checked!
  ///   }
  /// }
  checked_bool( checked_bool const& );

  /// \brief Overload of operator=
  ///
  /// See comments in copy constructor.
  checked_bool& operator=( checked_bool const& );

  ///Destructor
  ~checked_bool();

  ///Overload of the bool operator
  operator bool() const;

  ///Checked bool message.
  std::string const& message() const;

  bool to_bool();

private:
  bool value_;
  mutable bool checked_;
  std::string msg_;
};

/// \brief Create a \c false checked_bool object using stream operators.
///
/// Allows, for example
/// \code
///    return checked_bool_create( "failed because " << x << " != " << y );
/// \endcode
#define checked_bool_create( X ) vidtk::checked_bool( ( std::ostringstream() << X ).str() )

struct read_documentation_of_checked_bool_with_logical_operators;

/// \brief Overload to prevent users from calling it.
read_documentation_of_checked_bool_with_logical_operators& operator&&( checked_bool const&, checked_bool const& );

/// \brief Overloads to allow logical && with plain bool.
inline bool operator&&( bool const& a, checked_bool const& b )
{
  return a && bool(b);
}

/// \brief Overloads to allow logical && with plain bool.
inline bool operator&&( checked_bool const& a, bool const& b )
{
  return bool(a) && b;
}


} // end namespace vidtk

#endif // vidtk_checked_bool_h_
