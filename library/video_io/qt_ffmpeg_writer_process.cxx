/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "qt_ffmpeg_writer_process.h"

#ifdef Q_OS_WIN
  #include <QEventLoop>
  #include <QFile>
  #include <QFutureWatcher>
  #include <QtConcurrentRun>
#else
  #include <QProcess>
#endif

#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QStringList>
#include <QTimer>

#include <logger/logger.h>

VIDTK_LOGGER( "qt_ffmpeg_writer_process" );

static const qint64 MAX_WRITE_BUFFER = 1<<24;
static const int CHECK_CANCEL_INTERVAL = 150;

static const char* const FRAME_TRANSPORT_FORMAT = "bmp";

namespace vidtk
{

class qt_ffmpeg_writer_process::priv
{
public:

  // Constructors and destructors
  priv()
  : frame_rate( 29.97 ),
    encoding_args( "" ),
    output_encoding( false ),
    completed_count( 0 ),
    total_count( 0 ),
    last_logged( -1 ),
    started( false ),
#ifdef Q_OS_WIN
    pipe_handle( 0 ),
    pipe( &wrapper )
#else
    pipe( &process )
#endif
  {}

  ~priv() {}

#ifdef Q_OS_WIN
  template <typename T> void wait( QFuture<T> future );
#endif

  config_block config;

  QString output_path;
  QString ffmpeg_path;
  qreal frame_rate;
  QStringList encoding_args;
  bool output_encoding;

  image_t input;
  int completed_count;
  int total_count;
  int last_logged;
  bool started;

#ifdef Q_OS_WIN
  FILE* pipe_handle;
  QFile wrapper;
#else
  QProcess process;
#endif

