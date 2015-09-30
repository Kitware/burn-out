/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <QApplication>
#include <QCleanlooksStyle>
#include "VIRATGeovis.h"

// ./VIRATGeovis ~/projects/development/KWVision/vidtk/tools/geovis/world.topo.bathy.200407.3x5000x2500.jpg ~/projects/development/data/virat/kitware.png --Mockup ~/projects/development/data/virat/an07c01-s000-005589-006386-di-raw.avi ~/projects/development/data/virat/an02c01-s002-002601-003900-di-raw.avi ~/projects/development/data/virat/an02c01-s002-002601-003900-di-raw.avi ~/projects/development/data/virat/an07c01-s000-005589-006386-di-raw.avi

// ./VIRATGeovis ~/projects/development/KWVision/vidtk/tools/geovis/world.topo.bathy.200407.3x5000x2500.jpg ~/projects/development/data/virat/kitware.png --Meta ~/projects/development/data/virat/shotTest/output-0000010.avi ~/projects/development/data/virat/shotTest/output-0000010-metadata.txt

// ./VIRATGeovis ~/projects/development/KWVision/vidtk/tools/geovis/world.topo.bathy.200407.3x5000x2500.jpg ~/projects/development/data/virat/kitware.png --Query ~/projects/development/data/virat/q280/Q280.mpg ~/projects/development/data/virat/q280/Q.280_queryResults.xml

extern int qInitResources_icons();

int main( int argc, char** argv )
{
  // QT Stuff
  QApplication app( argc, argv );

  char *imageFile = 0;
  char *imageFile2 = 0;
  char *videoFile = 0;
  char *videoFile2 = 0;
  char *videoFile3 = 0;
  char *videoFile4 = 0;
  char *metaFile=0;
  char *queryResultFile=0;
  
  bool meta=false;
  bool query=false;
  
  if (argc == 8 || argc == 6)
    {
    imageFile = argv[1];
    imageFile2 = argv[2];
    std::string option=argv[3];
    videoFile=argv[4];
    if(option.compare("--Mockup")==0)
      {
      videoFile2=argv[5];
      videoFile3=argv[6];
      videoFile4=argv[7];
      }
    if(option.compare("--Meta")==0)
      {
      meta=true;
      metaFile=argv[5];
      }
    else // --Query
      {
      query=true;
      queryResultFile=argv[5];
      }
    }
  else
    {
    cerr << "Usage: VIRATGeovis <background-image> <map-image> [--Mockup <video-file1> <video-file2> <video-file3> <video-file4> | --Meta <video-file> <meta-file> ] | --Query <video-file> <queryresult-file>" << endl;
    return 0;
    }

  QApplication::setStyle(new QCleanlooksStyle);
  
  qInitResources_icons();
  
  VIRATGeovis myVIRATGeovis;
  if(imageFile!=0)
    {
    myVIRATGeovis.loadImage(imageFile);
    }
  if(imageFile2!=0)
    {
    myVIRATGeovis.loadMapImage(imageFile2);
    }
  if(meta)
    {
    if(videoFile!=0 && metaFile!=0)
      {
      vtkstd::string videoFileString(videoFile);
      vtkstd::string metaFileString(metaFile);
      myVIRATGeovis.AddVideoAndMetaFileName(&videoFileString,&metaFileString);
      }
    }
  else if(query)
    {
    if(videoFile!=0 && queryResultFile!=0)
      {
      vtkstd::string videoFileString(videoFile);
      vtkstd::string queryResultFileString(queryResultFile);
      myVIRATGeovis.AddVideoAndQueryResultFileName(&videoFileString,&queryResultFileString);
      }
    }
  else
    {
    if(videoFile!=0)
      {
      vtkstd::string videoFileString(videoFile);
      myVIRATGeovis.AddVideoFileName(&videoFileString);
      }
    }
  if(videoFile2!=0)
    {
    vtkstd::string videoFileString2(videoFile2);
    myVIRATGeovis.AddVideoFileName(&videoFileString2);
    }
  if(videoFile3!=0)
    {
    vtkstd::string videoFileString3(videoFile3);
    myVIRATGeovis.AddVideoFileName(&videoFileString3);
    }
  if(videoFile4!=0)
    {
    vtkstd::string videoFileString4(videoFile4);
    myVIRATGeovis.AddVideoFileName(&videoFileString4);
    }
  myVIRATGeovis.show();
  
  return app.exec();
}
