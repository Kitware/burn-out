/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidTrackRepresentation.h"
#include "vtkObjectFactory.h"
#include "vidTrack.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkProperty.h"

vtkCxxRevisionMacro(vidTrackRepresentation, "$Revision$");
vtkStandardNewMacro(vidTrackRepresentation);
vtkCxxSetObjectMacro(vidTrackRepresentation,Track,vidTrack);

 // yes, hardcoded
const double height=480.0;
const double z=1.5;

// ----------------------------------------------------------------------------
void vidTrackRepresentation::SetZHigh(double value)
{
  if(value!=this->ZHigh)
    {
    this->ZHigh=value;
    // should we call modify?
    }
}

// ----------------------------------------------------------------------------
double vidTrackRepresentation::GetZHigh() const
{
  return this->ZHigh;
}

// ----------------------------------------------------------------------------
void vidTrackRepresentation::UpdateGeometry(int frame)
{
  this->LastFrame=frame;
  
  // change point location
  if(this->Track!=0)
    {
    vtkstd::vector<vtkSmartPointer<vidBBox> > *boxes=this->Track->GetBoxes();
    
//    this->In=true; //this->LastFrame>=this->Track->GetStartFrame() && this->LastFrame<=this->Track->GetLastFrame();
  
    this->In=this->LastFrame>=this->Track->GetStartFrame() && this->LastFrame<=this->Track->GetLastFrame() &&
      this->LastFrame<boxes->size();
    
    cout << "Track is ";
    if(this->In)
      {
      cout << "in." << endl;
      vidBBox *b=(*boxes)[this->LastFrame]; // -this->Track->GetStartFrame()];
      vtkPoints *positions=this->Quad->GetPoints();
//      positions->SetPoint(0,b->GetLeft(),b->GetLower(),z);
//      positions->SetPoint(1,b->GetRight(),b->GetLower(),z);
//      positions->SetPoint(2,b->GetRight(),b->GetUpper(),z);
//      positions->SetPoint(3,b->GetLeft(),b->GetUpper(),z);
      
      positions->SetPoint(0,b->GetLeft(),height-b->GetLower(),z+this->ZHigh);
      positions->SetPoint(1,b->GetRight(),height-b->GetLower(),z+this->ZHigh);
      positions->SetPoint(2,b->GetRight(),height-b->GetUpper(),z+this->ZHigh);
      positions->SetPoint(3,b->GetLeft(),height-b->GetUpper(),z+this->ZHigh);
     
      
      
      positions->Modified();
      this->Quad->Modified();
      this->Actor->SetVisibility(this->TrackVisibility && this->BoxVisibility);
      cout << "actor setvisibility=" << (this->TrackVisibility && this->BoxVisibility) << endl;
      cout << "actor getvisibility=" << this->Actor->GetVisibility() << endl;
      vtkProperty *p=this->Actor->GetProperty();
      p->SetColor(this->Color);
      }
    else
      {
      cout << "out."<< endl;
      this->Actor->SetVisibility(0);
      }
    }
  else
    {
    cout << "no track attached to representation." << endl;
    }
}
  
// ----------------------------------------------------------------------------
void vidTrackRepresentation::UpdateActorVisibility()
{
  this->Actor->SetVisibility(this->In && this->TrackVisibility && this->BoxVisibility);
}

// ----------------------------------------------------------------------------
int vidTrackRepresentation::GetLastFrame()
{
  return this->LastFrame;
}

// ----------------------------------------------------------------------------
vidTrackRepresentation::vidTrackRepresentation()
{
  this->ZHigh=500;
  this->TrackVisibility=false;
  this->BoxVisibility=false;
  this->In=false;
  this->Color[0]=1.0;
  this->Color[1]=1.0;
  this->Color[2]=1.0;
  
  this->Track=0;
  this->LastFrame=0;
  this->Actor=vtkActor::New();
  this->Mapper=vtkPolyDataMapper::New();
  this->Quad=vtkPolyData::New();
  
  this->Actor->SetMapper(this->Mapper);
  vtkProperty *p=this->Actor->GetProperty();
  p->SetLighting(false);
  
  this->Mapper->SetInput(this->Quad);
  
  
  
  vtkPoints *positions=vtkPoints::New();
  positions->SetNumberOfPoints(4);
  
  // some initial locations.
  positions->SetPoint(0,0.0,0.0,z+this->ZHigh);
  positions->SetPoint(1,1.0,0.0,z+this->ZHigh);
  positions->SetPoint(2,1.0,1.0,z+this->ZHigh);
  positions->SetPoint(3,0.0,1.0,z+this->ZHigh);
  
  this->Quad->SetPoints(positions);
  
  // build the quad cell
  vtkCellArray *quad=vtkCellArray::New();
  vtkIdList *topology=vtkIdList::New();
  topology->SetNumberOfIds(5);
  topology->SetId(0,0);
  topology->SetId(1,1);
  topology->SetId(2,2);
  topology->SetId(3,3);
  topology->SetId(4,0);
  topology->Modified();
  quad->InsertNextCell(topology);
  topology->Delete();
  this->Quad->SetLines(quad);
  quad->Delete();
  
}
  
// ----------------------------------------------------------------------------
void vidTrackRepresentation::BuildTrace()
{
  this->TraceActor=vtkActor::New();
  this->TraceMapper=vtkPolyDataMapper::New();
  this->TraceLine=vtkPolyData::New();
  
  this->TraceActor->SetMapper(this->TraceMapper);
  vtkProperty *p=this->TraceActor->GetProperty();
  p->SetLighting(false);
  p->SetColor(this->Color);
  
  this->TraceMapper->SetInput(this->TraceLine);
  
  vtkstd::vector<vtkSmartPointer<vidBBox> > *boxes=this->Track->GetBoxes();
  size_t c=boxes->size();
  size_t i;
  
  vtkPoints *positions=vtkPoints::New();
  positions->SetNumberOfPoints(c);
  
  // some initial locations.
   i=0;
  while(i<c)
    {
    vidBBox *b=(*boxes)[i];
    double centerX=(b->GetLeft()+b->GetRight())*0.5;
    double centerY=(2.0*height-b->GetUpper()-b->GetLower())*0.5;
    positions->SetPoint(i,centerX,centerY,this->ZHigh+z);
    ++i;
    }
  
  this->TraceLine->SetPoints(positions);
  
  // build the quad cell
  vtkCellArray *line=vtkCellArray::New();
  vtkIdList *topology=vtkIdList::New();
  
  topology->SetNumberOfIds(c);
  
  i=0;
  while(i<c)
    {
    topology->SetId(i,i);
    ++i;
    }
  topology->Modified();
  line->InsertNextCell(topology);
  topology->Delete();
  this->TraceLine->SetLines(line);
  line->Delete();
  
}

// ----------------------------------------------------------------------------
vidTrackRepresentation::~vidTrackRepresentation()
{
  if(this->Track!=0)
    {
    this->Track->Delete();
    }
  
  this->Actor->Delete();
  this->Mapper->Delete();
  this->Quad->Delete();
}

// ----------------------------------------------------------------------------
void vidTrackRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
