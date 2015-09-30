/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidVideoController.h"
#include "vtkPixelBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include <assert.h>
#include "vidVideoReader.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkPlaneSource.h"
#include "vtkMultiThreader.h"
#include "vidMonitor.h"
#include "vtkCameraActor.h"
#include "vtkCamera.h"
#include "vidVideoMetaDataReader.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vidStep.h"
#include "vtkMath.h"
#include "vtkGeoSphereTransform.h"
#include "vtkBoundingBox.h"
#include "vtkProperty.h"
#include "vidQueryResultsReader.h"
#include "vtkPropAssembly.h"

vtkCxxRevisionMacro(vidVideoController, "$Revision$");
vtkStandardNewMacro(vidVideoController);

// void *(*vtkThreadFunctionType)(void *);


void *thread_main(void *threadInfo)
{
  cout << "threadInfo=" << threadInfo <<endl;
  
  vtkMultiThreader::ThreadInfo *info=static_cast<vtkMultiThreader::ThreadInfo *>(threadInfo);
  
  void *data=info->UserData;
  
  vidVideoController *self=static_cast<vidVideoController *>(data);
  cout << "data=" << data << " self=" << self <<endl;
  self->ReadFrame();
  self->ThreadDone();
  return 0;
}

// ----------------------------------------------------------------------------
// Description:
// Create the reader, PBO and TO.
void vidVideoController::Initialize(vtkRenderWindow *context)
{
  if(this->Context!=context)
    {
    this->Context=context;
    this->PBO->SetContext(context);
    this->TO->SetContext(context);
    }
  this->Initialized=true;
}

// ----------------------------------------------------------------------------
bool vidVideoController::IsInitialized() const
{
  return this->Initialized;
}

// ----------------------------------------------------------------------------
void vidVideoController::SetZHigh(double value)
{
  if(value!=this->ZHigh)
    {
    this->ZHigh=value;
    // should we call modify?
    }
}

// ----------------------------------------------------------------------------
double vidVideoController::GetZHigh() const
{
  return this->ZHigh;
}

// ----------------------------------------------------------------------------
// Description:
// The gut of this class. Coordinate Reader and TO updates.
void vidVideoController::SetFrame(int frameId)
{
  assert("pre: is_initialized" && this->IsInitialized());
  
  cout << "vidVideoController::SetFrame() frameId=" << frameId <<endl;
  
//  this->Reader->SetFrame(frameId);
//  this->Reader->Update();
  this->FrameId=frameId;
  
  cout << "vidVideoController::SetFrame() this->FrameId=" << this->FrameId <<endl;
  
  cout << "vidVideoController::SetFrame() this=" << static_cast<void *>(this) << endl;
  
  this->Monitor->Increase(); // call by main thread
  
  if(this->Thread>=0)
    {
    // Release VTK resource
    this->Threader->TerminateThread(this->Thread);
    }
  this->Thread=this->Threader->SpawnThread(thread_main,this);
  
  // We can update the geometry in the calling thread, it is a cheap operation.
  this->ReadMetaData();
  
  // We can update the tracks in the calling thread, it is a cheap operation.
  this->ReadTracks();
  
#if 0
  vtkImageData *im=this->Reader->GetOutput();
  vtkDataArray *scalars=im->GetPointData()->GetScalars();
  int dims[3];
  unsigned int udims[2];
  im->GetDimensions(dims);
  udims[0]=dims[0];
  udims[1]=dims[1];
  vtkIdType increments[2];
  increments[0]=0;
  increments[1]=0;
  this->PBO->Upload2D(scalars->GetDataType(),
                      scalars->GetVoidPointer(0),
                      udims,scalars->GetNumberOfComponents(),increments);
  
  // non-blocking call.
  this->TO->Create2D(dims[0],dims[1],scalars->GetNumberOfComponents(),
                     this->PBO,false);
#endif
  cout <<"this->Texture->Update()" << endl;
//  this->Texture->Update(); // done during texture->render()
//  this->Texture->Modified();
}
  
// ----------------------------------------------------------------------------
void vidVideoController::ReadFrame()
{
  cout << "vidVideoController::ReadFrame() this=" << static_cast<void *>(this) << endl;
  cout << "vidVideoController::ReadFrame() this->FrameId=" << this->FrameId <<endl;
  this->Reader->SetFrame(this->FrameId);
  this->Reader->Update();
}

