/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//image_sequence_accessor.h
#ifndef vidtk_image_sequence_accessor_h_
#define vidtk_image_sequence_accessor_h_

#include<vcl_string.h>
#include<vcl_vector.h>

#include <vil/vil_image_view.h>
/**
\file
\brief
 Image API.
*/


namespace vidtk
{

template<class PixType>
class generic_frame_process;
///Method and field definitions for the image sequence accessor API.
class image_sequence_accessor
{
  public:
    ///Image input type
    enum type{ image_list_glob = 0, image_list_file = 1, stream };
    ///Constructor : pass the name and type.
    image_sequence_accessor(vcl_string fname, type t);
    ///Returns the current image
    vil_image_view<vxl_byte> const& get_image();
    ///Moves the frame pointer to frame_number, returns true if succeeds; otherwise false.
    bool seek_frame(unsigned int frame_number);
    ///Move the frame pointer by one frame, returns true if succeeds; otherwise false.
    bool seek_to_next();
    ///\brief Sets the region of interest (roi) using a string formated as WxH+x+y (IE 100x100+10+10).
    ///
    ///Setting an roi means on pixels in the roi will be returned.
    ///Returns true if succeeds
    bool set_roi(vcl_string roi);
    ///Sets the region of interest (roi) using numbers, returns true if succeeds; otherwise false.
    bool set_roi( unsigned int x, unsigned int y,
                  unsigned int w, unsigned int h );
    ///Returns the region of interest (roi) to be the full frame
    void reset_roi();
    ///Access all the frames in a range, returns true if succeeds; otherwise false;
    bool get_frame_range(unsigned int begin, unsigned int end,
                         vcl_vector< vil_image_view<vxl_byte> > & results);
    ///Returns true
    bool is_valid();
  private:
    generic_frame_process< vxl_byte > * frame_process_;
    bool is_valid_;
};

};

#endif
