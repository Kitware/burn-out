/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidSceneObserver.h"

#include "vtkRenderWindow.h"
#include <QApplication>
#include "vidMapView.h"
#include "vidMonitor.h"

// ----------------------------------------------------------------------------
vidSceneObserver *vidSceneObserver::New()
{
  return new vidSceneObserver;
}

// ----------------------------------------------------------------------------
vidSceneObserver::vidSceneObserver()
{
  this->RenderWindow=0;
  this->MapView=0;
  this->Monitor=0;
}

// ----------------------------------------------------------------------------
vidSceneObserver::~vidSceneObserver()
{
  if(this->RenderWindow!=0)
    {
    this->RenderWindow->Delete();
    }
  if(this->MapView!=0)
    {
    this->MapView->Delete();
    }
  if(this->Monitor!=0)
    {
    this->Monitor->Delete();
    }
}

// ----------------------------------------------------------------------------
void vidSceneObserver::SetRenderWindow(vtkRenderWindow *w)
{
  if(this->RenderWindow!=w)
    {
    if(this->RenderWindow!=0)
      {
      this->RenderWindow->Delete();
      }
    this->RenderWindow=w;
    if(this->RenderWindow!=0)
      {
      this->RenderWindow->Register(this);
      }
    }
}

// ----------------------------------------------------------------------------
void vidSceneObserver::SetMapView(vidMapView *v)
{
  if(this->MapView!=v)
    {
    if(this->MapView!=0)
      {
      this->MapView->Delete();
      }
    this->MapView=v;
    if(this->MapView!=0)
      {
      this->MapView->Register(this);
      }
    }
}

// ----------------------------------------------------------------------------
void vidSceneObserver::SetMonitor(vidMonitor *m)
{
  if(this->Monitor!=m)
    {
    if(this->Monitor!=0)
      {
      this->Monitor->Delete();
      }
    this->Monitor=m;
    if(this->Monitor!=0)
      {
      this->Monitor->Register(this);
      }
    }
}

// ----------------------------------------------------------------------------
void vidSceneObserver::Execute(vtkObject *vtkNotUsed(caller),
                               unsigned long vtkNotUsed(event),
                               void *vtkNotUsed(calldata))
{
  cout << "scene observer: execute" << endl;
  
  bool savedRenderFlag;
  
  // Prevent double rendering if processEvent catch a user event that
  // need to re-render the render window.
  if(this->MapView!=0)
    {
    cout << "scene observer: mapview, set render flag to off (current is" << savedRenderFlag << ")" << endl;
    savedRenderFlag=this->MapView->GetRenderFlag();
    this->MapView->SetRenderFlag(false);
    }
  else
    {
    cout << "scene observer: no mapview" << endl;
    }
  
  cout << "qt processEvents()" << endl;
  QApplication::processEvents(); // keep the GUI responsive during animation.
  cout << "qt processEvents() done" << endl;
  
  if(this->MapView!=0)
    {
    cout << "scene observer: restore render flag to " << savedRenderFlag << ")" << endl;
    this->MapView->SetRenderFlag(savedRenderFlag);
    }
  
  if(this->RenderWindow!=0)
    {
    cout << " scene observer render: wait" << endl;
    this->Monitor->WaitForZero(); // blocking, non-busy wait
    cout << " scene observer render: done waitint" << endl;
    this->RenderWindow->Render();
    }
}
