/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include "vil_nitf_metadata_parser.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <vul/vul_reg_exp.h>

#include <boost/algorithm/string.hpp>

bool
vil_nitf_metadata_parser
::parse_latlon(const std::string &metadata, double &lat, double &lon)
{
  std::string str1lat = "X([0-9][0-9])([0-9][0-9])([0-9][0-9]\\.[0-9][0-9])";
  std::string str1lon = "Y([0-9][0-9][0-9])([0-9][0-9])([0-9][0-9]\\.[0-9][0-9])";
  std::string str2lat = "([\\+-][0-9][0-9]\\.[0-9][0-9][0-9][0-9][0-9][0-9])";
  std::string str2lon = "([\\+-][0-9][0-9][0-9]\\.[0-9][0-9][0-9][0-9][0-9][0-9])";

  std::stringstream ss1;
  ss1 << str1lat << str1lon;
  std::stringstream ss2;
  ss2 << str2lat << str2lon;

  vul_reg_exp re1(ss1.str().c_str());
  vul_reg_exp re2(ss2.str().c_str());

  if( re1.find(metadata) )
  {
    double latD, latM, latS, lonD, lonM, lonS;
    std::stringstream ss;
    ss << re1.match(1) << ' ' << re1.match(2) << ' ' <<  re1.match(3)
       << re1.match(4) << ' ' << re1.match(5) << ' ' <<  re1.match(6);
    ss >> latD >> latM >> latS >> lonD >> lonM >> lonS;
    lat = latD + (latM/60.0) + (latS/3600.0);
    lon = lonD + (lonM/60.0) + (lonS/3600.0);
  }
  else if( re2.find(metadata) )
  {
    std::stringstream ss;
    ss << re2.match(1) << ' ' << re2.match(2);
    ss >> lat >> lon;
  }
  else
  {
    return false;
  }

  return true;
}


bool
vil_nitf_metadata_parser
::parse_datetime(const char* metadata, std::time_t &value)
{
  std::string re_str = "([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])([0-9][0-9])([0-9][0-9])([0-9][0-9])";
  vul_reg_exp re(re_str.c_str());
  if( !re.find(metadata) )
  {
    return false;
  }

  std::stringstream conv;

  conv << re.match(1) << ' ' // YYYY
       << re.match(2) << ' ' // MM
       << re.match(3) << ' ' // DD
       << re.match(4) << ' ' // hh
       << re.match(5) << ' ' // mm
       << re.match(6);       // ss

  unsigned int YYYY, MM, DD, hh, mm, ss;
  std::tm tm0, tm1;
  std::memset(&tm0, 0, sizeof(std::tm));
  std::memset(&tm1, 0, sizeof(std::tm));

  tm0.tm_year = 70;
  tm0.tm_mday = 1;

  conv >> YYYY >> MM >> DD >> hh >> mm >> ss;
  tm1.tm_year = YYYY - 1900;
  tm1.tm_mon = MM - 1;
  tm1.tm_mday = DD;
  tm1.tm_hour = hh;
  tm1.tm_min = mm;
  tm1.tm_sec = ss;

  std::time_t time0 = std::mktime(&tm0);
  std::time_t time1 = std::mktime(&tm1);
  value = static_cast<std::time_t>(std::difftime(time1, time0));

  return true;
}


//: Parse the ENGRDA data block
bool
vil_nitf_metadata_parser
::parse_engrda(const char *data, size_t len, vil_nitf_engrda &engrda)
{
  return engrda.parse(data, len);
}

bool
vil_nitf_metadata_parser
::parse_bae_comment(const std::string& meta, double& milli)
{
  //[0]COLLECTION TIMESTAMP: [1]10-10-26T15:[2]10:[3]36.857Z
  std::vector<std::string> temp;
  boost::split(temp, meta, boost::is_any_of(":"));
  for(unsigned int i = 0; i < temp.size(); ++i)
  {
    if("COLLECTION TIMESTAMP" == temp[i])
    {
      i+= 3; //advace to the seconds part
      std::string re_str = "([0-9][0-9])\\.([0-9][0-9][0-9])Z";
      vul_reg_exp re(re_str.c_str());
      if( !re.find(temp[i]) )
      {
        return false;
      }
      std::stringstream ss;
      ss << re.match(2);
      ss >> milli;
      return true;
    }
  }
  return false;
}

