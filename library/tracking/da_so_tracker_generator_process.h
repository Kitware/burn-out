/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_da_so_tracker_generator_process_h_
#define vidtk_da_so_tracker_generator_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/da_so_tracker_generator.h>

#include <vcl_vector.h>

namespace vidtk
{

class da_so_tracker_generator_process
  : public process
{
  public:
    typedef da_so_tracker_generator_process self_type;

    da_so_tracker_generator_process( vcl_string name );
    ~da_so_tracker_generator_process();

    virtual config_block params() const;

    virtual bool set_params( config_block const& );

    virtual bool initialize();
    
    virtual bool reset();

    virtual bool step();

    /// Set of new tracks to initialize trackers on.
    void set_new_tracks( vcl_vector< track_sptr > const& tracks );
    VIDTK_INPUT_PORT( set_new_tracks, vcl_vector< track_sptr > const& );

    /// Set of active trackers.
    vcl_vector< da_so_tracker_sptr > const& new_trackers() const;
    VIDTK_OUTPUT_PORT( vcl_vector< da_so_tracker_sptr > const&, new_trackers );

    //Replicating the above trackers for width-height kalman

    /// Set of active trackers.
    vcl_vector< da_so_tracker_sptr > const& new_wh_trackers() const;
    VIDTK_OUTPUT_PORT( vcl_vector< da_so_tracker_sptr > const&, new_wh_trackers );


  protected:
    config_block config_;
    da_so_tracker_generator generator_;
     da_so_tracker_generator wh_generator_;
    bool disabled_;

    bool wh_disabled_;

    /// New tracks on which to initialize new trackers
    vcl_vector< track_sptr > const* new_trks_;

    /// Set of new trackers.
    vcl_vector< da_so_tracker_sptr > trackers_;

     /// Set of new trackers for width - height Kalman
    vcl_vector< da_so_tracker_sptr > wh_trackers_;
};

} //namespace vidtk

#endif
