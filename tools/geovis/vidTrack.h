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

#ifndef vidTrack_h
#define vidTrack_h

#include "vtkObject.h"

#include <vtkstd/vector>
#include "vtkSmartPointer.h"
#include "vidBBox.h"

class vidTrackMetaData;

class vidTrack : public vtkObject
{
public:
  static vidTrack* New();
  vtkTypeRevisionMacro(vidTrack,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Id of the first frame.
  vtkGetMacro(StartFrame,int);
  vtkSetMacro(StartFrame,int);
  
  // Description:
  // Id of the last frame.
  vtkGetMacro(LastFrame,int);
  vtkSetMacro(LastFrame,int);
  
  // Description:
  // Id of the track.
  vtkGetMacro(Id,int);
  vtkSetMacro(Id,int);
  
  // Description:
  // List of bounding boxes.
  vtkstd::vector<vtkSmartPointer<vidBBox> > *GetBoxes();
  
  vidTrackMetaData *GetMetaData();
  
protected:
  vidTrack();
  ~vidTrack();
  
  int StartFrame;
  int LastFrame;
  int Id;
  
  vtkstd::vector<vtkSmartPointer<vidBBox> > *Boxes;
  vidTrackMetaData *MetaData;
  
  
private:
  vidTrack(const vidTrack&); // Not implemented.
  void operator=(const vidTrack&); // Not implemented.
};

#endif
