/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_SPLIT_VECTOR_PROCESS_H_
#define _VIDTK_SPLIT_VECTOR_PROCESS_H_

#include <string>
#include <functional>
#include <vector>

#include <boost/function.hpp>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Seperate a vector into one of three categories based off of a predicate
/// function returning -, 0, or +
template<typename T, int Predicate(const T&)>
class split_vector_process : public process
{
public:
  typedef split_vector_process self_type;

  split_vector_process( const std::string &name);

  ~split_vector_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_input( const std::vector<T> &in );
  VIDTK_INPUT_PORT( set_input, const std::vector<T>& );

  std::vector<T>& positive(void);
  VIDTK_OUTPUT_PORT( std::vector<T>&, positive );

  std::vector<T>& zero(void);
  VIDTK_OUTPUT_PORT( std::vector<T>&, zero );

  std::vector<T>& negative(void);
  VIDTK_OUTPUT_PORT( std::vector<T>&, negative );

private:
  config_block config_;

  const std::vector<T> *in_;
  std::vector<T> positive_;
  std::vector<T> zero_;
  std::vector<T> negative_;
};

}  // namespace vidtk

#endif

