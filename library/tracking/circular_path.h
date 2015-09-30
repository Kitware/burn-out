/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef circular_path_h
#define circular_path_h

#include <vnl/vnl_double_2.h>

#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>

#include <vcl_string.h>
#include <vcl_cmath.h>

#include <vnl/vnl_double_2x2.h>

namespace vidtk
{
class circular_path
{
  public:
    circular_path()
      : center_(150.,150.), radius_(60), current_direction_(1.,1.), speed_(3.),
        clockwise_(false)
    {
      config_.add_parameter("radius", "0", "The radius of the circle");
      config_.add_parameter("clockwise", "true", "move around the cirle in a clockwise motion");
      config_.add_parameter("center", "150. 150.", "The center of the circle");
      config_.add_parameter("initial_radius_direction", "0.707107 0.707107", "The initial direction the radius is pointing");
      config_.add_parameter("speed", "4.0", "How far to move for a unit of time.");
      current_direction_.normalize();
    }
    config_block params() const
    {
      return config_;
    }
    vcl_string name() const
    { return "circular_path"; }
    bool set_params( config_block const& blk)
    {
      try
      {
        speed_ = blk.get< double >( "speed" );
        clockwise_ = blk.get< bool >( "clockwise" );
        radius_= blk.get< double >( "radius" );
        current_direction_ = blk.get< vnl_double_2 >( "initial_radius_direction" );
        center_ = blk.get< vnl_double_2 >( "center" );
      }
      catch( unchecked_return_value & )
      {
        // restore previous state
        this->set_params( config_ );
        return false;
      }
      current_direction_.normalize();
      return true;
    }

    vnl_double_2 current() const
    {
      return center_ + radius_*current_direction_;
    }

    vnl_double_2 advance(double delta_t)
    {
      double l = delta_t*speed_;
      double ldr = l/radius_;
      vnl_double_2x2 rot;
      double cos_ldr = vcl_cos(ldr);
      double sin_ldr = vcl_sin(ldr);
      if( clockwise_ )
      {
        rot(0,0) = cos_ldr; rot(0,1) = -sin_ldr;
        rot(1,0) = sin_ldr; rot(1,1) =  cos_ldr;
      }
      else
      {
        rot(0,0) =  cos_ldr; rot(0,1) = sin_ldr;
        rot(1,0) = -sin_ldr; rot(1,1) = cos_ldr;
      }
      current_direction_ = rot*current_direction_;
      current_direction_.normalize();
      return this->current();
    }

    void set_speed(double s)
    {
      speed_ = s;
    }

    vnl_double_2 get_velocity() const
    {
      vnl_double_2 r;
      //TODO:  Need to do this
      return r;
    }

    double get_speed() const
    {
      return speed_;
    }
  protected:
    config_block config_;

    vnl_double_2 center_;
    double radius_;
    vnl_double_2 current_direction_;
    double speed_;
    bool clockwise_;
};



} //namespace vidtk

#endif
