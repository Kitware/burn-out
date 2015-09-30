/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vsl_reader_h_
#define vsl_reader_h_

#include <vcl_vector.h>
#include <vsl/vsl_fwd.h>
#include <tracking/track.h>

#include "track_reader.h"

namespace vidtk
{
  class vsl_reader : public track_reader
  {
  public: 
    vsl_reader() 
    : track_reader(), bin_(NULL)
    {
    }
    
    vsl_reader(const char* name) 
    : track_reader(name), bin_(NULL) 
    {
    }
    
    bool read(vcl_vector< track_sptr >& trks);
  
  private:
    vsl_b_istream *bin_;
  };
}

#endif

