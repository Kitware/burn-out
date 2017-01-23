/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_TRACK_STATE_UTIL_H_
#define _VIDTK_TRACK_STATE_UTIL_H_

#include <tracking_data/track_state.h>
#include <utilities/geo_coordinate.h>

namespace vidtk {

  /**
   * @brief Get lat/lon from image object.
   *
   * @param track
   * @param is_utm
   * @param zone
   * @param is_north
   *
   * @return
   */
  geo_coord::geo_coordinate_sptr get_lat_lon( vidtk::track_state_sptr track,
                                              bool is_utm,
                                              int zone,
                                              bool is_north );
} // end namespace

#endif /* _VIDTK_TRACK_STATE_UTIL_H_ */
