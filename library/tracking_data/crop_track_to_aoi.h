/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef crop_track_to_aoi_h_
#define crop_track_to_aoi_h_

#include <vector>

#include <tracking_data/track.h>

#include <utilities/aoi.h>

namespace vidtk
{

///Function to crop a track to be with-in an aoi.
class crop_track_to_aoi
{
public:
  crop_track_to_aoi(aoi::intersection_mode m = aoi::UTM)
  :mode_(m)
  {}
  //Crops track geographically to the aoi.
  //If the track is not in the aoi, returns null
  track_sptr operator()(track_sptr,aoi_sptr) const;
private:
  aoi::intersection_mode mode_;
};

} //namespace vidtk

#endif