// ----------------------------------------------------------------------------
void vidVideoController::ThreadDone()
{
  this->Monitor->Decrease(); // call by sub-thread.
}

// ----------------------------------------------------------------------------
// Description:
// Return the current frame.
int vidVideoController::GetFrame() const
{
  return this->Reader->GetFrame();
}

// ----------------------------------------------------------------------------
vtkTexture *vidVideoController::GetTexture() const
{
  return this->Texture;
}

// ----------------------------------------------------------------------------
vidVideoReader *vidVideoController::GetReader() const
{
  return this->Reader;
}

// ----------------------------------------------------------------------------
vidVideoMetaDataReader *vidVideoController::GetMetaDataReader() const
{
  return this->MetaDataReader;
}

// ----------------------------------------------------------------------------
vidQueryResultsReader *vidVideoController::GetTracksReader() const
{
  return this->TracksReader;
}

// ----------------------------------------------------------------------------
// Name of the metainfo file. Initial value is "".
const vtkstd::string *vidVideoController::GetMetaName() const
{
  assert("post: result_exists" && this->MetaName!=0);
  return this->MetaName;
}

// ----------------------------------------------------------------------------
void vidVideoController::SetMetaName(const vtkstd::string *name)
{
  assert("pre: name_exists" && name!=0);
  
  if(this->MetaName!=name)
    {
    if(*this->MetaName!=*name)
      {
      delete this->MetaName;
      this->MetaName=new vtkstd::string(*name);
      }
    }
  assert("post: is_set" && *GetMetaName()==*name);
}

// ----------------------------------------------------------------------------
void vidVideoController::SetRenderer(vtkRenderer *r)
{
  assert("pre: r_exists" && r!=0);
  assert("pre: not_init" && this->Renderer==0);
  
  this->Renderer=r;
  this->Renderer->Register(this);
  this->AddSceneElements();
}

// ----------------------------------------------------------------------------
void vidVideoController::SetIndex(size_t i)
{
  this->Index=i;
}

// ----------------------------------------------------------------------------
vtkRenderer *vidVideoController::GetRenderer() const
{
  return this->Renderer;
}

