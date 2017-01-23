/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "rescale_tracks.h"

#include <tracking_data/image_object.h>
#include <tracking_data/tracking_keys.h>

#include <vil/vil_decimate.h>
#include <vil/vil_resample_bilin.h>
#include <vil/vil_resample_nearest.h>
#include <vil/vil_new.h>

#include <vgl/vgl_box_2d.h>

namespace vidtk
{

template< typename Type >
void
scale_bbox( vgl_box_2d< Type >& bbox, double scale )
{
  bbox = vgl_box_2d< Type >( bbox.min_x() * scale,
                             bbox.max_x() * scale,
                             bbox.min_y() * scale,
                             bbox.max_y() * scale );
}

void
rescale_tracks( const track::vector_t& input_tracks,
                const rescale_tracks_settings& parameters,
                track::vector_t& output_tracks )
{
  output_tracks.clear();

  const double scale_factor = parameters.scale_factor;
  const double scale_factor_sqr = scale_factor * scale_factor;

  for( unsigned i = 0; i < input_tracks.size(); ++i )
  {
    track_sptr filtered_track;

    if( parameters.deep_copy )
    {
      filtered_track = input_tracks[i]->clone();
    }
    else
    {
      filtered_track = input_tracks[i];
    }

    const std::vector< track_state_sptr >& history = filtered_track->history();

    for( unsigned j = 0; j < history.size(); ++j )
    {
      track_state_sptr state = history[j];

      state->img_vel_ = state->img_vel_ * scale_factor;

      state->pixel_loc_ = state->pixel_loc_ * scale_factor;
      state->pixel_vel_ = state->pixel_vel_ * scale_factor;
      state->pixel_var_ = state->pixel_var_ * scale_factor;

      vgl_box_2d< unsigned > modified_bbox;

      if( state->bbox( modified_bbox ) )
      {
        scale_bbox( modified_bbox, scale_factor );
        state->set_bbox( modified_bbox );
      }

      image_object_sptr image_object;

      if( state->image_object( image_object ) )
      {
        image_object->set_image_loc( image_object->get_image_loc() * scale_factor );
        image_object->set_image_area( image_object->get_image_area() * scale_factor_sqr );

        typedef vgl_polygon< image_object::float_type > polygon_t;

        const polygon_t& poly = image_object->get_boundary();

        for( unsigned k = 0; k < poly.num_sheets(); ++k )
        {
          polygon_t::sheet_t sheet = poly[k];

          for( unsigned s = 0; s < sheet.size(); ++s )
          {
            sheet[s] = polygon_t::point_t( sheet[s].x() * scale_factor,
                                           sheet[s].y() * scale_factor );
          }
        }

        if( parameters.resize_chips )
        {
          image_object::image_point_type mask_origin;
          vil_image_view< bool > original_mask;
          image_object->get_object_mask( original_mask, mask_origin );

          if( original_mask.size() != 0 )
          {
            vil_image_view< bool > resized_mask;
            vil_resample_nearest( original_mask, resized_mask,
                                  original_mask.ni() * scale_factor,
                                  original_mask.nj() * scale_factor );

            // set the rescaled mask and origin into the image_object
            image_object::image_point_type scaled_origin;
            scaled_origin.set( mask_origin.x() * scale_factor,
                               mask_origin.y() * scale_factor );

            image_object->set_object_mask( resized_mask, scaled_origin );
          }

          vil_image_resource_sptr image_rsrc;
          unsigned int border;
          if( image_object->get_image_chip( image_rsrc, border) )
          {
            vil_image_view< vxl_byte > image_chip = image_rsrc->get_view();
            vil_image_view< vxl_byte > resized_chip;
            vil_resample_bilin( image_chip, resized_chip,
                                image_chip.ni() * scale_factor,
                                image_chip.nj() * scale_factor );


            image_object->set_image_chip(
              vil_new_image_resource_of_view(resized_chip), border );
          }
        }
      }
    }

    output_tracks.push_back( filtered_track );
  }
}

} // end namespace vidtk
