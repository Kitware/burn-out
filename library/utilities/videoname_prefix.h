/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_VIDEONAME_PREFIX_H_
#define _VIDTK_VIDEONAME_PREFIX_H_

#include <string>

namespace vidtk {

class config_block;

// ----------------------------------------------------------------
/*! \brief Manage video name [singleton]
 *
 * This class manages the video name that is used as a prefix for file
 * names. In many cases output files need to be made unique for each
 * input video being processed. This singleton class provides the
 * needed storage and methods for some top level entity, which knows
 * the video file name, to register that and make it available to the
 * highly nested processes that need to make output files unique.
 *
 * In cases where the video name are not known or there is no file
 * name (i.e. processing a live video stream), the prefix defaults to
 * the null string, which works just fine.
 *
 */
class videoname_prefix
{
public:
  ~videoname_prefix();

  /** \brief Get pointer to singleton object.
   *
   * This method returns the pointer to the singleton video name
   * prefix object. An object is created when first accessed.
   *
   * @return Pointer to videoname object.
   */
  static videoname_prefix * instance();

  /** \brief Set string to be used as videoname prefix.
   *
   * This method makes the video name prefix from the supplied file
   * name. The file base name is used as the video prefix after
   * stripping the directory and extension components.
   *
   * @param filename Video file name
   */
  void set_videoname_prefix( std::string const& filename );


  /** \brief Add video name to file name.
   *
   * This method adds the name of the current video being processed as
   * a prefix to the specified config item key.  The string in the
   * config block is modified in place by prefixing the video name.
   *
   * If the string from the config block is empty or the video name
   * prefix is empty, then the config entry is not modified.
   *
   * The main purpose for this operation is to generate unique file
   * names for each video, so the config string is processeed as a
   * file path.
   *
   * @param[in] block - config block to look in for the key
   * @param[in] key - config item name (key) to be modified
   *
   * @retval true - if key was found
   * @retval false - key not found
   */
  bool add_videoname_prefix( config_block& block, std::string const& key );


  /** \brief Get videoname prefix string.
   *
   * This method returns the videoname prefix string.
   *
   * @return The videoname prefix string is returned.
   */
  std::string const& get_videoname_prefix() const;


private:
  videoname_prefix();

  std::string m_video_basename;
  static videoname_prefix * s_singleton_ptr;

}; // end class videoname_prefix

} // end namespace

#endif /* _VIDTK_VIDEONAME_PREFIX_H_ */
