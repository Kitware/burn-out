/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_modality_h_
#define vidtk_video_modality_h_

#include <vcl_string.h>
#include <vcl_ostream.h>

namespace vidtk
{
  
enum video_modality{VIDTK_INVALID=0,
                    VIDTK_COMMON,
                    VIDTK_EO_N,
                    VIDTK_EO_M,
                    VIDTK_IR_N,
                    VIDTK_IR_M,
                    VIDTK_EO_FB,
                    VIDTK_IR_FB,
                    VIDTK_VIDEO_MODALITY_END };

vcl_string video_modality_to_string( video_modality vm );

inline vcl_ostream & operator<< (vcl_ostream & str, video_modality const & obj)
{ str << video_modality_to_string (obj); return str; }

}

#endif // vidtk_video_modality_h_

