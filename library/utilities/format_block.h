/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_format_block_h_
#define vidtk_format_block_h_

/// \file

#include <string>

namespace vidtk
{


/// \brief Take a (long) message and format it to a fixed-line-length
/// block.
///
/// For example, formatting the string "The quick brown     fox jumped
/// over the moon\n yes, the moon.\nWow." with a prefix of "#-" and a line
/// length of 11 should result in:
/// \code
/// #-The quick
/// #-brown fox
/// #-jumped
/// #-over the
/// #-moon
/// #- yes, the
/// #- moon.
/// #-Wow.
/// \endcode
///
/// Unless the output block is empty, the output will end in a
/// newline, even if the source content does not.
///
/// If the source content is the empty string, the output will also be
/// the empty string.
///
/// \param[in] content Contains the content to be formatted. "\n"
/// represents an explicit new line.  Multiple consecutive spaces are
/// treated as a single space, except those immediately following a
/// newline.
///
/// \param[in] prefix Each output line will be prefixed by this
/// string. The prefix counts toward the line length.
///
/// \param[in] line_length The maximum length of each output line.  If
/// a single word exceeds this length, the word <e> will not </e>
/// be split to fit (i.e. the line length will be exceeded).
std::string
format_block( std::string const& content,
              std::string const& prefix,
              unsigned line_length );


} // end namespace vidtk


#endif // vidtk_format_block_h_
