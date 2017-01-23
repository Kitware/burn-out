/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ring_buffer_process_h_
#define vidtk_ring_buffer_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/ring_buffer.h>
#include <utilities/timestamp.h>


namespace vidtk
{


/**
   \todo: 2 Options should be considered with regard to this class.
          1) Arslan's desire is to depricate the ring_buffer
          class and processes. This option is probably best.
          Barring that, 2) We should remove the inheritance
          of the ring_buffer class in the ring_buffer_process.

 **/

/// @todo: This warning is being suppressed because, at the moment, there's nothing that can be done.
/// See the suggestions above. Fixing the code that produces the warning is complicated. Deciding on the above
/// will help figure out the correct approach.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

template <class Data>
class ring_buffer_process
  : public process,
    public ring_buffer<Data>
{
public:
  typedef ring_buffer_process self_type;

  ring_buffer_process( std::string const& name );

  virtual ~ring_buffer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  virtual std::vector< Data > const& data() const { return this->buffer_; }

  /// Set the next item to be inserted into the buffer.
  void set_next_datum( Data const& item );
  VIDTK_INPUT_PORT( set_next_datum, Data const& );

  /// \brief Return the item as close as possible to \a offset away from the last item.
  ///
  /// An \a offset of 0 refers to the last item.
  ///
  /// If there is no datum at the requested offset, then the last item
  /// is returned.  E.g., if the buffer has 5 items (offsets 0..4),
  /// and you request an item at offset 8, you will obtain the item at
  /// offset 4.
  ///
  /// It is an error to ask for an item when the buffer is empty.  Use
  /// size() to verify.
  Data datum_nearest_to( unsigned offset ) const;
  VIDTK_OUTPUT_PORT_1_ARG( Data, datum_nearest_to, unsigned );

  VIDTK_OUTPUT_PORT_1_ARG( bool, has_datum_at, unsigned );

  /// \brief The whole buffer currently stored in the process.
  vidtk::buffer<Data> const& buffer() const;
  VIDTK_OUTPUT_PORT( vidtk::buffer<Data> const&, buffer );

  unsigned offset_of( Data const& ) const { return unsigned(-1); }

protected:
  config_block config_;

  bool disable_capacity_error_;

  Data const* next_datum_;

  bool disabled_;
};

// Declare specializations of offset_of for types we can handle.

template <>
unsigned
vidtk::ring_buffer_process< timestamp >
::offset_of( timestamp const& ) const;


} // end namespace vidtk


#endif // vidtk_ring_buffer_process_h_
