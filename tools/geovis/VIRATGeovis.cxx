/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "ui_VIRATGeovis.h"
#include "VIRATGeovis.h"

#include <vtkGeoAlignedImageRepresentation.h>
#include <vtkGeoAlignedImageSource.h>
#include <vtkGeoGlobeSource.h>
#include <vtkGeoTerrain.h>
#include <vtkGeoView.h>
#include <vtkJPEGReader.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <QVTKWidget.h>
#include "vidMapView.h"
#include "vtkPNGReader.h"
#include "vtkGeoInteractorStyle.h"
#include "vtkGeoCamera.h"
#include "vidVideoController.h"
#include "vidVideoReader.h"
#include <cassert>
#include "vtkPNGWriter.h"
#include "vidSceneObserver.h"
#include "vtkAnimationScene.h"
#include "vidMonitor.h"
#include <QComboBox>
#include "vidVideoMetaDataReader.h"
#include "vidQueryResultsReader.h"
#include "DialogQueries.h"
#include <QFileDialog>
#include "vtkMinimalStandardRandomSequence.h"

// ----------------------------------------------------------------------------
class vidAnimationCueObserver : public vtkCommand
{
public:
  static vidAnimationCueObserver *New()
    {
      return new vidAnimationCueObserver;
    }
  
  virtual void Execute(vtkObject *vtkNotUsed(caller),
                       unsigned long vtkNotUsed(event),
                       void *calldata)
    { 
      vtkAnimationCue::AnimationCueInfo *info=
        static_cast<vtkAnimationCue::AnimationCueInfo *>(calldata);
  
  cout << "vidAnimationCueObserver::Execute() start=" << info->StartTime << " anim=" << info->AnimationTime << " diff=" << (info->AnimationTime -info->StartTime)<< endl;
  
  if(this->VideoController!=0)
    {
    cout << "video controller exists" << endl;
//    this->VideoController->SetFrame(info->AnimationTime - 
//                                    info->StartTime);
    this->VideoController->SetFrame(info->AnimationTime);
    }
    }
  
  void SetVideoController(vidVideoController *c)
    {
      this->VideoController=c;
    }
  
protected:
  vidAnimationCueObserver()
    {
      this->VideoController=0;
    }
  ~vidAnimationCueObserver()
    {
    }
  
  vidVideoController *VideoController;
};


