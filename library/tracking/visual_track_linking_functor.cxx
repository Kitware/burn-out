/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//visual_track_linking_functor.cxx

#include "visual_track_linking_functor.h"

#include <tracking/tracking_keys.h>

#include <utilities/log.h>

#include <vnl/vnl_double_2.h>

namespace vidtk 
{

double
visual_track_linking_functor
::correlation( vil_image_view<vxl_byte> const & data_x,
               vil_image_view<vxl_byte> const & data_y,
               vil_image_view<bool> const & mask,
               unsigned int x_xbegin, unsigned int x_ybegin,
               unsigned int y_xoffset, unsigned int y_yoffset ) const
{
  double count = 0;
  double mean_x = 0;
  double mean_y = 0;
  for(unsigned int x = 0; x < mask.ni(); ++x)
  {
    for(unsigned int y = 0; y < mask.nj(); ++y)
    {
      if(mask(x,y) && x+y_xoffset < data_y.ni() && y+y_yoffset < data_y.nj())
      {
        count++;
        mean_x += data_x( x + x_xbegin,  y + x_ybegin,  0 );
        mean_y += data_y( x + y_xoffset, y + y_yoffset, 0 );
      }
    }
  }
  mean_x = mean_x / count;
  mean_y = mean_y / count;
  double var_x = 0;
  double var_y = 0;
  double sxy = 0;
  for(unsigned int x = 0; x < mask.ni(); ++x)
  {
    for(unsigned int y = 0; y < mask.nj(); ++y)
    {
      if(mask(x,y) && x+y_xoffset < data_y.ni() && y+y_yoffset < data_y.nj())
      {
        double vx = data_x( x + x_xbegin,  y + x_ybegin,  0 );
        double vy= data_y( x + y_xoffset, y + y_yoffset, 0 );
        var_x += (vx - mean_x)*(vx - mean_x);
        var_y += (vy - mean_y)*(vy - mean_y);
        sxy += (vx - mean_x)*(vy - mean_y);
      }
    }
  }
  var_x = var_x/count;
  var_y = var_y/count;
  return sxy/(vcl_sqrt(var_x)*vcl_sqrt(var_y)*(count/*-1*/));
}

double
visual_track_linking_functor
::find_best( vil_image_view<vxl_byte> x, vil_image_view<vxl_byte> y,
             vil_image_view<bool> bmask, unsigned int x_xbegin,
             unsigned int x_ybegin ) const
{
  double best = -2;
  if(bmask.ni() < y.ni() && bmask.nj() < y.nj())
  {
    for(unsigned int i = 0; i < y.ni() - bmask.ni(); ++i)
    {
      for(unsigned int j = 0; j < y.nj() - bmask.nj(); ++j)
      {
        double temp = correlation( x, y, bmask, x_xbegin, x_ybegin, i, j );
        if(temp > best)
        {
          best = temp;
        }
      }
    }
  }
  return best;
}


vnl_double_2
visual_track_linking_functor
::normalize_cross_correlation( vcl_vector<vidtk::image_object_sptr> const & objs_end,
                               vcl_vector<vidtk::image_object_sptr> const & objs_start ) const
{
  vil_image_view<vxl_byte> data_end, data_start;
  log_assert(objs_end[0]->data_.has(vidtk::tracking_keys::pixel_data), "Failed to add image info to start of track\n");
  objs_end[0]->data_.get(vidtk::tracking_keys::pixel_data, data_end);
  log_assert(objs_start[0]->data_.has(vidtk::tracking_keys::pixel_data), "Failed to add image info to end of track\n");
  objs_start[0]->data_.get(vidtk::tracking_keys::pixel_data, data_start);
  unsigned int offset_end = static_cast<unsigned int>(-1), 
               offset_start = static_cast<unsigned int>(-1);
  log_assert(objs_end[0]->data_.has( vidtk::tracking_keys::pixel_data_buffer),
                                     "Failed to add pixel data buffer info to end of track\n");
  objs_end[0]->data_.get(vidtk::tracking_keys::pixel_data_buffer, offset_end);
  log_assert(objs_start[0]->data_.has(vidtk::tracking_keys::pixel_data_buffer),
                                      "Failed to add pixel data buffer info to start of track\n");
  objs_start[0]->data_.get(vidtk::tracking_keys::pixel_data_buffer, offset_start);
  unsigned int offset_end_x = offset_end, offset_end_y = offset_end;
  unsigned int offset_start_x = offset_start, offset_start_y = offset_start;
  if(offset_end_x > objs_end[0]->bbox_.min_x())
  {
    offset_end_x = objs_end[0]->bbox_.min_x();
  }
  if(offset_end_y > objs_end[0]->bbox_.min_y())
  {
    offset_end_y = objs_end[0]->bbox_.min_y();
  }

  if(offset_start_x > objs_start[0]->bbox_.min_x())
  {
    offset_start_x = objs_start[0]->bbox_.min_x();
  }
  if(offset_start_x > objs_start[0]->bbox_.min_y())
  {
    offset_start_x = objs_start[0]->bbox_.min_y();
  }

  vnl_double_2 r(find_best(data_end, data_start, objs_end[0]->mask_, offset_end_x, offset_end_y),
                 find_best(data_start, data_end, objs_start[0]->mask_, offset_start_x, offset_start_y) );
  return r;
}

void
visual_track_linking_functor
::mean_var( vil_image_view<vxl_byte> const & data, vil_image_view<bool> const & mask,
            unsigned int xbegin, unsigned int ybegin, double & mean, double & var ) const
{
  double count = 0.0;
  mean = 0.0;
  var = 0.0;
  for( unsigned int i = 0; i < mask.ni(); ++i )
  {
    for( unsigned int j = 0; j < mask.nj(); ++j )
    {
      if(mask(i,j))
      {
        mean += data(i+xbegin,j+ybegin,0);
        count++;
      }
    }
  }
  mean = mean/count;
  for( unsigned int i = 0; i < mask.ni(); ++i )
  {
    for( unsigned int j = 0; j < mask.nj(); ++j )
    {
      if(mask(i,j))
      {
        var += (data(i+xbegin,j+ybegin,0) - mean) * (data(i+xbegin,j+ybegin,0) - mean);
      }
    }
  }
  var = var/count;
}

void
visual_track_linking_functor
::mean_var( vcl_vector<vidtk::image_object_sptr> const & objs, 
            double & mean, double & var ) const
{
  vil_image_view<vxl_byte> data;
  log_assert(objs[0]->data_.has(vidtk::tracking_keys::pixel_data), "Failed to add image info to track\n");
  objs[0]->data_.get(vidtk::tracking_keys::pixel_data, data);
  unsigned int offset = static_cast<unsigned int>(-1);
  log_assert( objs[0]->data_.has(vidtk::tracking_keys::pixel_data_buffer),
              "Failed to add pixel data buffer info to track\n");
  objs[0]->data_.get(vidtk::tracking_keys::pixel_data_buffer, offset);
  unsigned int offset_x = offset, offset_y = offset;
  if(offset_x > objs[0]->bbox_.min_x())
  {
    offset_x = objs[0]->bbox_.min_x();
  }
  if(offset_y > objs[0]->bbox_.min_y())
  {
    offset_y = objs[0]->bbox_.min_y();
  }
  mean_var( data, objs[0]->mask_, offset_x, offset_y, mean, var );
}

double
visual_track_linking_functor
::operator()( TrackTypePointer track_i, TrackTypePointer track_j )
{
  double cost = 0.0;

  TrackStateList track_i_states = track_i->history();
  TrackStateList track_j_states = track_j->history();

  TrackState track_i_last_state = track_i_states.back();
  TrackState track_j_first_state = track_j_states.front();

  // Check temporal constraint.
  // track_i must end before track_j begins.

  if( track_j_first_state->time_.time_in_secs() < track_i_last_state->time_.time_in_secs() ) 
  {
    return impossible_match_cost_;
  }
  else 
  {

    vcl_vector<vidtk::image_object_sptr> track_i_last_objs, track_j_first_objs;
    track_i_last_state->data_.get(vidtk::tracking_keys::img_objs, track_i_last_objs);
    track_j_first_state->data_.get(vidtk::tracking_keys::img_objs, track_j_first_objs);

    // Full patch means and std diff
    double track_i_last_mean, track_i_last_var;
    mean_var( track_i_last_objs, track_i_last_mean, track_i_last_var );
    double track_j_first_mean, track_j_first_var;
    mean_var( track_j_first_objs, track_j_first_mean, track_j_first_var );
    double tmp = vnl_math::sqr(track_i_last_mean-track_j_first_mean);
    double ave_meh_distance = ( vcl_sqrt(tmp/track_i_last_var)+ vcl_sqrt(tmp/track_j_first_var) )*0.5;
    tmp = vnl_math::sqr(vcl_sqrt(track_i_last_var) - vcl_sqrt(track_j_first_var));
    double ave_chi_sqr = ( vcl_sqrt(tmp/track_i_last_var) + vcl_sqrt(tmp/track_j_first_var) )*0.5;

    //area
    double max_area, min_area;
    if(track_i_last_objs[0]->area_ > track_j_first_objs[0]->area_)
    {
      max_area = track_i_last_objs[0]->area_;
      min_area = track_j_first_objs[0]->area_;
    }
    else
    {
      min_area = track_i_last_objs[0]->area_;
      max_area = track_j_first_objs[0]->area_;
    }
    double tam = min_area/max_area;
    // Cost is length of the difference vector
    cost = mean_dist_penalty_ * ave_meh_distance +
           chi_penalty_ * ave_chi_sqr +
           area_penalty_ * tam;
  }

  

  if(cost > impossible_match_cost_)
  {
    return impossible_match_cost_;
  }

  return cost;
}

visual_track_linking_functor
::visual_track_linking_functor( double area_penalty,
                                double mean_dist_penalty,
                                double chi_penalty )
   : track_linking_functor(1000.0), area_penalty_(area_penalty),
     mean_dist_penalty_(mean_dist_penalty), chi_penalty_(chi_penalty)
{
}

}

