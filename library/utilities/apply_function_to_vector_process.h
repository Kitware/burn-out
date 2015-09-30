/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_apply_function_to_vector_process_h_
#define vidtk_apply_function_to_vector_process_h_

#include <vcl_string.h>
#include <vcl_functional.h>
#include <vcl_vector.h>

#include <boost/function.hpp>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Apply a function to a vector of data
template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
class apply_function_to_vector_process : public process
{
public:
  typedef apply_function_to_vector_process self_type;

  apply_function_to_vector_process( const vcl_string &name);

  ~apply_function_to_vector_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_input( const vcl_vector<IN_T> &in );
  VIDTK_INPUT_PORT( set_input, const vcl_vector<IN_T>& );

  vcl_vector<OUT_T>& get_output(void);
  VIDTK_OUTPUT_PORT( vcl_vector<OUT_T>&, get_output );

private:
  config_block config_;

  const vcl_vector<IN_T> *in_;
  vcl_vector<OUT_T> out_;
};

}

#endif
