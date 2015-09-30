/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_buffer_h_
#define vidtk_buffer_h_

namespace vidtk
{


template<class Data>
class buffer
{
public:
  virtual ~buffer() { }

  /// The maximum number of elements that can be stored in the buffer.
  virtual unsigned capacity() const = 0;

  /// \brief The number of elements currently in the buffer.
  virtual unsigned size() const = 0;

  /// \brief Return the item \a offset away from the last item.
  ///
  /// An \offset of 0 refers to the last item.
  ///
  /// It is an error to ask for an offset beyond the number of items
  /// currently in the buffer.  Use has_datum_at() or length() to
  /// verify.
  virtual Data const& datum_at( unsigned offset ) const = 0;

  /// \brief Check if there is an \a offset away from the last item.
  ///
  /// An \offset of 0 refers to the last item.
  ///
  /// If <tt>has_datum_at(x)</tt> returns \c true, then
  /// <tt>has_datum_at(y)</tt> will also return \c true for all 0 \<=
  /// y \<= x.
  virtual bool has_datum_at( unsigned offset ) const = 0;

  virtual unsigned offset_of( Data const& ) const = 0;
};


} // end namespace vidtk


#endif // vidtk_buffer_h_
