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

#ifndef vidTrackVisualProperty_h
#define vidTrackVisualProperty_h

#include "vtkObject.h"

class vidTrackVisualProperty : public vtkObject
{
public:
  static vidTrackVisualProperty* New();
  vtkTypeRevisionMacro(vidTrackVisualProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Tell if the track is visible or not.
  // It has priority over box and trace visibility.
  // Initial value is true.
  vtkGetMacro(Visibility,bool);
  vtkSetMacro(Visibility,bool);
  
  // Description:
  // Tell if the track box is visible or not.
  // A Box is really visible if Visibility() && BoxVisibility()
  // Initial value is true.
  vtkGetMacro(BoxVisibility,bool);
  vtkSetMacro(BoxVisibility,bool);

  // Description:
  // Tell if the track trace is visible or not.
  // A trace is really visible if Visibility() && TraceVisibility()
  // Initial value is true.
  vtkGetMacro(TraceVisibility,bool);
  vtkSetMacro(TraceVisibility,bool);
  
  // Description:
  // RGB Color of the trace or box.
  // Initial value is white (1.0,1.0,1.0)
  vtkSetVector3Macro(Color,double);
  vtkGetVector3Macro(Color,double);
  
protected:
  vidTrackVisualProperty();
  ~vidTrackVisualProperty();
  
  bool Visibility;
  bool BoxVisibility;
  bool TraceVisibility;
  double Color[3];
  
private:
  vidTrackVisualProperty(const vidTrackVisualProperty&); // Not implemented.
  void operator=(const vidTrackVisualProperty&); // Not implemented.
};

#endif
