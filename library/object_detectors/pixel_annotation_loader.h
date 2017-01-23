/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pixel_annotation_loader_h_
#define vidtk_pixel_annotation_loader_h_

#include <vil/vil_image_view.h>

#include <vector>
#include <map>
#include <string>

#include <boost/format.hpp>

namespace vidtk
{


/// Settings for the pixel_annotation_loader class.
struct pixel_annotation_loader_settings
{
  /// Is the input path a directory containing images, or a filelist?
  enum { DIRECTORY, FILELIST } load_mode;

  /// Is the image type a raw boolean mask or RGB colors?
  ///
  /// If specified as pure colors, each color is given an annotation ID
  /// according to the following conversion table:
  ///
  ///  BACKGROUND_ID = 0,
  ///  GREEN_PIXEL_ID = 1,
  ///  PIXELS BORDERING GREEN = 2,
  ///  YELLOW_PIXEL_ID = 3,
  ///  BLUE_PIXEL_ID = 4,
  ///  RED_PIXEL_ID = 5,
  ///  PINK_PIXEL_ID = 6
  ///
  /// If using a boolean mask, any non-zero pixels will be given a label
  /// value of 1.
  enum { MASK, COLOR_EXTREMA } image_type;

  std::string path;
  std::string pattern;

  /// Set default settings
  pixel_annotation_loader_settings()
  : load_mode( DIRECTORY ),
    image_type( COLOR_EXTREMA ),
    path( "." ),
    pattern( "gt-%d.png" )
  {}
};

/// Given a filename for some GT image, load an annotated ID image.
bool load_gt_image( const std::string& filename,
                    vil_image_view<unsigned>& id_image,
                    const bool is_mask_image = true );


/// \brief A class for assisting with loading pixel-level annotations for
/// a video, either from a filelist or from a directory of labeled images.
///
/// This class functions as a state machine, and should be used by first
/// specifying either a filelist or directory at the start of a video.
/// For every frame in the video, the get_annotation_for_frame function
/// should then be called.
class pixel_annotation_loader
{
public:

  pixel_annotation_loader() {}
  virtual ~pixel_annotation_loader() {}

  /// Set any options for the pixel annotation loader.
  virtual bool configure( const pixel_annotation_loader_settings& settings );

  /// Get an annotated image for a given frame, if possible.
  virtual bool get_annotation_for_frame( const unsigned frame_number,
                                         vil_image_view<unsigned>& ids );

private:

  pixel_annotation_loader_settings settings_;
  std::map< unsigned, std::string > frame_to_filename_;
  boost::format pattern_;
};


} // end namespace vidtk

#endif // vidtk_pixel_annotation_loader_h_
