/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _vidtk_apply_functor_to_vector_process_h_
#define _vidtk_apply_functor_to_vector_process_h_




#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/**
 * \brief Apply a function to a vector of data
 *
 * This class applies a function to each element of the input vector
 * to produce an output vector from the elements returned from the
 * function.
 *
 * @tparam IN_T Element type for input vector
 * @tparam OUT_T Element type for output vector
 * @tparam Function Name of function that will do the work
 */
template< typename IN_T, typename OUT_T, class FUNCTOR >
class apply_functor_to_vector_process
  : public process
{
public:
  typedef apply_functor_to_vector_process self_type;

  /**
   * \brief Create new applier object.
   *
   * This object takes ownership of the functor object, therefore the
   * functor \b must be allocated from the heap, since it is managed
   * by shared pointer.
   *
   * @param name Instance name of this process.
   * @param func Address of functor object
   *
   * @return
   */
  apply_functor_to_vector_process( std::string const& name,
                                   boost::shared_ptr< FUNCTOR > func );
  virtual ~apply_functor_to_vector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void input( std::vector<IN_T> const& in );
  VIDTK_INPUT_PORT( input, std::vector<IN_T> const& );

  std::vector<OUT_T> output(void);
  VIDTK_OUTPUT_PORT( std::vector<OUT_T>, output );

  void reset_functor( boost::shared_ptr< FUNCTOR > func );

private:
  config_block config_;

  const std::vector<IN_T> *in_;
  std::vector<OUT_T> out_;

  boost::shared_ptr<FUNCTOR> functor_;
};

}

#endif
