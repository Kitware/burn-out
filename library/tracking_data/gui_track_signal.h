/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gui_track_signal_h_
#define vidtk_gui_track_signal_h_

#include <utilities/timestamp.h>
#include <utilities/uuid_able.h>

#include <vgl/vgl_point_2d.h>

#include <vector>
#include <string>

#include <ostream>

#include <boost/function.hpp>

namespace vidtk
{

/// A signal received from some GUI, which tells us we want to either
/// initialize a track for a certain object on some frame, or stop
/// tracking all objects altogether.
class gui_track_signal
{
public:

  /// The type of message as described below:
  ///
  /// USER_CLICK indicates a user has clicked a point on some frame
  /// STOP indicates we want to stop tracking anything
  /// SHUTDOWN indicates we want to turn the system off
  /// ERROR indicates an error has occured
  enum message_type { STOP, USER_CLICK, SHUTDOWN, ERROR_CONDITION };
  message_type message_type_;

  /// The type of the user designated object, if known.
  enum object_type { PERSON, VEHICLE, BUILDING, UNKNOWN };
  object_type object_type_;

  /// Timestamp of the user annotation.
  vidtk::timestamp ts_;

  /// Point location of the object, in the image plane.
  vgl_point_2d< unsigned > location_;

  /// ID of the request
  unsigned qid_;

  /// An optional UUID we should assign to the track.
  std::string uuid_;

  /// Are two signals equivalent?
  bool operator==( const gui_track_signal& other ) const;

  /// Does this gui signal have a timestamp lower than another?
  bool operator<( const gui_track_signal& other ) const;

  /// Does this gui signal have a timestamp higher than another?
  bool operator>( const gui_track_signal& other ) const;

  gui_track_signal()
  : message_type_( ERROR_CONDITION ),
    object_type_( UNKNOWN ),
    qid_( 0 )
  {}
};

/// Output stream operator for the gui_track_signal class.
std::ostream& operator<<( std::ostream& str, const vidtk::gui_track_signal& obj );

/// A vector of GUI user clicks for a given frame
typedef std::vector< vgl_point_2d<unsigned> > gui_specified_loc_list;

/// A vector of GUI track signals for a given frame
typedef std::vector< gui_track_signal > gui_track_signal_list;

/// A function returning a vector of gui signals
typedef boost::function< bool (std::vector< gui_track_signal >&) > gui_get_signals_func;

/// A function which takes a single gui track signal as an argument
typedef boost::function< void (const gui_track_signal&) > gui_track_signal_func;

}

#endif
