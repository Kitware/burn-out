/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_reader_h_
#define track_reader_h_

#include <tracking/track.h>

namespace vidtk
{
  class track_reader
  {
  protected:
    vcl_string filename_;

    bool ignore_occlusions_;
    bool ignore_partial_occlusions_;

    bool been_read_;

    virtual void init()
    {
      ignore_occlusions_ = true;
      ignore_partial_occlusions_ = true;
      been_read_ = false;
    }
  public:
    track_reader()
    {
      init();
    }
    track_reader(const char* name)
    {
      filename_ = name;
      init();
    }
    virtual ~track_reader()
    {
    }
    void set_filename(const char* name)
    {
      filename_ = name;
      been_read_ = false;
    }
    vcl_string filename() const
    {
      return filename_;
    }
    virtual bool read(vcl_vector<track_sptr>& trks) = 0;

    inline void set_ignore_occlusions(bool ignore)
    {
      ignore_occlusions_ = ignore;
    }

    inline void set_ignore_partial_occlusions(bool ignore)
    {
      ignore_partial_occlusions_ = ignore;
    }
  };
}
#endif