// ----------------------------------------------------------------------------
void vidVideoController::AddSceneElements()
{
  this->OpenBox=vtkPolyData::New();
  this->OpenBoxMapper=vtkPolyDataMapper::New();
  this->OpenBoxActor=vtkActor::New();
  this->OpenBoxActor->SetMapper(this->OpenBoxMapper);
  vtkProperty *openBoxActorProperty=this->OpenBoxActor->GetProperty();
  openBoxActorProperty->SetColor(0.4,0.0,0.4);
  
  this->VideoActor=vtkActor::New();
  this->VideoMapper=vtkPolyDataMapper::New();
  this->VideoPlaneSource=vtkPlaneSource::New();
  this->Quad=vtkPolyData::New();
  this->VideoActor->SetMapper(this->VideoMapper);
  vtkProperty *videoActorProperty=this->VideoActor->GetProperty();
  videoActorProperty->SetLighting(false);
#if 0
  this->VideoMapper->SetInputConnection(
    this->VideoPlaneSource->GetOutputPort());
  
  this->VideoPlaneSource->SetOrigin(this->Index*1000+0.0,this->Index*1000+0.0,1.0);
  this->VideoPlaneSource->SetPoint1(this->Index*1000+720.0,this->Index*1000+0.0,1.0);
  this->VideoPlaneSource->SetPoint2(this->Index*1000+0.0,this->Index*1000+480.0,1.0);
  this->VideoPlaneSource->SetResolution(1,1);
#else
  vtkPoints *positions=vtkPoints::New();
  positions->SetNumberOfPoints(4);
  double xMin=this->Index*1000;
  double xMax=this->Index*1000+720.0;
  double yMin=this->Index*1000;
  double yMax=this->Index*1000+480.0;
  double zLow=0.0;
  double z=zLow+this->ZHigh;
  positions->SetPoint(0,xMin,yMin,z);
  positions->SetPoint(1,xMax,yMin,z);
  positions->SetPoint(2,xMax,yMax,z);
  positions->SetPoint(3,xMin,yMax,z);
  positions->Modified();
  this->Quad->SetPoints(positions);
  positions->Delete();
  vtkCellArray *quad=vtkCellArray::New();
  vtkIdList *topology=vtkIdList::New();
  topology->SetNumberOfIds(4);
  topology->SetId(0,0);
  topology->SetId(1,1);
  topology->SetId(2,2);
  topology->SetId(3,3);
  topology->Modified();
  quad->InsertNextCell(topology);
  topology->Delete();
  
  // normals will have to be recomputed when position change.
  double normal[3];
  normal[0]=0.0;
  normal[1]=0.0;
  normal[2]=1.0;
  vtkDoubleArray *normals=vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(4);
  vtkIdType i=0;
  while(i<4)
    {
    normals->SetTupleValue(i,normal);
    ++i;
    }
  normals->Modified();
  vtkDoubleArray *texCoords=vtkDoubleArray::New();
  texCoords->SetNumberOfComponents(2);
  texCoords->SetNumberOfTuples(4);
  
  double texCoord[2];
  texCoord[0]=0.0;
  texCoord[1]=0.0;
  texCoords->SetTupleValue(0,texCoord);
  texCoord[0]=1.0;
  texCoords->SetTupleValue(1,texCoord);
  texCoord[1]=1.0;
  texCoords->SetTupleValue(2,texCoord);
  texCoord[0]=0.0;
  texCoords->SetTupleValue(3,texCoord);
  texCoords->Modified();
  this->Quad->SetPolys(quad);
  this->Quad->GetPointData()->SetNormals(normals);
  this->Quad->GetPointData()->SetTCoords(texCoords);
  normals->Delete();
  texCoords->Delete();
  quad->Delete();
  this->VideoMapper->SetInput(this->Quad);
#endif
  this->Renderer->AddActor(this->VideoActor);
  this->VideoActor->SetTexture(this->Texture);
  
  // Open box.
  
  vtkPoints *bpositions=vtkPoints::New();
  bpositions->SetNumberOfPoints(4*4);
  
  // -x side
  bpositions->SetPoint(0,xMin,yMin,zLow);
  bpositions->SetPoint(1,xMin,yMin,z);
  bpositions->SetPoint(2,xMin,yMax,z);
  bpositions->SetPoint(3,xMin,yMax,zLow);
  
  // +x side
  bpositions->SetPoint(4,xMax,yMax,zLow);
  bpositions->SetPoint(5,xMax,yMax,z);
  bpositions->SetPoint(6,xMax,yMin,z);
  bpositions->SetPoint(7,xMax,yMin,zLow);
  
  // -y side
  bpositions->SetPoint(8,xMax,yMin,zLow);
  bpositions->SetPoint(9,xMax,yMin,z);
  bpositions->SetPoint(10,xMin,yMin,z);
  bpositions->SetPoint(11,xMin,yMin,zLow);
  
  // +y side
  bpositions->SetPoint(12,xMin,yMax,zLow);
  bpositions->SetPoint(13,xMin,yMax,z);
  bpositions->SetPoint(14,xMax,yMax,z);
  bpositions->SetPoint(15,xMax,yMax,zLow);
  
  bpositions->Modified();
  this->OpenBox->SetPoints(bpositions);
  bpositions->Delete();
  vtkCellArray *boxSides=vtkCellArray::New();
  vtkIdList *btopology=vtkIdList::New();
  btopology->SetNumberOfIds(4);
  btopology->SetId(0,0);
  btopology->SetId(1,1);
  btopology->SetId(2,2);
  btopology->SetId(3,3);
  btopology->Modified();
  boxSides->InsertNextCell(btopology);
  btopology->SetId(0,4);
  btopology->SetId(1,5);
  btopology->SetId(2,6);
  btopology->SetId(3,7);
  btopology->Modified();
  boxSides->InsertNextCell(btopology);
  btopology->SetId(0,8);
  btopology->SetId(1,9);
  btopology->SetId(2,10);
  btopology->SetId(3,11);
  btopology->Modified();
  boxSides->InsertNextCell(btopology);
  btopology->SetId(0,12);
  btopology->SetId(1,13);
  btopology->SetId(2,14);
  btopology->SetId(3,15);
  btopology->Modified();
  boxSides->InsertNextCell(btopology);
  btopology->Delete();
  
  // normals will have to be recomputed when position change.
  normals=vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(4*4);
  normal[0]=-1.0;
  normal[1]=0.0;
  normal[2]=0.0;
  i=0;
  while(i<4)
    {
    normals->SetTupleValue(i,normal);
    ++i;
    }
  normal[0]=1.0;
  while(i<8)
    {
    normals->SetTupleValue(i,normal);
    ++i;
    }
  normal[0]=0.0;
  normal[1]=-1.0;
  while(i<12)
    {
    normals->SetTupleValue(i,normal);
    ++i;
    }
  normal[1]=1.0;
  while(i<16)
    {
    normals->SetTupleValue(i,normal);
    ++i;
    }
  
  normals->Modified();
  
  this->OpenBox->SetPolys(boxSides);
  this->OpenBox->GetPointData()->SetNormals(normals);
  normals->Delete();
  boxSides->Delete();
  this->OpenBoxMapper->SetInput(this->OpenBox);
  this->Renderer->AddActor(this->OpenBoxActor);
  
  // end of openbox
  
  
  
  this->ViewActor=vtkCameraActor::New();
  this->Renderer->AddActor(this->ViewActor);
  this->Camera=vtkCamera::New();
  double position[3];
  double focalPoint[3];
  
  position[0]=this->Index*1000+720/2.0;
  position[1]=this->Index*1000+480/2.0;
  position[2]=1000.0;
  
  focalPoint[0]=this->Index*1000+720/2.0;
  focalPoint[1]=this->Index*1000+480/2.0;
  focalPoint[2]=1.0;
  
  this->Camera->SetPosition(position);
  this->Camera->SetFocalPoint(focalPoint);
  double upVector[3];
  
  double p0[3];
  double p3[3];
  positions->GetPoint(0,p0);
  positions->GetPoint(3,p3);
  
  upVector[0]=p3[0]-p0[0];
  upVector[1]=p3[1]-p0[1];
  upVector[2]=p3[2]-p0[2];
  
  this->Camera->SetViewUp(upVector);
//  this->Camera->SetViewUp(0.0,1.0,0.0);
  double angle=13.5;
  this->Camera->SetViewAngle(angle*2.0);
  // initial clip=(0.1,1000). near>0, far>near);
//  this->Camera->SetClippingRange(this->ClippingRange);
  double clippingRange[2];
  clippingRange[0]=0.1;
  clippingRange[1]=clippingRange[0]+this->Camera->GetDistance();
  this->Camera->SetClippingRange(clippingRange);
  
  this->ViewActor->SetCamera(this->Camera);
  this->ViewActor->SetWidthByHeightRatio(720.0/480.0);
  this->ViewActor->SetUseBounds(true);
  this->ViewActor->SetVisibility(1);
  vtkProperty *p=this->ViewActor->GetProperty();
  p->SetColor(1.0,0.0,0.0);
  p->SetLighting(false);
  
  
//  this->ViewActor->SetVisibility(0);
//  this->VideoActor->SetVisibility(0);
  
  this->TracksActor=vtkPropAssembly::New();
  this->Renderer->AddActor(this->TracksActor);
  
//  this->ConeActor=vtkActor::New();
//  this->ConeSource=vtkConeSource::New();
//  this->ConeMapper=vtkPolyDataMapper::New();
//  this->ConeActor->SetMapper(this->ConeMapper);
//  this->ConeMapper->SetInputConnection(this->ConeSource->GetOutputPort());
}

