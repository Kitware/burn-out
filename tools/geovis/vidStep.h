/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidVideoReader - read video frames and return them in vtkImageData
// .SECTION Description
// Latitudes and longitudes are expressed in degrees. Positive latitudes are
// north, positive longitudes are east.

#ifndef vidStep_h
#define vidStep_h

#include "vtkObject.h"

class vidStep : public vtkObject
{
public:
  static vidStep* New();
  vtkTypeRevisionMacro(vidStep,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Frame number relative to a shot. Frame shots start at zero.
  vtkGetMacro(FrameInShot,int);
  vtkSetMacro(FrameInShot,int);
  
  // Description:
  // Frame number relative to the video source.
  vtkGetMacro(FrameInSource,int);
  vtkSetMacro(FrameInSource,int);
  
  // Description:
  // Time stamp (integer values encoded on double). I don't know the unit.
  vtkGetMacro(TimeStamp,double);
  vtkSetMacro(TimeStamp,double);
  
  // Description:
  // Tells if the information about position of sensor and frame corners is
  // valid.
  vtkGetMacro(Valid,bool);
  vtkSetMacro(Valid,bool);
  
  // Description:
  // Sensor latitude.
  vtkGetMacro(SensorLatitude,double);
  vtkSetMacro(SensorLatitude,double);
  
  // Description:
  // Sensor longitude.
  vtkGetMacro(SensorLongitude,double);
  vtkSetMacro(SensorLongitude,double);
  
  // Description:
  // Sensor altitude in meters.
  vtkGetMacro(SensorAltitude,double);
  vtkSetMacro(SensorAltitude,double);
  
  // Description:
  // Frame upper left corner latitude.
  vtkGetMacro(FrameUpperLeftLatitude,double);
  vtkSetMacro(FrameUpperLeftLatitude,double);
  
  // Description:
  // Frame upper left corner longitude.
  vtkGetMacro(FrameUpperLeftLongitude,double);
  vtkSetMacro(FrameUpperLeftLongitude,double);
  
  // Description:
  // Frame upper right corner latitude.
  vtkGetMacro(FrameUpperRightLatitude,double);
  vtkSetMacro(FrameUpperRightLatitude,double);
  
  // Description:
  // Frame upper right corner longitude.
  vtkGetMacro(FrameUpperRightLongitude,double);
  vtkSetMacro(FrameUpperRightLongitude,double);
  
  // Description:
  // Frame lower left corner latitude.
  vtkGetMacro(FrameLowerLeftLatitude,double);
  vtkSetMacro(FrameLowerLeftLatitude,double);
  
  // Description:
  // Frame lower left corner longitude.
  vtkGetMacro(FrameLowerLeftLongitude,double);
  vtkSetMacro(FrameLowerLeftLongitude,double);
  
  // Description:
  // Frame lower right corner latitude.
  vtkGetMacro(FrameLowerRightLatitude,double);
  vtkSetMacro(FrameLowerRightLatitude,double);
  
  // Description:
  // Frame lower right corner longitude.
  vtkGetMacro(FrameLowerRightLongitude,double);
  vtkSetMacro(FrameLowerRightLongitude,double);
  
protected:
  vidStep();
  ~vidStep();
  
  int FrameInShot;
  int FrameInSource;
  double TimeStamp;
  bool Valid;
  double SensorLatitude;
  double SensorLongitude;
  double SensorAltitude;
  double FrameUpperLeftLatitude;
  double FrameUpperLeftLongitude;
  double FrameUpperRightLatitude;
  double FrameUpperRightLongitude;
  double FrameLowerLeftLatitude;
  double FrameLowerLeftLongitude;
  double FrameLowerRightLatitude;
  double FrameLowerRightLongitude;
  
private:
  vidStep(const vidStep&); // Not implemented.
  void operator=(const vidStep&); // Not implemented.
};

#endif
