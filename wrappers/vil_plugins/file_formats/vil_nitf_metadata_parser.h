/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#ifndef VIL_CORE_FILE_FORMATS_NITF_METADATA_PARSER_H_
#define VIL_CORE_FILE_FORMATS_NITF_METADATA_PARSER_H_

#include <string>
#include <ctime>
#include "vil_nitf_engrda.h"

class vil_nitf_metadata_parser
{
public:
  //: Parse a single set of lat / lon coordinates in the NITF ASCII format
  static bool parse_latlon(const std::string &meta, double &lat, double &lon);

  //: Parse a date-time string in NITF ASCII format
  static bool parse_datetime(const char* meta, std::time_t &time);

  //: Parse the ENGRDA data block
  static bool parse_engrda(const char* data, size_t len,
                           vil_nitf_engrda &engrda);

  //: Parse the ENGRDA data block
  static bool parse_engrda(const std::string &meta, vil_nitf_engrda &engrda)
  {
    return vil_nitf_metadata_parser::parse_engrda(meta.c_str(), meta.size(), engrda);
  }

  static bool parse_bae_comment(const std::string& meta, double& milli);
};

#endif
