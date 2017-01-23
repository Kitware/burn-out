/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_VIDTK_ORACLE_ADAPTER_H
#define INCL_VIDTK_ORACLE_ADAPTER_H

///
/// This maps vidtk tracks into track_oracle data_terms.
///
/// This lives in vidtk rather than track_oracle, because it's much
/// more tightly coupled to vidtk than track_oracle, and because
/// it's not really a file format per se.
///

#include <track_oracle/track_base.h>
#include <track_oracle/file_format_base.h>
#include <track_oracle/file_format_manager.h>
#include <track_oracle/data_terms/data_terms.h>
#include <track_oracle/state_flags.h>

namespace vidtk
{

struct vidtk_track_schema_t: public vidtk::track_base< vidtk_track_schema_t >
{
  track_field< vidtk::dt::tracking::external_id > external_id;
  track_field< vidtk::dt::tracking::track_uuid > unique_id;

  track_field< vidtk::dt::tracking::frame_number > frame_number;
  track_field< vidtk::dt::tracking::timestamp_usecs > timestamp_usecs;
  track_field< vidtk::dt::tracking::obj_location > obj_location;
  track_field< vidtk::dt::tracking::bounding_box > bounding_box;
  track_field< vidtk::dt::tracking::world_x > world_x;
  track_field< vidtk::dt::tracking::world_y > world_y;
  track_field< vidtk::dt::tracking::world_z > world_z;
  track_field< vidtk::dt::tracking::world_gcs > world_gcs;
  track_field< vidtk::dt::tracking::velocity_x > velocity_x;
  track_field< vidtk::dt::tracking::velocity_y > velocity_y;
  track_field< vidtk::dt::tracking::fg_mask_area > fg_mask_area;
  track_field< vidtk::dt::utility::state_flags > attributes;

  vidtk_track_schema_t()
  {
    Track.add_field( external_id );
    Track.add_field( unique_id );

    Frame.add_field( frame_number );
    Frame.add_field( timestamp_usecs );
    Frame.add_field( obj_location );
    Frame.add_field( bounding_box );
    Frame.add_field( world_x );
    Frame.add_field( world_y );
    Frame.add_field( world_z );
    Frame.add_field( world_gcs );
    Frame.add_field( velocity_x );
    Frame.add_field( velocity_y );
    Frame.add_field( fg_mask_area );
    Frame.add_field( attributes );
  }
};

} // vidtk


#endif
