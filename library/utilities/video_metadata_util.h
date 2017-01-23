/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_video_metadata_util_h_
#define vidtk_video_metadata_util_h_

#include <ostream>
#include <istream>

namespace vidtk
{

class video_metadata;

std::ostream& ascii_serialize( std::ostream& str, video_metadata const& vmd );
std::istream& ascii_deserialize( std::istream& str, video_metadata& vmd );
std::istream& ascii_deserialize_red_river( std::istream& str, video_metadata& vmd );

} // end namspace

#endif /* vidtk_video_metadata_util_h_ */
