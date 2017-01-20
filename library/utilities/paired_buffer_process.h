/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_paired_buffer_process_h_
#define vidtk_paired_buffer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/paired_buffer.h>

namespace vidtk
{

template< class keyT, class datumT >
class paired_buffer_process
  : public process
{
public:
  typedef paired_buffer_process self_type;
  typedef typename paired_buffer< keyT, datumT>::key_datum_t pair_t;
  typedef paired_buffer< keyT, datumT> paired_buff_t;

  paired_buffer_process( std::string const& );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  // ---------------------------- I/O ports --------------------------------

  void set_next_key( keyT const& );
  VIDTK_INPUT_PORT( set_next_key, keyT const& );

  void set_next_datum( datumT const& );
  VIDTK_INPUT_PORT( set_next_datum, datumT const& );

  void set_updated_items( std::vector< pair_t > const&  );
  VIDTK_INPUT_PORT( set_updated_items, std::vector< pair_t > const& );

  paired_buff_t buffer() const;
  VIDTK_OUTPUT_PORT( paired_buff_t, buffer );

private:
  paired_buff_t paired_buff_;

  // I/O data
  keyT const * next_key_;
  datumT const * next_datum_;

  // Configuration parameters
  config_block config_;
};

} // namespace vidtk

#endif // vidtk_paired_buffer_process_h_
