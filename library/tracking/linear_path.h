/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef linear_path_h
#define linear_path_h

#include <vnl/vnl_double_2.h>

#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>

#include <vcl_string.h>

namespace vidtk
{
class linear_path
{
  public:
    linear_path()
      :direction_(1.,1.), current_location_(10.,5.), speed_(4.)
    {
      config_.add_parameter("direction", "0.707107 0.707107", "The direction of the linear path");
      config_.add_parameter("initial_location", "10.0 5.0", "The initial location of the path");
      config_.add_parameter("speed", "4.0", "How far to move for a unit of time.");
      direction_.normalize();
    }
    config_block params() const
    {
      return config_;
    }
    vcl_string name() const
    { return "linear_path"; }
    bool set_params( config_block const& blk)
    {
      try
      {
        speed_ = blk.get< double >( "speed" );
        current_location_ = blk.get< vnl_double_2 >( "initial_location" );
        direction_ = blk.get< vnl_double_2 >( "direction" );
      }
      catch( unchecked_return_value & )
      {
        // restore previous state
        this->set_params( config_ );
        return false;
      }

      config_.update( blk );
      direction_.normalize();

      return true;
    }
    vnl_double_2 current() const
    {
      return current_location_;
    }
    vnl_double_2 advance(double delta_t)
    {
      current_location_ = current_location_ + (speed_*delta_t*direction_);
      return current_location_;
    }
    void set_speed(double s)
    {
      speed_ = s;
    }
    void set_direction( vnl_double_2 const & dir)
    {
      direction_ = dir;
    }
    void set_current_location( vnl_double_2 const & loc )
    {
      current_location_ = loc;
    }

    vnl_double_2 get_velocity() const
    {
      return speed_*direction_;
    }

    double get_speed() const
    {
      return speed_;
    }
  protected:
    config_block config_;

    vnl_double_2 direction_;
    vnl_double_2 current_location_;
    double speed_;
};

} //namespace vidtk

#endif
