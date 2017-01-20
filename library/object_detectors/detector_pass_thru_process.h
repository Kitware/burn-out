/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_DETECTOR_SUPER_PROCESS_PASS_THRU_H_
#define _VIDTK_DETECTOR_SUPER_PROCESS_PASS_THRU_H_

#include <process_framework/pipeline_aid.h>
#include <object_detectors/detector_super_process_port_def.h>

namespace vidtk
{

// ----------------------------------------------------------------
/**
 * @brief Pass through utility process for detectors
 *
 * This is a utility pass-thru process for use in the object detector
 * implementations. Sometimes the detector implementation does not
 * need or otherwise pass-thru an input, but it must be passed to the
 * output. That is where this process steps in. You can connect the
 * input pads to the output pads through this pass-thru using the
 * following code.
 *
 * \code
#define connect_pass_thru( PAD )                                        \
  p->connect( this->pad_source_ ## PAD->value_port(), proc_pass->set_input_ ## PAD ## _port() ); \
  p->connect( proc_pass->get_output_ ## PAD ## _port(), this->pad_output_ ## PAD ->set_value_port() )
  * \endcode
  *
  * Note that not all defined ports need to be connected. If an input
  * from a pad is used by the implementation, just don't connect it to
  * the pass-thru.
  *
  * Note that this pass-thru process will only work as a bypass for
  * detectors that do not delay the inputs. The detector must produce
  * an output for each input. If the detector delays, then you need a
  * more robust solution.
  */
template< class PixType >
class detector_pass_thru_process
  : public vidtk::process
{
public:
  typedef detector_pass_thru_process self_type;

  // process interface
  detector_pass_thru_process( std::string const& n ) : process( n, "detector_pass_thru_process" ) { }
  virtual ~detector_pass_thru_process() {}

  virtual config_block params() const { return config_block(); }
  virtual bool set_params( config_block const& ) { return true; }
  virtual bool initialize() { return true; }
  virtual bool step() { return true; }


  // set_input_foo()
  // get_output_foo()
#define PT(N,T) VIDTK_PASS_THRU_PORT(T,N);
  DETECTOR_SUPER_PROCESS_OUTPUTS(PT)
#undef PT

  // We need to add a port for the incoming mask_image
  VIDTK_PASS_THRU_PORT( mask_image, vil_image_view< bool > );

};

#undef DETECTOR_SUPER_PROCESS_INPUTS
#undef DETECTOR_SUPER_PROCESS_OUTPUTS

} // end namespace

#endif /* _VIDTK_DETECTOR_SUPER_PROCESS_PASS_THRU_H_ */
