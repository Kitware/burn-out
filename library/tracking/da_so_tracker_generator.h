/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_da_so_tracker_generator_h_
#define vidtk_da_so_tracker_generator_h_

#include <tracking/da_so_tracker.h>
#include <tracking/track.h>
#include <tracking/da_so_tracker_kalman.h>
#include <tracking/wh_tracker_kalman.h>
#include <tracking/da_so_tracker_kalman_extended.h>
#include <tracking/extended_kalman_functions.h>

#include <vbl/vbl_smart_ptr.h>
#include <vbl/vbl_ref_count.h>

#include <vcl_string.h>

#include <utilities/config_block.h>

namespace vidtk
{

class da_so_tracker_generator
  : public vbl_ref_count
{
  public:
    da_so_tracker_generator();
    ~da_so_tracker_generator();

    void set_kalman_params(da_so_tracker_kalman::const_parameters_sptr);

    bool set_so_type(vcl_string type);

    da_so_tracker_sptr generate( track_sptr init_track );

    vcl_string get_so_type() const;

    vcl_string get_options() const;

    ///This function adds the parameters for each of its
    ///possible single object trackers to the config block
    ///and this generator
    void add_default_parameters(config_block & block);

    ///This updates the parameters based on the config_block
    bool update_params(config_block const & block);

    void set_to_default();

  protected:

    struct so_tracker_types
    {
      enum type{ kalman, wh_kalman, ekalman_speed_heading };
    };

    so_tracker_types::type tracker_to_generate_;

    da_so_tracker_sptr generate_kalman( track_sptr init_track );
    da_so_tracker_sptr generate_wh_kalman( track_sptr init_track );
    da_so_tracker_sptr generate_speed_heading( track_sptr init_track );

    da_so_tracker_kalman::const_parameters_sptr kalman_parameters_;
    wh_tracker_kalman::const_parameters_sptr wh_kalman_parameters_;
    da_so_tracker_kalman_extended<extended_kalman_functions::speed_heading_fun>::const_parameters_sptr ekalman_heading_parameters_;
};

typedef vbl_smart_ptr< da_so_tracker_generator > da_so_tracker_generator_sptr;

} //namespace vidtk

#endif //vidtk_da_so_tracker_generator_h_