// ----------------------------------------------------------------------------
VIRATGeovis::VIRATGeovis() 
{
  this->Randomizer=vtkMinimalStandardRandomSequence::New();
  this->Randomizer->SetSeed(1664); // random beer
  
  this->ui = new Ui_VIRATGeovis;
  this->ui->setupUi(this);

  this->DiaQueries=0;
  
  this->Monitor=vidMonitor::New();
  
  this->GeoView = vtkSmartPointer<vtkGeoView>::New();
  this->TerrainSource = vtkSmartPointer<vtkGeoGlobeSource>::New();
  this->ImageSource = vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  this->MapView = vtkSmartPointer<vidMapView>::New();
  this->MapView->SetMonitor(this->Monitor);
  
  // Initialize interactor first and then the renderwindow
  // This order is required.
  this->GeoView->SetInteractor(this->ui->GeoView->GetInteractor());
  this->ui->GeoView->SetRenderWindow(this->GeoView->GetRenderWindow());

  // Add default terrain
  this->TerrainSource->Initialize();
  vtkSmartPointer<vtkGeoTerrain> terrain =
    vtkSmartPointer<vtkGeoTerrain>::New();
  terrain->SetSource(this->TerrainSource);
  this->GeoView->SetTerrain(terrain);
  this->GeoView->GetRenderer()->SetBackground(0.2, 0.2, 0.2);
  this->GeoView->GetRenderer()->SetBackground2(0.0, 0.0, 0.0);
  this->GeoView->GetRenderer()->GradientBackgroundOn();
  
  // Set up action signals and slots
  connect(this->ui->actionOpen_Query_Results,SIGNAL(triggered()), this, SLOT(slotOpenQueryResults()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
  connect(this->ui->actionPlay, SIGNAL(triggered()), this, SLOT(slotPlay()));
  connect(this->ui->actionPause, SIGNAL(triggered()), this, SLOT(slotPause()));
  connect(this->ui->actionSnap, SIGNAL(triggered()), this, SLOT(slotSnap()));
  connect(this->ui->actionCheckTraces, SIGNAL(triggered()), this, SLOT(slotCheckTraces()));
  connect(this->ui->actionDialogQueries, SIGNAL(triggered()), this, SLOT(slotDialogQueries()));
  this->MapView->SetInteractor(this->ui->map3d->GetInteractor());
  this->ui->map3d->SetRenderWindow(this->MapView->GetRenderWindow());
  
  this->AnimationScene=vtkAnimationScene::New();
  this->AnimationScene->AddObserver(vtkCommand::AnimationCueTickEvent,
                                    this->MapView->GetObserver());
  
  this->ComboBox=new QComboBox();
  this->ui->toolBar->addWidget(this->ComboBox);
  
  connect(this->ComboBox, SIGNAL(currentIndexChanged(int)),this,SLOT(slotActiveVideo(int)));
  
  this->Videos=new std::vector<vtkSmartPointer<vidVideoController > >;
  this->ActiveVideo=-1;
  
  this->UpdateComboBox();
  this->CheckTraces=false;
}

// ----------------------------------------------------------------------------
VIRATGeovis::~VIRATGeovis()
{
  this->TerrainSource->ShutDown();
  this->ImageSource->ShutDown();
  
  this->AnimationScene->Delete();
  delete this->ComboBox;
  
  delete this->Videos;
  this->Monitor->Delete();
  
  if(this->DiaQueries!=0)
    {
    delete this->DiaQueries;
    }
}

// ----------------------------------------------------------------------------
void VIRATGeovis::UpdateComboBox()
{
  this->ComboBox->clear();
  size_t i=0;
  size_t c=this->Videos->size();
  while(i<c)
    {
    const vtkstd::string *name=(*this->Videos)[i]->GetReader()->GetFileName();
    this->ComboBox->addItem(QString(name->c_str()));
    ++i;
    }
}

// ----------------------------------------------------------------------------
void VIRATGeovis::loadImage(const char* imageFile)
{
  vtkSmartPointer<vtkJPEGReader> reader =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(imageFile);
  reader->Update();

  // Add image representation
  this->ImageSource->SetImage(reader->GetOutput());
  this->ImageSource->Initialize();
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  rep->SetSource(this->ImageSource);
  this->GeoView->AddRepresentation(rep);
  this->GeoView->Update();
}

// ----------------------------------------------------------------------------
void VIRATGeovis::loadMapImage(const char* imageFile)
{
  vtkSmartPointer<vtkPNGReader> reader =
    vtkSmartPointer<vtkPNGReader>::New();
  reader->SetFileName(imageFile);
  reader->Update();
  
  this->MapView->SetMapTexture(reader->GetOutput());
  this->MapView->Update();
}

// ----------------------------------------------------------------------------
void VIRATGeovis::AddVideoAndMetaFileName(const vtkstd::string *fileName,
                                          const vtkstd::string *metaFileName)
{
  assert("pre: fileName_exists" && fileName!=0);
//  if(this->VideoFileName!=fileName && (*this->VideoFileName)!=(*fileName))
//    {
  
  vidVideoController *c=vidVideoController::New();
  c->SetMonitor(this->Monitor);
  c->Initialize(this->MapView->GetRenderWindow());
  c->GetReader()->SetFileName(fileName);
  c->GetMetaDataReader()->SetFileName(metaFileName);
  c->SetIndex(this->Videos->size()); // most be called before SetRenderer()
  c->SetRenderer(this->MapView->GetRenderer());
  this->Videos->push_back(c);
  c->Delete();
  
#if 0
    // for debugging video reader.
    vtkPNGWriter *writer=vtkPNGWriter::New();
    writer->SetInputConnection(
      this->VideoController->GetReader()->GetOutputPort());
    this->VideoController->GetReader()->SetFrame(120);
    this->VideoController->GetReader()->Update();
    
    writer->SetFileName("image_from_video.png");
    writer->Write();
    writer->Delete();
#endif
//    this->AnimationScene->RemoveAllCues();
    vtkAnimationCue *cue=vtkAnimationCue::New();
    cue->SetStartTime(50.0); // 50.0
    cue->SetEndTime(100.0); // 100.0
    this->AnimationScene->AddCue(cue);
    cue->Delete();
    
    vidAnimationCueObserver *o=vidAnimationCueObserver::New();
    o->SetVideoController(c);
    cue->AddObserver(vtkCommand::AnimationCueTickEvent,o);
    o->Delete();
    
    this->AnimationScene->SetLoop(1);
    this->AnimationScene->SetFrameRate(1);
    this->AnimationScene->SetStartTime(0.0);
    this->AnimationScene->SetEndTime(1000.0); // 100.0  hardcoded
    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
//    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_REALTIME);
    
    this->UpdateComboBox();
//    }
}

// ----------------------------------------------------------------------------
void VIRATGeovis::AddVideoAndQueryResultFileName(
  const vtkstd::string *fileName,
  const vtkstd::string *queryResultFileName)
{
  assert("pre: fileName_exists" && fileName!=0);
  assert("pre: queryResultFileName_exists" && queryResultFileName!=0);
  
  vidVideoController *c=vidVideoController::New();
  c->SetMonitor(this->Monitor);
  c->Initialize(this->MapView->GetRenderWindow());
  c->GetReader()->SetFileName(fileName);
//  c->GetMetaDataReader()->SetFileName(metaFileName);
  c->GetTracksReader()->SetFileName(queryResultFileName);
  
  c->SetIndex(this->Videos->size()); // most be called before SetRenderer()
  c->SetRenderer(this->MapView->GetRenderer());
  this->Videos->push_back(c);
  c->Delete();
  
  connect(c,SIGNAL(signalModelChanged()),this,SLOT(slotUpdateView()));
  
#if 0
    // for debugging video reader.
    vtkPNGWriter *writer=vtkPNGWriter::New();
    writer->SetInputConnection(
      this->VideoController->GetReader()->GetOutputPort());
    this->VideoController->GetReader()->SetFrame(120);
    this->VideoController->GetReader()->Update();
    
    writer->SetFileName("image_from_video.png");
    writer->Write();
    writer->Delete();
#endif
//    this->AnimationScene->RemoveAllCues();
    vtkAnimationCue *cue=vtkAnimationCue::New();
    cue->SetStartTime(0.0);
    cue->SetEndTime(1000.0); // 100.0
    this->AnimationScene->AddCue(cue);
    cue->Delete();
    
    vidAnimationCueObserver *o=vidAnimationCueObserver::New();
    o->SetVideoController(c);
    cue->AddObserver(vtkCommand::AnimationCueTickEvent,o);
    o->Delete();
    
    this->AnimationScene->SetLoop(0);
    this->AnimationScene->SetFrameRate(1);
    this->AnimationScene->SetStartTime(0.0);
    this->AnimationScene->SetEndTime(1000.0); // 100.0
    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
//    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_REALTIME);
    
    this->UpdateComboBox();
//    }
}


// ----------------------------------------------------------------------------
void VIRATGeovis::AddVideoFileName(const vtkstd::string *fileName)
{
  assert("pre: fileName_exists" && fileName!=0);
//  if(this->VideoFileName!=fileName && (*this->VideoFileName)!=(*fileName))
//    {
  
  vidVideoController *c=vidVideoController::New();
  c->SetMonitor(this->Monitor);
  this->Randomizer->Next();
  c->SetZHigh(this->Randomizer->GetRangeValue(1.0,1000.0));
  c->Initialize(this->MapView->GetRenderWindow());
  c->GetReader()->SetFileName(fileName);
  c->SetIndex(this->Videos->size()); // most be called before SetRenderer()
  c->SetRenderer(this->MapView->GetRenderer());
  this->Videos->push_back(c);
  c->Delete();
  
#if 0
    // for debugging video reader.
    vtkPNGWriter *writer=vtkPNGWriter::New();
    writer->SetInputConnection(
      this->VideoController->GetReader()->GetOutputPort());
    this->VideoController->GetReader()->SetFrame(120);
    this->VideoController->GetReader()->Update();
    
    writer->SetFileName("image_from_video.png");
    writer->Write();
    writer->Delete();
#endif
//    this->AnimationScene->RemoveAllCues();
    vtkAnimationCue *cue=vtkAnimationCue::New();
    cue->SetStartTime(0.0); // 50.0
    cue->SetEndTime(1000.0); // 100.0
    this->AnimationScene->AddCue(cue);
    cue->Delete();
    
    vidAnimationCueObserver *o=vidAnimationCueObserver::New();
    o->SetVideoController(c);
    cue->AddObserver(vtkCommand::AnimationCueTickEvent,o);
    o->Delete();
    
    this->AnimationScene->SetLoop(1);
    this->AnimationScene->SetFrameRate(1);
    this->AnimationScene->SetStartTime(0.0);
    this->AnimationScene->SetEndTime(1000.0); //100.0
    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
//    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_REALTIME);
    
    this->UpdateComboBox();
//    }
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotOpenQueryResults()
{
  QString fileName=QFileDialog::getOpenFileName(this,tr("Open Query Results"),".",tr("Query Results files (*.xml)"));
  if(!fileName.isEmpty())
    {
//    this->LoadQueryResults(fileName);
    }
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotExit()
{
  this->TerrainSource->ShutDown();
  this->ImageSource->ShutDown();
  qApp->exit();
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotPlay()
{
  std::cerr << "play video\n";
  std::cout << "animation time=" <<this->AnimationScene->GetAnimationTime() << std::endl;
  this->AnimationScene->Play();
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotPause()
{
  std::cerr << "pause video\n";
  this->AnimationScene->Stop();
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotSnap()
{
  std::cerr << "snap it" << endl;
  vtkGeoCamera *c=this->GeoView->GetGeoInteractorStyle()->GetGeoCamera();
  
  // Kitware: 28 corporate drive, clifton park ny 12065
  // latitude decimal:  42.849941 degrees
  // longitude decimal: -73.75909 degrees
  // altitude: 89 meters.
  // mean earth radius=6371km
  c->SetLatitude(42.849941);
  c->SetLongitude(-73.75909);
  
  std::cerr << "distance=" << c->GetDistance() << endl;
   c->SetDistance(287181);
   c->SetTilt(20);
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotActiveVideo(int index)
{
  std::cerr << "active video from combo box=" << index << endl;
  this->ActiveVideo=index;
}
// ----------------------------------------------------------------------------
void VIRATGeovis::slotCheckTraces()
{
  std::cerr << "slotCheckTraces" << endl;
  this->CheckTraces=!this->CheckTraces;
  
  size_t i=0;
  size_t c=this->Videos->size();
  while(i<c)
    {
    (*this->Videos)[i]->SetTraceVisibility(this->CheckTraces);
    ++i;
    }
  this->MapView->Update();
  this->MapView->Render();
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotDialogQueries()
{
  std::cerr << "slotDialogQueries" << endl;
  
  size_t i=0;
  size_t c=this->Videos->size();
  
  if(this->DiaQueries==0)
    {
    this->DiaQueries=new DialogQueries(this);
    if(c==1)
      {
      connect(this->DiaQueries,SIGNAL(signalPropertyChanged(int)),(*this->Videos)[0],SLOT(slotUpdateProperty(int)));
      connect(this->DiaQueries,SIGNAL(signalAllPropertiesChanged()),(*this->Videos)[0],SLOT(slotUpdateProperties()));
      }
    }
  if(c==1) // single video mode.
    {
    vidQueryResultsReader *reader=(*this->Videos)[0]->GetTracksReader();
    vtkstd::vector<vtkSmartPointer<vidTrack > > *tracks;
    tracks=reader->GetTracks();
    
    vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *properties;
    properties=(*this->Videos)[0]->GetTracksProperties();
    this->DiaQueries->BuidList(tracks,properties);
    }
  
  std::cerr << "slotDialogQueries: show" << endl;
  this->DiaQueries->show();
  std::cerr << "slotDialogQueries: show done. raise" << endl;
  this->DiaQueries->raise();
  std::cerr << "slotDialogQueries: raise done. activateWindow" << endl;
  this->DiaQueries->activateWindow();
  std::cerr << "slotDialogQueries: activateWindow done." << endl;
}

// ----------------------------------------------------------------------------
void VIRATGeovis::slotUpdateView()
{
  this->MapView->Render();
}
