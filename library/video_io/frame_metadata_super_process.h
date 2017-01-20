/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_metadata_super_process_h_
#define vidtk_frame_metadata_super_process_h_


#include <pipeline_framework/super_process.h>

#include <process_framework/pipeline_aid.h>
#include <process_framework/process.h>

#include <utilities/config_block.h>

#include <string>

#include <video_io/frame_process_sp_facade_interface.h>


namespace vidtk
{

template< class PixType >
class frame_metadata_super_process_impl;

// ----------------------------------------------------------------
/*! \brief NITF reader process
 *
 *
 */
template< class PixType >
class frame_metadata_super_process
: public super_process,
  public frame_process_sp_facade_interface<PixType>
{
public:
  typedef frame_metadata_super_process< PixType > self_type;

  frame_metadata_super_process( std::string const& name );
  virtual ~frame_metadata_super_process();

  // Methods required by both super_process and the fp interface
  config_block params() const;
  bool set_params( config_block const& blk );
  bool initialize() { return this->super_process::initialize(); }

  process::step_status step2();
  bool step()
  {
    return this->step2() == process::SUCCESS;
  }

  // Output methods and ports
  vidtk::timestamp timestamp() const;
  vidtk::video_metadata metadata() const;
  vil_image_view<PixType> image() const;

  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );
  VIDTK_OUTPUT_PORT( vidtk::video_metadata, metadata );
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

  // frame_process_sp_facade_interface specific methods
  bool seek( unsigned frame_number );
  double frame_rate() const;
  unsigned nframes() const;
  unsigned ni() const;
  unsigned nj() const;

private:
  frame_metadata_super_process_impl<PixType> *impl_;

};

}

#endif