// ----------------------------------------------------------------------------
void vidVideoController::RemoveSceneElements()
{
  this->Renderer->RemoveActor(this->VideoActor);
  this->VideoActor->Delete();
  
  this->Renderer->RemoveActor(this->OpenBoxActor);
  this->OpenBoxActor->Delete();
  this->OpenBoxMapper->Delete();
  this->OpenBox->Delete();
  
  this->VideoPlaneSource->Delete();
  this->VideoMapper->Delete();
  this->Renderer->RemoveActor(this->ViewActor);
  this->ViewActor->Delete();
  this->Camera->Delete();
  this->Quad->Delete();
  this->Renderer->RemoveActor(this->TracksActor);
  this->TracksActor->Delete();
  
//  this->ConeActor->Delete();
//  this->ConeMapper->Delete();
//  this->ConeSource->Delete();
}

// ----------------------------------------------------------------------------
void vidVideoController::ReadMetaData()
{
  this->MetaDataReader->SetFrame(this->FrameId);
  this->MetaDataReader->ReadFrame();
  if(this->MetaDataReader->LastFrameValid())
    {
    vidStep *s=this->MetaDataReader->GetLastFrame();
    s->Print(cout);
    
    vtkPoints *p=this->Quad->GetPoints();
    double geo[3];
    double *xyz;
    
    const double scale=1; //0.0001;
    
    geo[0]=s->GetFrameLowerLeftLongitude();
    geo[1]=s->GetFrameLowerLeftLatitude();
    geo[2]=0.0; // altitude, earth is flat :-)
    xyz=this->Transform->TransformDoublePoint(geo);
    
    xyz[0]=xyz[0]*scale;
    xyz[1]=xyz[1]*scale;
    xyz[2]=xyz[2]*scale;
    cout << "xyz0=(" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" <<endl;
    p->SetPoint(0,xyz);
    
    geo[0]=s->GetFrameLowerRightLongitude();
    geo[1]=s->GetFrameLowerRightLatitude();
    xyz=this->Transform->TransformDoublePoint(geo);
    xyz[0]=xyz[0]*scale;
    xyz[1]=xyz[1]*scale;
    xyz[2]=xyz[2]*scale;
    p->SetPoint(1,xyz);
    cout << "xyz1=(" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" <<endl;
    geo[0]=s->GetFrameUpperRightLongitude();
    geo[1]=s->GetFrameUpperRightLatitude();
    xyz=this->Transform->TransformDoublePoint(geo);
    xyz[0]=xyz[0]*scale;
    xyz[1]=xyz[1]*scale;
    xyz[2]=xyz[2]*scale;
    p->SetPoint(2,xyz);
    cout << "xyz2=(" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" <<endl;
    geo[0]=s->GetFrameUpperLeftLongitude();
    geo[1]=s->GetFrameUpperLeftLatitude();
    xyz=this->Transform->TransformDoublePoint(geo);
    xyz[0]=xyz[0]*scale;
    xyz[1]=xyz[1]*scale;
    xyz[2]=xyz[2]*scale;
    p->SetPoint(3,xyz);
    cout << "xyz3=(" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" <<endl;
    p->Modified();
    
    p->Print(cout);
    
    // Recompute face normal from 3 first points.
    double n1[3];
    double n2[3];
    double n3[3];
    
    double p0[3];
    double p1[3];
    double p2[3];
    double p3[3];
    
    p->GetPoint(0,p0);
    p->GetPoint(1,p1);
    p->GetPoint(2,p2);
    p->GetPoint(3,p3);
    
    cout << "p0=(" << p0[0] << ", " << p0[1] << ", " << p0[2] << ")" <<endl;
    cout << "p1=(" << p1[0] << ", " << p1[1] << ", " << p1[2] <<  ")" << endl;
    cout << "p2=(" << p2[0] << ", " << p2[1] << ", " << p2[2] << ")"  << endl;
    cout << "p3=(" << p3[0] << ", " << p3[1] << ", " << p3[2] <<  ")" << endl;
    
    // compute geometric video ratio
    double width=sqrt(vtkMath::Distance2BetweenPoints(p0,p1));
    double height=sqrt(vtkMath::Distance2BetweenPoints(p0,p3));
    
    cout << "width=" << width << endl;
    cout << "height=" << height << endl;
    cout << "width/height=" << (width/height) << endl;
    
    vtkIdType i;
    i=0;
    while(i<3)
      {
      n1[i]=p1[i]-p0[i];
      n2[i]=p2[i]-p0[i];
      ++i;
      }
    vtkMath::Cross(n1,n2,n3);
    double n=vtkMath::Normalize(n3);
    if(n!=0.0)
      {
      vtkDoubleArray *normals=
        static_cast<vtkDoubleArray *>(
          this->Quad->GetPointData()->GetNormals());
      i=0;
      while(i<4)
        {
        normals->SetTupleValue(i,n3);
        ++i;
        }
      normals->Modified();
      }
    
    this->Quad->Modified();
    
    // view point
    geo[0]=s->GetSensorLongitude();
    geo[1]=s->GetSensorLatitude();
    geo[2]=s->GetSensorAltitude(); // altitude, earth is flat :-)
    xyz=this->Transform->TransformDoublePoint(geo);
    
    xyz[0]=xyz[0]*scale;
    xyz[1]=xyz[1]*scale;
    xyz[2]=xyz[2]*scale;
    
    this->Camera->SetPosition(xyz);
    
    cout << "Camera position xyz=" << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
    
    p->ComputeBounds();
    double bounds[6];
    p->GetBounds(bounds);
    vtkBoundingBox box;
    box.SetBounds(bounds);
    double focalPoint[3];
    box.GetCenter(focalPoint);
    this->Camera->SetFocalPoint(focalPoint);
    
    double upVector[3];
    
    upVector[0]=p3[0]-p0[0];
    upVector[1]=p3[1]-p0[1];
    upVector[2]=p3[2]-p0[2];
    
    this->Camera->SetViewUp(upVector);
    this->Camera->OrthogonalizeViewUp();
    double angle;
    
    double upperMiddle[3];
    
    upperMiddle[0]=(p3[0]+p2[0])/2.0;
    upperMiddle[1]=(p3[1]+p2[1])/2.0;
    upperMiddle[2]=(p3[2]+p2[2])/2.0;
    
    double distance=sqrt(vtkMath::Distance2BetweenPoints(
                           this->Camera->GetFocalPoint(),upperMiddle));
    
    angle=2.0*atan2(distance,this->Camera->GetDistance());
    angle=vtkMath::DegreesFromRadians(angle);
    
    this->Camera->SetViewAngle(angle);
    
    double clippingRange[2];
    clippingRange[0]=0.001;
    clippingRange[1]=clippingRange[0]+this->Camera->GetDistance();
    this->Camera->SetClippingRange(clippingRange);
    this->Camera->Modified();
    vtkProperty *property=this->ViewActor->GetProperty();
    if(s->GetValid())
      {
      property->SetColor(0.0,0.4,0.0);
      }
    else
      {
      property->SetColor(1.0,0.0,0.0);
      }
    
    this->ViewActor->Modified();
    }
}

