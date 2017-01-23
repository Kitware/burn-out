/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_queue_process_h_
#define vidtk_queue_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>


namespace vidtk
{


template <class TData>
class queue_process
  : public process
{
public:
  typedef queue_process self_type;

  queue_process( std::string const& name );

  ~queue_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  virtual process::step_status step2();

  /// Returns true if the queue is on away from the max size
  bool is_close_to_full() const;

  unsigned length();

  void clear();

  /// \brief Set the flag to start/stop popping items out of the queue at each step.
  void set_disable_read( bool disable_pop );

  void set_input_datum( TData const& item );

  VIDTK_INPUT_PORT( set_input_datum, TData const& );

  TData get_output_datum();

  VIDTK_OUTPUT_PORT( TData, get_output_datum );

  unsigned index_nearest_to( TData const & /*D*/ ) const { return unsigned(-1); }

  TData const & datum_at( unsigned ) const;

  std::vector< TData > & get_data_store() { return queue_; }
  unsigned int get_max_length() const { return max_length_; }

protected:
  config_block config_;

  bool disable_read_;

  unsigned max_length_;

  TData const* input_datum_;
  TData output_datum_;

  std::vector< TData > queue_;
};

// Declare specializations of offset_of for types we can handle.

template <>
unsigned
queue_process< vidtk::timestamp >
::index_nearest_to( timestamp const& ) const;

} // end namespace vidtk


#endif // vidtk_queue_process_h_
