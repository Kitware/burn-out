/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_DEMUX_PAIRS_PROCESS_H_
#define _VIDTK_DEMUX_PAIRS_PROCESS_H_

#include <string>
#include <utility>
#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Seperate a vector of pairs into two seperate vectors
template<typename T1, typename T2>
class demux_pairs_process : public process
{
public:
  typedef demux_pairs_process self_type;
  typedef std::pair<T1, T2> std::pair_t1_t2;

  demux_pairs_process( const std::string &name );

  ~demux_pairs_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_pairs( std::vector<std::pair_t1_t2> &p );
  VIDTK_INPUT_PORT( set_pairs, std::vector<std::pair_t1_t2>& );

  std::vector<T1>& first(void);
  VIDTK_OUTPUT_PORT( std::vector<T1>&, first );

  std::vector<T2>& second(void);
  VIDTK_OUTPUT_PORT( std::vector<T2>&, second );

private:
  config_block config_;

  std::vector<std::pair<T1, T2> > *p_;
  std::vector<T1> first_;
  std::vector<T2> second_;
};

}  // namespace vidtk

#endif

