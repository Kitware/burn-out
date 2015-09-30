/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// ./TestTrackReader /home/fbertel/projects/development/data/virat/q280/Q.280_queryResults.xml

#include <fstream>
#include <iostream>
#include <cassert>

#include "vidQueryResultsReader.h"



int main(int argc,
         char *argv[])
{
  int result=0;
  
  if(argc!=2)
    {
    std::cerr << "track XML filename missing in argument" << std::endl;
    return 1;
    }
  
  std::string fileName=argv[1];
  
  vidQueryResultsReader *reader=vidQueryResultsReader::New();
  reader->SetFileName(&fileName);
  
  reader->ReadXMLFile();
  
  reader->Print(std::cout);
  
  reader->Delete();
  
  // <vibrantDescriptors>
  // list of tracks
  // </vibrantDescriptors>
  
  // <track startFrame="837" lastFrame="892"   id="128385024" frameNumberOrigin="clip">
  // videoid
  // sourcetrackids
  // trackstyle
  // list of bbox
  // list of descriptors
  // 
  // </track>
  
  // <videoID>name</videoID>
  // <sourceTrackIDs> list of ids </sourceTrackIDs>
  // <trackStyle> name </trackStyle>
  
  // <bbox type="raw" frame="837" ulx="228" uly="411" lrx="250" lry="453" timestamp="1221665012111117312" />
  
  // <descriptor type=""> list of elements </descriptor>
  // if descriptor type="classifier", empty
  // if descriptor type="metadataDescriptor": sensorlatitude, sensorlongitude, ul|cornerlonglat,...
  
  
//  xmlNode trackElement // <track> </track>
  
  
  
  return result;
}
