/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidTrackRepresentation - Graphics representation of a track: a
// bounding box on a texture.
// .SECTION Description
// 

#ifndef vidTrackRepresentation_h
#define vidTrackRepresentation_h

#include "vtkObject.h"
class vidTrack;
class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;

class vidTrackRepresentation : public vtkObject
{
public:
  static vidTrackRepresentation *New();
  vtkTypeRevisionMacro(vidTrackRepresentation,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkGetObjectMacro(Track,vidTrack);
  virtual void SetTrack(vidTrack *track);
  
  void UpdateGeometry(int frame);
  
  int GetLastFrame();
  
  vtkGetObjectMacro(Actor,vtkActor);
  vtkGetObjectMacro(TraceActor,vtkActor);
  
  // Description:
  // Set/Get the color.
  vtkSetVector3Macro(Color,double);
  vtkGetVector3Macro(Color,double);
  
  vtkSetMacro(TrackVisibility,bool);
  vtkGetMacro(TrackVisibility,bool);
  
  vtkSetMacro(BoxVisibility,bool);
  vtkGetMacro(BoxVisibility,bool);
  
   // Default is 500.
  double GetZHigh() const;
  void SetZHigh(double value);
  
  void BuildTrace();
  void UpdateActorVisibility();
protected:
  // Description:
  // Default constructor.
  vidTrackRepresentation();
  
  ~vidTrackRepresentation();
  
  int LastFrame;
  vidTrack *Track;
  vtkActor *Actor;
  vtkPolyDataMapper *Mapper;
  vtkPolyData *Quad;
  double Color[3];
  
  vtkActor *TraceActor;
  vtkPolyDataMapper *TraceMapper;
  vtkPolyData *TraceLine;
  
  bool In;
  bool TrackVisibility;
  bool BoxVisibility;
  
  double ZHigh;
  
};

#endif // #define vidTrackRepresentation_h