// ----------------------------------------------------------------------------
void vidVideoController::ReadTracks()
{
  cout << "ReadTracks" << endl;
  vtkstd::vector<vtkSmartPointer<vidTrack > > *tracks;
   size_t c;
   size_t i;
  
  if(this->TracksTime<this->TracksReader->GetMTime())
    {
    cout << "ReadTracks really" << endl;
    bool status=this->TracksReader->ReadXMLFile();
    this->TracksTime.Modified();
    if(status)
      {
      this->TracksActor->GetParts()->RemoveAllItems();
      this->TrackRepresentations->clear();
      this->TracksProperties->clear();
      
//      this->TracksActor->Modified();
      tracks=this->TracksReader->GetTracks();
      
      c=tracks->size();
      i=0;
      vtkMath::RandomSeed(1664); // random beer
      while(i<c)
        {
        vidTrackRepresentation *r=vidTrackRepresentation::New();
        this->TrackRepresentations->push_back(r);
        r->SetTrack((*tracks)[i]);
        r->SetZHigh(this->GetZHigh());
        this->TracksActor->AddPart(r->GetActor());
        double red=vtkMath::Random(0.0,1.0);
        double green=vtkMath::Random(0.0,1.0);
        double blue=vtkMath::Random(0.0,1.0);
        
        vidTrackVisualProperty *p=vidTrackVisualProperty::New();
        this->TracksProperties->push_back(p);
        p->SetColor(red,green,blue);
        p->SetTraceVisibility(this->TraceVisibility);
        p->SetBoxVisibility(true);
        p->SetVisibility(true);
        
        r->SetTrackVisibility(p->GetVisibility());
        r->SetBoxVisibility(p->GetBoxVisibility());
        
        r->SetColor(red,green,blue);
        r->BuildTrace();
        this->TracksActor->AddPart(r->GetTraceActor());
        r->GetTraceActor()->SetVisibility(this->TraceVisibility);
        ++i;
        }
      }
    
    }
  
  // Update positions
  tracks=this->TracksReader->GetTracks();
  c=tracks->size();
  i=0;
  while(i<c)
    {
    vidTrackRepresentation *r=(*this->TrackRepresentations)[i];
    cout << "track " << i << " ";
    r->UpdateGeometry(this->FrameId);
    ++i;
    }
  this->TracksActor->Modified();
}

