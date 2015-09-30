/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidMapView - render view with a map and a list of video clips.
// .SECTION Description
// 
#ifndef vidSceneObserver_h
#define vidSceneObserver_h

#include "vtkCommand.h"

class vtkRenderWindow;
class vidMapView;
class vidMonitor;

class vidSceneObserver : public vtkCommand
{
public:
  static vidSceneObserver *New();
  
  virtual void Execute(vtkObject *caller,
                       unsigned long event,
                       void *calldata);
  
  void SetRenderWindow(vtkRenderWindow *w);
  void SetMapView(vidMapView *v);
  
  void SetMonitor(vidMonitor *m);
  
protected:
  vidSceneObserver();
  ~vidSceneObserver();
  
  vtkRenderWindow *RenderWindow;
  vidMapView *MapView;
  vidMonitor *Monitor;
};

#endif // #ifndef vidSceneObserver_h
