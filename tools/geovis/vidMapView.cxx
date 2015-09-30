/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidMapView.h"
#include "vtkObjectFactory.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPlaneSource.h"
#include "vtkTexture.h"
#include "vtkRenderer.h"
#include "vtkProperty.h"
#include "vtkImageData.h"
#include "vidSceneObserver.h"
#include "vtkRenderWindow.h"
#include "vtkAxesActor.h"

vtkCxxRevisionMacro(vidMapView, "$Revision$");
vtkStandardNewMacro(vidMapView);

// ----------------------------------------------------------------------------
vidMapView::vidMapView()
{
  this->MapActor=vtkActor::New();
  this->MapMapper=vtkPolyDataMapper::New();
  this->MapSource=vtkPlaneSource::New();
  this->MapTexture=0;
  
  this->MapActor->SetMapper(this->MapMapper);
  this->MapActor->SetVisibility(1);
  this->MapMapper->SetInputConnection(this->MapSource->GetOutputPort());
  
  double delta=20.0;
  
  this->MapSource->SetOrigin(-5.0,0.0,5.0+delta);
  this->MapSource->SetPoint1(5.0+delta,0.0,5.0+delta);
  this->MapSource->SetPoint2(-5.0,0.0,-5.0);
  this->MapSource->SetResolution(1,1);

  this->Renderer->AddActor(this->MapActor);
  this->Renderer->ResetCamera();
  this->Renderer->SetBackground(0.1,0.0,0.0);
  this->Renderer->SetBackground2(0.0,0.6,1.0);
  
  // true by default in superclass which trigger a render each time you
  // move the mouse, don't want that...
  this->SetDisplayHoverText(false);
  
  this->Observer=vidSceneObserver::New();
  this->Observer->SetRenderWindow(this->RenderWindow);
  this->Observer->SetMapView(this);
  this->RenderFlag=true;
  
  vtkAxesActor *axes=vtkAxesActor::New();
  this->Renderer->AddActor(axes);
  axes->Delete();
  axes->SetVisibility(0);
}

// ----------------------------------------------------------------------------
vidMapView::~vidMapView()
{
  this->MapActor->Delete();
  this->MapMapper->Delete();
  this->MapSource->Delete();
  if(this->MapTexture!=0)
    {
    this->MapTexture->Delete();
    }
  
  this->Observer->Delete();
}

// ----------------------------------------------------------------------------
void vidMapView::SetMapTexture(vtkImageData *texture)
{
  if(texture!=this->MapTexture)
    {
    if(this->MapTexture!=0)
      {
      this->MapTexture->Delete();
      }
    this->MapTexture=texture;
    if(this->MapTexture!=0)
      {
      this->MapTexture->Register(this);
      }
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vidMapView::SetMonitor(vidMonitor *m)
{
  this->Observer->SetMonitor(m);
}

// ----------------------------------------------------------------------------
void vidMapView::Render()
{
  cout << "vidMapView::Render()" << endl;
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  
  if(this->RenderFlag)
    {
    cout << "vidMapView::Render() with render" << endl;
    this->RenderWindow->Render();
    }
  else
    {
    cout << "vidMapView::Render() without render" << endl;
    }
}

// ----------------------------------------------------------------------------
void vidMapView::Update()
{
  cout << "vidMapView::Update()" << endl;
  this->Superclass::Update();
  
  if(this->MapTexture!=0)
    {
    vtkTexture *t=this->MapActor->GetTexture();
    if(t==0)
      {
      t=vtkTexture::New();
      this->MapActor->SetTexture(t);
      t->SetInterpolate(1);
      t->Delete();
      }
    t->SetInput(this->MapTexture);
    
    int dims[3];
    double spacing[3];
    this->MapTexture->GetDimensions(dims);
    this->MapTexture->GetSpacing(spacing);
    cout << "update mapsource" << endl;
    cout << "dim=" <<dims[0]<<","<<dims[1]<<","<<dims[2]<< endl;
    cout << "spacing="<<spacing[0]<<","<<spacing[1]<<","<<spacing[2]<<endl;
    double scale=5.0;
    this->MapSource->SetOrigin(0.0,0.0,0.0);
    this->MapSource->SetPoint1(dims[0]*spacing[0]*scale,0.0,0.0);
    this->MapSource->SetPoint2(0.0,dims[1]*spacing[1]*scale,0.0);
    }
}

// ----------------------------------------------------------------------------
vidSceneObserver *vidMapView::GetObserver() const
{
  return this->Observer;
}

// ----------------------------------------------------------------------------
void vidMapView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