// ----------------------------------------------------------------------------
void vidVideoController::SetTraceVisibility(bool value)
{
  this->TraceVisibility=value;
  size_t c=this->TrackRepresentations->size();
  size_t i=0;
  while(i<c)
    {
    vidTrackRepresentation *r=(*this->TrackRepresentations)[i];
    vtkActor *a=r->GetTraceActor();
    if(a!=0)
      {
      a->SetVisibility(this->TraceVisibility);
      }
    vidTrackVisualProperty *p=(*this->TracksProperties)[i];
    p->SetTraceVisibility(this->TraceVisibility);
    ++i;
    }
}
// ----------------------------------------------------------------------------
void vidVideoController::slotUpdateProperty(int p)
{
  vidTrackRepresentation *r=(*this->TrackRepresentations)[p];
  vidTrackVisualProperty *prop=(*this->TracksProperties)[p];
  r->SetColor(prop->GetColor());
  r->GetTraceActor()->SetVisibility(prop->GetVisibility() &&
                                    prop->GetTraceVisibility());
  r->SetTrackVisibility(prop->GetVisibility());
  r->SetBoxVisibility(prop->GetBoxVisibility());
  r->UpdateActorVisibility();
  
  emit signalModelChanged();
}

// ----------------------------------------------------------------------------
void vidVideoController::slotUpdateProperties()
{
  vtkstd::vector<vtkSmartPointer<vidTrack > > *tracks;
  size_t c;
  size_t i;
   
  tracks=this->TracksReader->GetTracks();
      
  c=tracks->size();
  i=0;
  while(i<c)
    {
    vidTrackRepresentation *r=(*this->TrackRepresentations)[i];
    vidTrackVisualProperty *prop=(*this->TracksProperties)[i];
    r->SetColor(prop->GetColor());
    r->GetTraceActor()->SetVisibility(prop->GetVisibility() &&
                                      prop->GetTraceVisibility());
    
    r->SetTrackVisibility(prop->GetVisibility());
    r->SetBoxVisibility(prop->GetBoxVisibility());
    r->UpdateActorVisibility();
    ++i;
    }
  emit signalModelChanged();
}

