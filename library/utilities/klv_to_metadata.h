/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_klv_to_metadata_h_
#define vidtk_klv_to_metadata_h_

#include <klv/klv_data.h>

namespace vidtk
{

  class video_metadata;

  /// Converts an 0601 klv datablock to video metadata
  bool klv_0601_to_metadata( const klv_data &klv, video_metadata &metadata );

  /// Converts an 0104 klv datablock to video metadata
  bool klv_0104_to_metadata( const klv_data &klv, video_metadata &metadata );
}

#endif //vidtk_klv_to_metadata_h_
