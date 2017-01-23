/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_raw_descriptor_writer_process_h_
#define vidtk_raw_descriptor_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/raw_descriptor.h>

#include <string>
#include <iostream>
#include <fstream>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{


/// Write raw descriptors to a file, either to aid with generating classifiers
/// or for debugging purposes.
class raw_descriptor_writer_process
  : public process
{

public:

  typedef raw_descriptor_writer_process self_type;

  raw_descriptor_writer_process( std::string const& name );
  virtual ~raw_descriptor_writer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_descriptors( raw_descriptor::vector_t const& desc_list );
  VIDTK_INPUT_PORT( set_descriptors, raw_descriptor::vector_t const& );

private:


  // Input(s)
  raw_descriptor::vector_t inputs_;

  // Config block
  config_block config_;

  class impl;
  boost::scoped_ptr< impl > impl_;
};


} // end namespace vidtk


#endif // vidtk_raw_descriptor_writer_process_h_
