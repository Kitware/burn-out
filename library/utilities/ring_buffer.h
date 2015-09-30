/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ring_buffer_h_
#define vidtk_ring_buffer_h_

#include <utilities/buffer.h>

#include <vcl_vector.h>

namespace vidtk
{

template<class Data>
class ring_buffer
    : public buffer< Data >
{
public:
  ring_buffer();
  virtual ~ring_buffer();

  virtual void set_capacity( unsigned size );

  virtual unsigned capacity() const;

  virtual unsigned size() const;

  /// \brief Inserts an item into the buffer.
  ///
  /// Note: This will overwrite the oldest item in the buffer if
  /// the buffer is at capacity.
  virtual void insert( Data const& item );

  /// The index of of the newest item in the buffer.
  virtual unsigned head() const;

  /// \brief Return the item \a offset away from the last item.
  ///
  /// An \offset of 0 refers to the last (oldest) item.
  ///
  /// It is an error to ask for an offset beyond the number of items
  /// currently in the buffer.  Use has_datum_at() or size() to
  /// verify.
  virtual Data const& datum_at( unsigned offset ) const;

  /// \brief Check if there is an \a offset away from the last item.
  ///
  /// An \offset of 0 refers to the last (oldest) item.
  virtual bool has_datum_at( unsigned offset ) const;

  /// \brief Not implemented
  virtual unsigned offset_of( Data const& ) const { return unsigned(-1); }

  /// \brief Empties the buffer. Returns the buffer to the initial state.
  virtual void clear();


protected:
  vcl_vector< Data > buffer_;
  unsigned head_;
  unsigned item_count_;
};


} // end namespace vidtk


#endif // vidtk_ring_buffer_h_