  QIODevice* const pipe;
};


qt_ffmpeg_writer_process
::qt_ffmpeg_writer_process( std::string const& _name )
  :process( _name, "qt_ffmpeg_writer_process" ),
    d( new priv() )
{
  d->config.add_parameter(
    "filename",
    "",
    "Output filename." );
  d->config.add_parameter(
    "frame_rate",
    "29.97",
    "Frame rate used for encoding." );
  d->config.add_parameter(
    "encoding_args",
    "",
    "Any ffmpeg encoding arguments we want to apply to the video encoding "
    "process. This can be used to over-ride default bit-rates, codecs, etc., "
    "see ffmpeg documentation for more detail." );
  d->config.add_parameter(
    "output_encoding",
    "false",
    "Whether or not to output ffmpeg encoding log to console." );
  d->config.add_parameter(
    "ffmpeg_location",
    "ffmpeg",
    "This parameter can be used to over-ride which ffmpeg version is being"
    "used for encoding. By default, the ffmpeg in the path is used." );
}

qt_ffmpeg_writer_process
::~qt_ffmpeg_writer_process()
{
  this->join();
}

config_block
qt_ffmpeg_writer_process
::params() const
{
  return d->config;
}

bool
qt_ffmpeg_writer_process
::set_params( config_block const& blk )
{
  try
  {
    d->output_path = QString::fromStdString( blk.get< std::string >( "filename" ) );
    d->ffmpeg_path = QString::fromStdString(blk.get< std::string >( "ffmpeg_location" ));
    d->frame_rate = blk.get< qreal >( "frame_rate" );
    d->output_encoding = blk.get< bool >( "output_encoding" );

    d->encoding_args = QStringList(
      QString::fromStdString(
        blk.get< std::string >( "encoding_args" ) ) );

    if (d->ffmpeg_path != "ffmpeg")
    {
      d->ffmpeg_path = "\"" + d->ffmpeg_path + "\"";
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  d->completed_count = 0;
  d->last_logged = -1;
  d->config.update( blk );
  return true;
}


bool
qt_ffmpeg_writer_process
::initialize()
{
  return start();
}


QImage
convert_image( const vil_image_view< vxl_byte >& vi )
{
  // Check that the image is valid, and is a supported format
  if( !( vi.is_contiguous() && vi.memory_chunk() && vi.memory_chunk()->data() &&
        ( vi.pixel_format() == VIL_PIXEL_FORMAT_BYTE ||
          vi.pixel_format() == VIL_PIXEL_FORMAT_RGB_BYTE ||
          vi.pixel_format() == VIL_PIXEL_FORMAT_RGBA_BYTE ) ) )
  {
    qDebug() << "ERROR: Cannot convert image, invalid type!";
    return QImage();
  }

  if( vi.nplanes() != 1 && vi.nplanes() != 3 )
  {
    qDebug() << "vxlImage::toQImage: don't know how to convert image with"
             << vi.nplanes() << "planes";
    return QImage();
  }

  const unsigned char* const raw_pixels = vi.top_left_ptr();
  QSize const size( vi.ni(), vi.nj() );

  if( vi.planestep() == 1 && vi.istep() == 3 )
  {
    return QImage( raw_pixels, size.width(), size.height(),
                   vi.jstep(), QImage::Format_RGB888 );
  }

  if (vi.planestep() == -1 && vi.istep() == 4)
  {
    return QImage( raw_pixels - 2, size.width(), size.height(),
                   vi.jstep(), QImage::Format_RGB32 );
  }

  QImage qi(size, QImage::Format_RGB32);
  const int iStep = vi.istep();
  const int jStep = vi.jstep();
  const int planeStep = (vi.nplanes() == 1 ? 0 : vi.planestep());
  const int width = size.width();

  QRgb* const newPixels = reinterpret_cast<QRgb*>( qi.bits() );

  int y = vi.nj();
  while (y--)
  {
    const unsigned char* const rawScanLine = raw_pixels + (y * jStep);
    QRgb* const newScanLine = newPixels + (y * width);

    int x = vi.ni();
    while (x--)
    {
      const unsigned char* const pixoff = rawScanLine + (x * iStep);
      const unsigned char r = *(pixoff + (0 * planeStep));
      const unsigned char g = *(pixoff + (1 * planeStep));
      const unsigned char b = *(pixoff + (2 * planeStep));
      newScanLine[x] = qRgb(r, g, b);
    }
  }

  return qi;
}


bool
qt_ffmpeg_writer_process
::step()
{
  if( !d->input )
  {
    return false;
  }
  else
  {
    if( !convert_image(d->input).save(d->pipe, FRAME_TRANSPORT_FORMAT) )
    {
      LOG_ERROR( "Unable to write image to ffmpeg pipe, check encoding options." );
      return false;
    }

    while( d->pipe->bytesToWrite() > 0 )
    {
      d->pipe->waitForBytesWritten( 1000 );

#ifndef Q_OS_WIN
      if( d->process.state() == QProcess::NotRunning )
      {
        LOG_ERROR( "Unable to write to ffmpeg pipe" );
        return false;
      }
#endif
    }

    d->completed_count++;

    if( d->total_count > 0 )
    {
      int percent = static_cast< int >( ( 100.0 * d->completed_count ) / d->total_count );

      percent = std::min( 100, std::max( 0, percent ) );

      if( percent > d->last_logged && percent % 5 == 0 )
      {
        qDebug( "Percent complete: %02d%%", percent );
        d->last_logged = percent;
      }
    }
  }

  d->input = image_t();
  return true;
}


void
qt_ffmpeg_writer_process
::set_frame( vil_image_view< vxl_byte > const& img )
{
  d->input = img;
}


QStringList
default_codec_args_for_ext( const QString& filename )
{
  QStringList args;

  const QString ext = QFileInfo(filename).suffix().toLower();
  if (ext == "webm")
    {
    args << "-c:v" << "libvpx" // VP8
         << "-b:v" << "16M" << "-qmin" << "0" << "-qmax" << "24";
    }
  else if (ext == "mp4")
    {
    args << "-c:v" << "libx264" // MPEG4 part 10 a.k.a. H.264
         << "-b:v" << "16M" << "-qmin" << "0" << "-qmax" << "24";
    }
  else if (ext == "wmv")
    {
    args << "-c:v" << "wmv2" // Windows Media Video 8
         << "-q:v" << "2";
    }
  else if (ext == "mpg" || ext =="mpeg" || ext =="mpeg2" )
    {
      args << "-c:v" << "mpeg2video" // MPEG2
           << "-q:v" << "2";
    }
  else if (ext == "avi")
    {
    args << "-q:v" << "2";
    }
  else
    {
    LOG_ERROR( "Default codec not specified for file extension: "
               << ext.toStdString()
               << ", not specifying codec for ffmpeg." );
    }

  return args;
}


#ifdef Q_OS_WIN

template <typename T>
void qt_ffmpeg_writer_process::priv::wait( QFuture<T> future )
{
  //if (future.isRunning())
  {
    future.waitForFinished();
  }
}

#endif


bool
qt_ffmpeg_writer_process
::start()
{
  if( d->started )
  {
    qWarning() << "qt_ffmpeg_writer_process::start:"
               << "cannot start aprocess that was already started";
    return false;
  }

  // Prepare arguments for encodeprocess
  QStringList encodeArgs;
  const QString frame_rate_str = QString::number( d->frame_rate, 'f', 6 );

  encodeArgs
#ifdef Q_OS_WIN
    // Name of executable (only needed on Windows, because _popen wants a
    // complete command string; elsewhere, it is specified separately)
    << d->ffmpeg_path
#endif
    // Input FPS
    << "-r" << frame_rate_str
    // Input source options
    << "-f" << "image2pipe" << "-c:v" << FRAME_TRANSPORT_FORMAT << "-i" << "-"
    // Encoding options
    << ( d->encoding_args.last().size() == 0 ?
         default_codec_args_for_ext( d->output_path ) :
         d->encoding_args )
    // Output FPS
    << "-r" << frame_rate_str
    // Output file name
    << "-y" << d->output_path
#ifdef Q_OS_WIN
    << (d->output_encoding ? "" : " 2> NUL" );
#else
    ;
#endif

#ifdef Q_OS_WIN

  // Set up encodeprocess
  d->pipe_handle = _popen( qPrintable( encodeArgs.join(" ") ), "wb" );

  if( ! d->pipe_handle )
  {
    LOG_ERROR( "Unable to set up ffmpeg writing pipe" );
    return false;
  }

  // Bind a QIODevice to the file handle for easier writing to the pipe
  d->wrapper.open( d->pipe_handle, QIODevice::WriteOnly );

  if (!d->wrapper.isOpen() || !d->wrapper.isWritable())
  {
    LOG_ERROR("Unable to open pipe for writing");
    return false;
  }

  d->started = true;

#else

  // Set up encodeprocess
  d->process.setProcessChannelMode( d->output_encoding ? QProcess::ForwardedChannels : QProcess::SeparateChannels );
  d->process.start( "ffmpeg", encodeArgs );
  d->started = d->process.waitForStarted();

#endif

  return d->started;
}


void
qt_ffmpeg_writer_process::join()
{
#ifdef Q_OS_WIN

  if( d->pipe_handle )
  {
    // Close the QFile wrapper... this flushes the QIODevice buffers but does
    // NOT close the underlying file handle (which we want to do separately
    // anyway, as it is not truly a file, but a pipe)
    //d->wait( QtConcurrent::run( &d->wrapper, &QFile::close ) );
    d->wrapper.close();

    // Now close the actual pipe, and clear the handle so we know that it is
    // closed
    //d->wait( QtConcurrent::run( &_pclose, d->pipe_handle ) );
    _pclose( d->pipe_handle );
    d->pipe_handle = 0;
  }

#else

  // Close the write channel and wait for the process to complete
  d->process.closeWriteChannel();
  d->process.waitForFinished( -1 );

#endif
}


int
qt_ffmpeg_writer_process
::completed_count() const
{
  return d->completed_count;
}


void
qt_ffmpeg_writer_process
::set_frame_count( int count )
{
  d->total_count = count;
}


} // end namespace vidtk
