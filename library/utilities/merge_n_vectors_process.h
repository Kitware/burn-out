/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_MERGE_NVECTORS_PROCESS_H_
#define _VIDTK_MERGE_NVECTORS_PROCESS_H_

#include <string>
#include <utility>
#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <boost/preprocessor/repetition/repeat.hpp>

//
// This macro is used for debugging. If it is defined before including
// this file, the code will be executed on each vector added to the
// merge. This is the easiest way of determining which source vector
// has contributed bad input to the resulting vector.
//
#if !defined(CHECK_VECTOR)
#define CHECK_VECTOR(I,V)
#endif

#define MACRO_MERGE_N_INPUTS_ADD_VECTOR(Z, I, DK)                       \
  void add_vector_ ## I( const std::vector<T> &v1 )                     \
  { received_input_[I] = true; v_in_[I] = v1; CHECK_VECTOR(I, v1) }     \
  VIDTK_INPUT_PORT( add_vector_ ## I, const std::vector<T>& );

/// combine N vectors into a single vector
#define VIDTK_MERGE_N_VECTOR(N)                                 \
namespace vidtk                                                 \
{                                                               \
                                                                \
template<typename T>                                            \
class merge_ ## N ## _vectors_process : public process          \
{                                                               \
public:                                                         \
  typedef merge_ ## N ##_vectors_process self_type;             \
                                                                \
  merge_ ## N ##_vectors_process( const std::string &name );    \
                                                                \
  virtual ~merge_## N ##_vectors_process()                      \
  {}                                                            \
                                                                \
  virtual config_block params() const                           \
  { return this->config_; }                                     \
                                                                \
  virtual bool set_params( const config_block &)                \
  { return true; }                                              \
                                                                \
  virtual bool initialize()                                     \
  { return true; }                                              \
                                                                \
  virtual bool step();                                          \
                                                                \
  BOOST_PP_REPEAT(N, MACRO_MERGE_N_INPUTS_ADD_VECTOR, void)     \
                                                                \
  std::vector<T> v_all(void);                                   \
  VIDTK_OUTPUT_PORT( std::vector<T>, v_all );                   \
                                                                \
private:                                                        \
  void clear();                                                 \
  config_block config_;                                         \
  bool received_input_[N];                                      \
  std::vector<T> v_in_[N];                                      \
  std::vector<T> v_all_;                                        \
};                                                              \
}  // namespace vidtk

#define VIDTK_MERGE_N_VECTOR_IMPL(N)                                    \
namespace vidtk                                                         \
{                                                                       \
                                                                        \
template<typename T>                                                    \
merge_ ## N ## _vectors_process<T>                                      \
::merge_ ## N ## _vectors_process( const std::string &_name )           \
: process(_name, "merge_" #N "_vectors_process")                        \
{                                                                       \
  clear();                                                              \
}                                                                       \
                                                                        \
template<typename T>                                                    \
bool                                                                    \
merge_ ## N ## _vectors_process<T>                                      \
::step()                                                                \
{                                                                       \
  bool result = false;                                                  \
                                                                        \
  this->v_all_.clear();                                                 \
  for( unsigned int i = 0; i < N; ++i )                                 \
  {                                                                     \
    this->v_all_.insert( this->v_all_.end(), this->v_in_[i].begin(),    \
                         this->v_in_[i].end() );                        \
    result = result || received_input_[i];                              \
  }                                                                     \
                                                                        \
  clear();                                                              \
  return result;                                                        \
}                                                                       \
                                                                        \
template<typename T>                                                    \
void                                                                    \
merge_ ## N ## _vectors_process<T>                                      \
::clear( )                                                              \
{                                                                       \
  for(unsigned int i = 0; i < N; ++i)                                   \
  {                                                                     \
    v_in_[i].clear();                                                   \
    received_input_[i] = false;                                         \
  }                                                                     \
}                                                                       \
                                                                        \
template<typename T>                                                    \
std::vector<T>                                                          \
merge_  ## N ## _vectors_process<T>                                     \
::v_all( void )                                                         \
{                                                                       \
  return this->v_all_;                                                  \
}                                                                       \
                                                                        \
}


// ----------------------------------------------------------------
/*! \brief Instantiate merge vector process.
 *
 * Use this macro to instantiate the merge vector process for the
 * desired number of inputs and vector datum type.
 */

#define INSTANTIATE_MERGE_N_VECTOR(N,T)                         \
VIDTK_MERGE_N_VECTOR(N)                                         \
VIDTK_MERGE_N_VECTOR_IMPL(N)                                    \
template class vidtk::merge_ ## N ## _vectors_process<T>;

#endif