// ----------------------------------------------------------------------------
 vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *
 vidVideoController::GetTracksProperties() const
{
  return this->TracksProperties;
}

// ----------------------------------------------------------------------------
// Description:
// Default constructor: call Initialize right after.
vidVideoController::vidVideoController()
{
  this->ZHigh=500; // hack
  this->Reader=vidVideoReader::New();
  this->PBO=vtkPixelBufferObject::New();
  this->TO=vtkTextureObject::New();
  this->Context=0;
  this->Initialized=false;
  this->Texture=vtkTexture::New();
  this->Texture->SetRepeat(0);
  this->Texture->SetEdgeClamp(1);
  this->Texture->SetInputConnection(this->Reader->GetOutputPort());
  
  vtkTransform *t=vtkTransform::New();
  t->Translate(0.0,1.0,0.0);
  t->Scale(1.0,-1.0,1.0); // source is video, flip the image
  this->Texture->SetTransform(t);
  t->Delete();
  
  this->MetaName=new vtkstd::string("");
  this->Renderer=0;
  this->VideoActor=0;
  this->VideoMapper=0;
  this->VideoPlaneSource=0;
  this->Index=0;
  
  this->OpenBox=0;
  this->OpenBoxMapper=0;
  this->OpenBoxActor=0;
  
  this->Threader=vtkMultiThreader::New();
  this->Monitor=0;
  this->Thread=-1;
  this->ViewActor=0;
  
  this->MetaDataReader=vidVideoMetaDataReader::New();
  this->Transform=vtkGeoSphereTransform::New();
  
  this->TracksReader=vidQueryResultsReader::New();
  this->TrackRepresentations=new vtkstd::vector<vtkSmartPointer<vidTrackRepresentation > >;
  this->TraceVisibility=false;
  
  this->TracksProperties=new vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > >;
}

// ----------------------------------------------------------------------------
vidVideoController::~vidVideoController()
{
  this->Reader->Delete();
  this->MetaDataReader->Delete();
  this->PBO->Delete();
  this->TO->Delete();
  this->Transform->Delete();
  if(this->Context!=0)
    {
    this->Context->Delete();
    }
  this->Texture->Delete();
  delete this->MetaName;
  
  if(this->Renderer!=0)
    {
    this->RemoveSceneElements();
    this->Renderer->Delete();
    }
  if(this->Thread>=0)
    {
    // Release VTK resource
    this->Threader->TerminateThread(this->Thread);
    }
  this->Threader->Delete();
  if(this->Monitor!=0)
    {
    this->Monitor->Delete();
    }
  this->TracksReader->Delete();
  delete this->TrackRepresentations;
  delete this->TracksProperties;
}

// ----------------------------------------------------------------------------
void vidVideoController::SetMonitor(vidMonitor *m)
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
void vidVideoController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
