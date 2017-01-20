/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _vidtk_detector_implementation_base_h_
#define _vidtk_detector_implementation_base_h_

#include <vil/vil_image_view.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <utilities/apply_functor_to_vector_process.h>

#include <tracking_data/image_object.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking_data/gui_frame_info.h>
#include <tracking_data/pixel_feature_array.h>

#include <process_framework/pipeline_aid.h>
#include <pipeline_framework/super_process.h>

#include <object_detectors/detector_super_process_port_def.h>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

// Connect source port to a process
#define connect_to_source(IPORT, PROC, OPORT)                           \
  p->connect( m_source_proc-> IPORT ##_port(), PROC->OPORT ## _port());

// connect two process ports
#define connect_ports(PROC1, PORT1, PROC2, PORT2)                       \
  p->connect( PROC1-> PORT1 ## _port(), PROC2-> PORT2 ## _port() )

  // to be superseded by sink and source connects
#define connect_pad2port(PAD, PROC, PORT)                               \
  p->connect( this->pad_source_ ## PAD->value_port(), PROC->PORT ## _port());

#define connect_port2pad(PROC, PORT, PAD)                               \
  p->connect( PROC->PORT ## _port(), this->pad_output_ ## PAD ->set_value_port() )

// special connect for pass thru
#define connect_pass_thru( PAD )                                        \
  p->connect( this->pad_source_ ## PAD->value_port(), proc_pass->set_input_ ## PAD ## _port() ); \
  p->connect( proc_pass->get_output_ ## PAD ## _port(), this->pad_output_ ## PAD ->set_value_port() )


// ----------------------------------------------------------------
/*! \brief Abstract base detector pipeline implementation.
 *
 * This class represents an object detection pipeline. It contains all
 * the detector independent information, such as the input and output
 * pads.
 *
 * This class provides an interface this is compatible with the
 * traditional super_process, both syntactically and semantically.
 *
 * Template parameter is needed to support templated port types.
 *
 * @tparam PixType data type for pixel data.
 */
template < class PixType >
class detector_implementation_base
{
public:
  typedef boost::shared_ptr< detector_implementation_base < PixType > > sptr;

  /*! \brief Construct detector pipeline.
   *
   * Minimize the amount of work done in the derived class constructor
   * since an object of this type will be created during parameter
   * collection. No point in doing more than is required so that
   * get_params() can be called to return the required
   * config. Actually building the pipeline can be deferred until
   * setup_pipeline() call.
   */
  detector_implementation_base( std::string const& name );
  virtual ~detector_implementation_base();


  /*! \brief Get configs from processes.
   *
   * This method collects the configs from all processes in the
   * pipeline and returns them in a single config block.
   */
  virtual config_block params() = 0;

  /*! \brief Send parameters to implementation.
   *
   * This method supplies updated parameters to the
   * implementation. The implementation will typically instantiate and
   * populate the pipeline.
   *
   * @param blk Updated parameters.
   *
   * @return \b true if success; \b false otherwise.
   */
  virtual bool set_params(config_block const& blk) = 0;


  /*! \brief Reset implementation.
   *
   * This method is called to reset the detector. This is typically done
   * when there is a video modality change. The parameters have already
   * been updated, so all that needs to be done is to reset state and
   * reinitialize using current config values.
   *
   * @return \b true if reset succeed; \b false otherwise.
   */
  virtual bool reset();


  /*! \brief initialize detector.
   *
   * This method is called as part of the pipeline startup process
   * after set_params().
   *
   * @return \b true if reset succeed; \b false otherwise.
   */
  virtual bool initialize();


  /*! \brief  Return pointer to pipeline.
   *
   *
   * @return Pointer to pipeline.
   */
  pipeline_sptr pipeline();


  /*! \brief Get name for this implementation.
   *
   * This method returns the name for this implementation. The name is
   * useful for identifying this instance of a detector.
   *
   * @return The detector name is returned.
   */
  std::string const& name() const;


  //
  // Use the list of detector_super_process ports to drive creation of
  // pad process pointers.  With these macros, we are creating the
  // process pointers for all pads.
  //
  // This pipeline will be integrated into a super-process by
  // attaching input and output port methods to these pads.
  // Input Pads
  //
  // These pads need to be public so that the glue code in the
  // detector_super_process can fetch new values and supply return
  // values.
  //
#define input_pads(T,N) typename vidtk::process_smart_pointer< vidtk::super_process_pad_impl < T > > pad_source_ ## N;

DETECTOR_SUPER_PROCESS_INPUTS(input_pads)
#undef input_pads

  // Output Pads
#define output_pads(T,N) typename vidtk::process_smart_pointer< vidtk::super_process_pad_impl < T > > pad_output_ ## N;

DETECTOR_SUPER_PROCESS_OUTPUTS(output_pads)
#undef output_pads


protected:
  /*! \brief Create all pads.
   *
   * This method creates input and output pads for all connections
   * supported by the detector super process. The pad implementation
   * pointers are initialized by this method.
   *
   * The list of detector_super_process ports is used to drive pad
   * creation.
   */
  void create_base_pads();


  /*! \brief Add pads to pipeline.
   *
   * This method adds all input and output pads to the supplied
   * pipeline. the pipeline is modified in place. create_base_pads()
   * must be called to create the pads before they can be added.
   *
   * The list of detector_super_process ports is used to drive pad
   * addition.
   *
   * @param[in,out] p Add pads to this pipeline.
   */
  template < class Pipeline >
  void setup_base_pipeline( Pipeline* p );


  /// Pointer to Generic pipeline.
  pipeline_sptr m_pipeline;

  /*!
   * \brief Config block for this implementation.
   *
   * It is initially filled in by get_params() method and returned to
   * upper level. It is then updated by the set_params() method with
   * the final config passed down from above.
   */
  config_block m_config;

  /*!
   * \brief Detector source config options
   *
   * This config enum is set up with all the available options from
   * the image_object source_code enum.
   */
  config_enum_option m_detector_source;

  /// Detector name.
  std::string m_name;
};

// Undefine these macros so we don't pollute the name space.
// If they are needed anywhere else, just use include to get definition.
#undef DETECTOR_SUPER_PROCESS_INPUTS
#undef DETECTOR_SUPER_PROCESS_OUTPUTS


// ----------------------------------------------------------------
/*! \brief Functor to set image object source type.
 *
 * This class updates image objects by setting the origin information
 * for the object.  The origin information specifies what type of
 * detector was used and which instance of the detector.
 */
class set_type_functor
{
public:
/**
 * Create new functor object
 *
 * @param type Type code to assign to objects
 * @param inst_name Name of instance that generated this object
 */
  set_type_functor( vidtk::image_object::source_code type,
                    std::string const& inst_name )
    : m_detection_type( type ),
      m_inst_name( inst_name )
  { }

  image_object_sptr operator() ( image_object_sptr const& obj )
  {
    obj->set_source( m_detection_type, m_inst_name );
    return obj;
  }

  /**
   * @brief Set new source for image objects.
   *
   * @param type detector type code
   * @param inst_name detector instance name
   */
  void set_source( vidtk::image_object::source_code type,
                    std::string const& inst_name )
  {
    this->m_detection_type = type;
    this->m_inst_name =  inst_name;
  }


private:
  vidtk::image_object::source_code m_detection_type;
  std::string m_inst_name;
};


// Type for process that sets image object source info.
typedef apply_functor_to_vector_process< image_object_sptr, image_object_sptr, set_type_functor > set_object_type_process;


} // namespace vidtk

#endif // vidtk_detector_implementation_base_h_
