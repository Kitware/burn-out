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

#ifndef vidMapView_h
#define vidMapView_h

#include "vtkRenderView.h"

class vtkPlaneSource;
class vtkPolyDataMapper;
class vtkActor;
class vtkTexture;
class vtkImageData;
class vidSceneObserver;
class vidMonitor;

// vidVideoFrame: a frame with timestamp
// vidRichedVideoFrame: frame+geolocalisation metainformation
// vidRichedVideo: geolocalisation of each frame.

class vidMapView : public vtkRenderView
{
public:
  vtkTypeRevisionMacro(vidMapView,vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vidMapView *New();
  
  // Description:
  // map texture
  void SetMapTexture(vtkImageData *texture);
  
  // Description:
  // video texture
  void SetTexture(vtkTexture *texture);
  
  // Description:
  // collection of riched video
//  vtkstd::vector<vtkSmartPointer<vidRichedVideo> > *Videos;
  
  // Description:
  // Override the superclass to call render on the render window only
  // if RenderFlag is on. To avoid double renderwindow when interacting
  // while a animation is playing.
  // See vidSceneObserver::Execute().
  virtual void Render();
  
  // Description:
  // From superclass. Overidden to update the texture of the map.
  virtual void Update();
  
  vidSceneObserver *GetObserver() const;
  
  // Description:
  // Use by the vidSceneObserver only. Initial value is true.
  vtkGetMacro(RenderFlag,bool);
  vtkSetMacro(RenderFlag,bool);
  
  void SetMonitor(vidMonitor *m);
  
protected:
  vidMapView();
  ~vidMapView();
  
  vtkActor *MapActor;
  vtkPolyDataMapper *MapMapper;
  vtkPlaneSource *MapSource;
  
  vtkImageData *MapTexture;
  
  vidSceneObserver *Observer;
  
  bool RenderFlag;
  
private:
  vidMapView(const vidMapView&);  // Not implemented.
  void operator=(const vidMapView&);  // Not implemented.
};

#endif
