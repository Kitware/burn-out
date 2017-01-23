/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_modality_h_
#define vidtk_video_modality_h_

#include <string>
#include <ostream>

namespace vidtk
{

enum video_modality { VIDTK_INVALID=0,
                      VIDTK_COMMON,
                      VIDTK_EO_N,
                      VIDTK_EO_M,
                      VIDTK_EO_H,
                      VIDTK_IR_N,
                      VIDTK_IR_M,
                      VIDTK_IR_H,
                      VIDTK_EO_FB,
                      VIDTK_IR_FB,
                      VIDTK_EO,   // unspecified FOV
                      VIDTK_IR,   // unspecified FOV
                      VIDTK_VIDEO_MODALITY_END // used for validity checking
};

std::string video_modality_to_string( video_modality vm );

bool is_video_modality_eo( video_modality vm );
bool is_video_modality_ir( video_modality vm );
bool is_video_modality_invalid( video_modality vm );

inline std::ostream & operator<< (std::ostream & str, video_modality const & obj)
{ str << video_modality_to_string (obj); return str; }

}

#endif // vidtk_video_modality_h_

