/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_MERGE_2VECTORS_PROCESS_H_
#define _VIDTK_MERGE_2VECTORS_PROCESS_H_

#include <vcl_string.h>
#include <vcl_utility.h>
#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// combine two vectors into a single vector
template<typename T>
class merge_2_vectors_process : public process
{
public:
  typedef merge_2_vectors_process self_type;

  merge_2_vectors_process( const vcl_string &name );

  ~merge_2_vectors_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_v1( const vcl_vector<T> &v1 );
  VIDTK_INPUT_PORT( set_v1, const vcl_vector<T>& );

  void set_v2( const vcl_vector<T> &v2 );
  VIDTK_INPUT_PORT( set_v2, const vcl_vector<T>& );

  vcl_vector<T>& v_all(void);
  VIDTK_OUTPUT_PORT( vcl_vector<T>&, v_all );

private:
  config_block config_;
  
  const vcl_vector<T> *v1_;
  const vcl_vector<T> *v2_;
  vcl_vector<T> v_all_;
};

}  // namespace vidtk

#endif

