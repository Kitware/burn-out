/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidVideoReader.h"
#include "video/vidl_ffmpeg_frame_process.h"
#include "vtkImageImport.h"
#include "vtkImageFlip.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
// KWVision/vidtk/library/utilities/config_block.h

#include "vtkStreamingDemandDrivenPipeline.h" // for vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()
#include "vtkImageAppendComponents.h"

vtkStandardNewMacro(vidVideoReader);
vtkCxxRevisionMacro(vidVideoReader, "$Revision$");

// ----------------------------------------------------------------------------
vidVideoReader::vidVideoReader()
{
  this->FileName=new vtkstd::string;
  
  this->Opened=false;
  this->LastFrame=0;
  this->Frame=0;
  this->Decoder=0;
  
  this->Importer=0;
  this->Flipper=0;
  
  this->ImporterGreen=0;
  this->ImporterBlue=0;
  this->Components=0;
  
  // it's a source
  this->SetNumberOfInputPorts(0);
}

// ----------------------------------------------------------------------------
vidVideoReader::~vidVideoReader()
{
  delete this->FileName;
  if(this->Decoder!=0)
    {
    delete this->Decoder;
    }
  if(this->Importer!=0)
    {
    this->Importer->Delete();
//    this->Flipper->Delete();
    
    this->ImporterGreen->Delete();
    this->ImporterBlue->Delete();
    this->Components->Delete();
    }
}

// ----------------------------------------------------------------------------
void vidVideoReader::SetFileName(const vtkstd::string *fileName)
{
  assert("pre: fileName_exists" && fileName!=0);
  if(this->FileName!=fileName)
    {
    (*this->FileName)=(*fileName);
    this->Modified();
    this->Opened=false;
    }
}

// ----------------------------------------------------------------------------
const vtkstd::string *vidVideoReader::GetFileName() const
{
  return this->FileName;
}

// ----------------------------------------------------------------------------
int vidVideoReader::GetFrame() const
{
  return this->Frame;
}

// ----------------------------------------------------------------------------
void vidVideoReader::SetFrame(int frame)
{
  if(this->Frame!=frame)
    {
    this->Frame=frame;
    this->Modified();
//    cout << "vidVideoReader::SetFrame() frame=" << frame << this->GetMTime() << endl;
    }
}

// ----------------------------------------------------------------------------
int vidVideoReader::RequestInformation(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
//  cout << "vidVideoReader::RequestInformation frame=" << this->Frame << endl;
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // the video cannot be broken down into pieces.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),1);
 
  // set whole extent, spacing and origin
  
  // so we need to read the file here.
  int status=this->ReadFrame();
  if(status==0)
    {
    return status;
    }
  
//  cout << "RequestInfo: before image()" << endl;
  vil_image_view<vxl_byte> image=this->Decoder->image();
//  cout << "RequestInfo: after image()" << endl;
  
  int w=static_cast<int>(image.ni());
  int h=static_cast<int>(image.nj());
  
  int ext[6];
  ext[0]=0;
  ext[1]=w-1;
  ext[2]=0;
  ext[3]=h-1;
  ext[4]=0;
  ext[5]=0;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);
  double spacing[3];
  spacing[0]=1.0;
  spacing[1]=1.0;
  spacing[2]=1.0;
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);
  double origin[3];
  origin[0] = 0.0;
  origin[1] = 0.0;
  origin[2] = 0.0;
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  
  return this->Superclass::RequestInformation(request,inputVector,
                                              outputVector);
}

// ----------------------------------------------------------------------------
int vidVideoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  cout << "vidVideoReader::RequestData frame=" << this->Frame << endl;
  vtkInformation *outInfo=outputVector->GetInformationObject(0);
  vtkImageData *output=vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  cout << "RequestData: before image()" << endl;
  vil_image_view<vxl_byte> image=this->Decoder->image();
  cout << "RequestData: after image()" << endl;

  int w=static_cast<int>(image.ni());
  int h=static_cast<int>(image.nj());
  
  cout << "nplanes=" << image.nplanes() << endl;
  cout << "is_contiguous="<<image.is_contiguous()<< endl;
  cout << "size_bytes=" << image.size_bytes() << endl;
  cout << "istep="<<image.istep()<< endl;
  cout << "jstep="<<image.jstep()<< endl;
  cout << "planestep="<<image.planestep()<< endl;
#if 0
  if(this->Importer==0)
    {
    this->Importer=vtkImageImport::New();
    this->Flipper=vtkImageFlip::New();
    
    this->Importer->SetDataScalarType(VTK_UNSIGNED_CHAR);
    this->Importer->SetNumberOfScalarComponents(3);
    
    this->Flipper->SetInputConnection(this->Importer->GetOutputPort());
    }
  
