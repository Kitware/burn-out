/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_frame_process_sp_facade_interface_h_
#define vidtk_frame_process_sp_facade_interface_h_

#include <pipeline_framework/super_process.h>
#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <vil/vil_image_view.h>


namespace vidtk
{


/// \brief Interface defining plug-in requirements for a super process to be used
/// as a frame_process.
///
/// This pure-virtual interface defines method requirements for the sub-classer
/// so that it may be used with the frame_process_sp_facade wrapper class. This
/// interface does not have any enforcement that the object that uses this is a
/// super_process, but this is enforced in frame_process_sp_facade
/// instantiation.
///
template< class PixType >
class frame_process_sp_facade_interface
{
public:
  frame_process_sp_facade_interface() { }
  virtual ~frame_process_sp_facade_interface() { }

  // Required methods:
  // NOTE: See frame_process.h for descriptions of methods
  virtual config_block params() const = 0;
  virtual bool set_params( config_block const& blk ) = 0;
  virtual bool initialize() = 0;
  virtual bool step() { return false; };
  virtual process::step_status step2() = 0;
  virtual bool seek( unsigned frame_number ) = 0;
  virtual vidtk::timestamp timestamp() const = 0;
  virtual vidtk::video_metadata metadata() const = 0;
  virtual vil_image_view<PixType> image() const = 0;
  virtual unsigned ni() const = 0;
  virtual unsigned nj() const = 0;

}; // end class frame_process_sp_facade_interface


} //end namespace vidtk


#endif // vidtk_frame_process_sp_facade_interface_h_
