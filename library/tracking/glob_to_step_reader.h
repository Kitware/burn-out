/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_glob_to_step_reader_h_
#define vidtk_glob_to_step_reader_h_

#include <vcl_string.h>
#include <vcl_map.h>

#include <tracking/track_reader.h>

namespace vidtk
{

class glob_to_step_reader : public track_reader
{
  public:
    glob_to_step_reader(track_reader * r = NULL, bool del_here = false);
    ~glob_to_step_reader();
    virtual bool read(vcl_vector<track_sptr>& trks);
  protected:
    vcl_map< unsigned int, vcl_vector<track_sptr> > terminated_at_;
    bool setup_steps();
    vcl_map<unsigned int,vcl_vector<track_sptr> >::iterator current_terminated_;

    track_reader* reader_;
    unsigned int frame_count_;
    bool del_here_;
};

}

#endif