//  cout << "w=" << w << "h=" << h << endl;
  
  this->Importer->SetImportVoidPointer(image.top_left_ptr());
  this->Importer->SetWholeExtent(0,w-1,0,h-1,0,0);
  this->Importer->SetDataExtentToWholeExtent();
  this->Importer->Modified();
  this->Importer->Update();
//  this->Flipper->SetFilteredAxis(1);
//  this->Flipper->Update();
  
//  output->ShallowCopy(this->Flipper->GetOutput());
  output->ShallowCopy(this->Importer->GetOutput());
#else
  if(this->Importer==0)
    {
    this->Importer=vtkImageImport::New();
    this->Importer->SetDataScalarType(VTK_UNSIGNED_CHAR);
    this->Importer->SetNumberOfScalarComponents(1);
    
    this->ImporterGreen=vtkImageImport::New();
    this->ImporterGreen->SetDataScalarType(VTK_UNSIGNED_CHAR);
    this->ImporterGreen->SetNumberOfScalarComponents(1);
    
    this->ImporterBlue=vtkImageImport::New();
    this->ImporterBlue->SetDataScalarType(VTK_UNSIGNED_CHAR);
    this->ImporterBlue->SetNumberOfScalarComponents(1);
    
    this->Components=vtkImageAppendComponents::New();
    this->Components->SetInputConnection(this->Importer->GetOutputPort());
    this->Components->AddInputConnection(this->ImporterGreen->GetOutputPort());
    this->Components->AddInputConnection(this->ImporterBlue->GetOutputPort());
    }
  
  this->Importer->SetImportVoidPointer(image.top_left_ptr());
  this->Importer->SetWholeExtent(0,w-1,0,h-1,0,0);
  this->Importer->SetDataExtentToWholeExtent();
  this->Importer->Modified();
  this->Importer->Update();
  
  this->ImporterGreen->SetImportVoidPointer(image.top_left_ptr()+image.planestep());
  this->ImporterGreen->SetWholeExtent(0,w-1,0,h-1,0,0);
  this->ImporterGreen->SetDataExtentToWholeExtent();
  this->ImporterGreen->Modified();
  this->ImporterGreen->Update();
  
  this->ImporterBlue->SetImportVoidPointer(image.top_left_ptr()+2*image.planestep());
  this->ImporterBlue->SetWholeExtent(0,w-1,0,h-1,0,0);
  this->ImporterBlue->SetDataExtentToWholeExtent();
  this->ImporterBlue->Modified();
  this->ImporterBlue->Update();
  
  this->Components->Update();
  output->ShallowCopy(this->Components->GetOutput());
  
#endif
  return 1;
}

// ----------------------------------------------------------------------------
int vidVideoReader::ReadFrame()
{
//  cout << "vidVideoReader::ReadFrame()" << endl;
  if(this->FileName->empty())
    {
    vtkErrorMacro(<<"file name required.");
    return 0;
    }
  bool status;
  
  if(this->Decoder==0)
    {
    this->Decoder=new vidtk::vidl_ffmpeg_frame_process("reader_process");
    vidtk::config_block c;
    c.add_parameter("filename","name of the file");
    c.set("filename",*this->FileName);
    // c.set("start_at_frame",0); // optional
    // c.set("stop_after_frame",1); // optional
    
    this->Decoder->set_params(c);
    status=this->Decoder->initialize();
    if(!status)
      {
      vtkErrorMacro(<<"failed to initialized the decoder");
      return 0;
      }
    }
  
//  cout << "frame=" << this->Frame << endl;
  
  if(!this->Opened || (this->Opened && (this->Frame<this->LastFrame || this->Frame-this->LastFrame>100)))
    {
    // try to be robust to not seekable files
    if(!(!this->Opened && this->Frame==0))
      {
      status=this->Decoder->seek(static_cast<unsigned int>(this->Frame));
      if(!status)
        {
        vtkErrorMacro(<<"failed to seek frame" << this->Frame << ".");
        return 0;
        }
      }
    
    status=this->Decoder->step();
    if(!status)
      {
      vtkErrorMacro(<<"failed to step.");
      return 0;
      }
    this->Opened=true;
    this->LastFrame=this->Frame;
    }
  else
    {
    int delta=this->Frame-this->LastFrame;
    int i=0;
    while(i<delta)
      {
      status=this->Decoder->step();
      if(!status)
        {
        vtkErrorMacro(<<"failed to step.");
        return 0;
        }
      ++i;
      }
    this->LastFrame=this->Frame;
    }
//  cout << "vidVideoReader::ReadFrame() END" << endl;
  return 1;
}

// ----------------------------------------------------------------------------
void vidVideoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "FileName: ";
  if(this->FileName==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->FileName << endl;
    }
 
  os << indent << "Frame: " << this->Frame << endl;
}
