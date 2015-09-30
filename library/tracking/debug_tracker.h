/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_debug_tracker_h_
#define vidtk_debug_tracker_h_

extern unsigned g_DT_frame;
extern unsigned g_DT_id;

bool DT_is_track_of_interest(unsigned id);

#endif
