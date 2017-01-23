/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_qt_ffmpeg_writer_process_h_
#define vidtk_qt_ffmpeg_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <boost/scoped_ptr.hpp>

#include <QtGlobal>
#include <QObject>

namespace vidtk
{

class qt_ffmpeg_writer_process
  : public process
{

public:

  typedef qt_ffmpeg_writer_process self_type;
  typedef vil_image_view< vxl_byte > image_t;

  explicit qt_ffmpeg_writer_process( std::string const& name );
  ~qt_ffmpeg_writer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_frame( image_t const& img );
  VIDTK_OPTIONAL_INPUT_PORT( set_frame, image_t const& );

  /// Get number of frames that have been processed.
  virtual int completed_count() const;

  /// Set total estimated number of frames which need to be processed
  virtual void set_frame_count( int count );

protected:

  /// Start the encode process.
  ///
  /// \param mode Specifies the encoding mode.
  ///
  /// \return \c true if the encode process was started successfully; \c false
  ///         if an error occurred.
  ///
  /// \sa Mode
  virtual bool start();

  /// Wait for process to complete.
  ///
  /// \note Calling this method is mandatory if the process was started. If the
  ///       process is running, and join() is not called explicitly, it will be
  ///       executed when the instance is destroyed.
  virtual void join();

private:

  // Internal member variables
  class priv;
  boost::scoped_ptr< priv > d;

};

} // end namespace vidtk

#endif // vidtk_qt_ffmpeg_writer_process_h_
