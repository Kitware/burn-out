/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_MERGE_NVECTORS_PROCESS_H_
#define _VIDTK_MERGE_NVECTORS_PROCESS_H_

#include <vcl_string.h>
#include <vcl_utility.h>
#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// combine two vectors into a single vector
template<typename T>
class merge_n_vectors_process : public process
{
public:
  typedef merge_n_vectors_process self_type;

  merge_n_vectors_process( const vcl_string &name );

  ~merge_n_vectors_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void add_vector( const vcl_vector<T> &v1 );
  VIDTK_INPUT_PORT( add_vector, const vcl_vector<T>& );

  vcl_vector<T>& v_all(void);
  VIDTK_OUTPUT_PORT( vcl_vector<T>&, v_all );

private:
  config_block config_;

  vcl_vector< vcl_vector<T> const * > v_in_;
  vcl_vector<T> v_all_;
  unsigned int size_;
};

}  // namespace vidtk

#endif

