/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_sptr_h_
#define vidtk_track_sptr_h_

#include <vbl/vbl_smart_ptr.h>
#include <vector>

/**
 \file
  \brief Method and field definitions for tracks.
*/

namespace vidtk
{
///Method and field definitions for tracks.
class track;
///Creates name for vbl_smart_ptr<track> type.
typedef vbl_smart_ptr< track > track_sptr;

typedef std::vector < track_sptr > track_vector_t;

}

#endif
