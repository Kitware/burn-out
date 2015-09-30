/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidTrackMetaData - Meta data coming with a track
// .SECTION Description
// Latitudes and longitudes are expressed in degrees. Positive latitudes are
// north, positive longitudes are east.

// similar structure is vidStep.

#ifndef vidTrackMetaData_h
#define vidTrackMetaData_h

#include "vtkObject.h"

class vidTrackMetaData : public vtkObject
{
public:
  static vidTrackMetaData* New();
  vtkTypeRevisionMacro(vidTrackMetaData,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
   // Description:
  // Time stamp (integer values encoded on double). I don't know the unit.
  vtkGetMacro(TimeStamp,double);
  vtkSetMacro(TimeStamp,double);
  
  // Description:
  // Sensor latitude.
  vtkGetMacro(SensorLatitude,double);
  vtkSetMacro(SensorLatitude,double);
  
  // Description:
  // Sensor longitude.
  vtkGetMacro(SensorLongitude,double);
  vtkSetMacro(SensorLongitude,double);
  
  // Description:
  // Frame upper left corner latitude.
  vtkGetMacro(UpperLeftLatitude,double);
  vtkSetMacro(UpperLeftLatitude,double);
  
  // Description:
  // Frame upper left corner longitude.
  vtkGetMacro(UpperLeftLongitude,double);
  vtkSetMacro(UpperLeftLongitude,double);
  
  // Description:
  // Frame upper right corner latitude.
  vtkGetMacro(UpperRightLatitude,double);
  vtkSetMacro(UpperRightLatitude,double);
  
  // Description:
  // Frame upper right corner longitude.
  vtkGetMacro(UpperRightLongitude,double);
  vtkSetMacro(UpperRightLongitude,double);
  
  // Description:
  // Frame lower left corner latitude.
  vtkGetMacro(LowerLeftLatitude,double);
  vtkSetMacro(LowerLeftLatitude,double);
  
  // Description:
  // Frame lower left corner longitude.
  vtkGetMacro(LowerLeftLongitude,double);
  vtkSetMacro(LowerLeftLongitude,double);
  
  // Description:
  // Frame lower right corner latitude.
  vtkGetMacro(LowerRightLatitude,double);
  vtkSetMacro(LowerRightLatitude,double);
  
  // Description:
  // Frame lower right corner longitude.
  vtkGetMacro(LowerRightLongitude,double);
  vtkSetMacro(LowerRightLongitude,double);
  
  // Description:
  // Horizontal field of view of the sensor.
  // Don't know if it is deg. or rad.
  vtkGetMacro(HorizontalFieldOfView,double);
  vtkSetMacro(HorizontalFieldOfView,double);
  
  // Description:
  // Vertical field of view of the sensor.
  // Don't know if it is deg. or rad.
  vtkGetMacro(VerticalFieldOfView,double);
  vtkSetMacro(VerticalFieldOfView,double);
  
  // Description:
  // No idea what this stuff is???
  vtkGetMacro(SlantRange,double);
  vtkSetMacro(SlantRange,double);
  
protected:
  vidTrackMetaData();
  ~vidTrackMetaData();
  
  double TimeStamp;
  double SensorLatitude;
  double SensorLongitude;
  double UpperLeftLatitude;
  double UpperLeftLongitude;
  double UpperRightLatitude;
  double UpperRightLongitude;
  double LowerLeftLatitude;
  double LowerLeftLongitude;
  double LowerRightLatitude;
  double LowerRightLongitude;
  double HorizontalFieldOfView;
  double VerticalFieldOfView;
  double SlantRange;
  
private:
  vidTrackMetaData(const vidTrackMetaData&); // Not implemented.
  void operator=(const vidTrackMetaData&); // Not implemented.
};

#endif
