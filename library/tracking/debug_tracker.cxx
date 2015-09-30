/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "debug_tracker.h"

// These variables are used for debug prints in functions where we don't
// have access to the current frame or track number.
unsigned g_DT_frame;
unsigned g_DT_id;

bool DT_is_track_of_interest(unsigned id)
{
  bool ret = false;

  switch( id )
  {
    case 65:
    case 14:
    case 117:
    case 89:
      ret = true;
      break;

    default:
      ret = false;
  }

  return ret;
}
