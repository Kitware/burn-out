/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidMonitor.h"
#include "vtkObjectFactory.h"
#include "vtkMutexLock.h"
#include "vtkConditionVariable.h"

vtkCxxRevisionMacro(vidMonitor, "$Revision$");
vtkStandardNewMacro(vidMonitor);

// ----------------------------------------------------------------------------
vidMonitor::vidMonitor()
{
  this->Lock=new vtkSimpleMutexLock;
  this->Count=0;
  this->CountReachZero=new vtkSimpleConditionVariable;
}

// ----------------------------------------------------------------------------
vidMonitor::~vidMonitor()
{
  delete this->Lock;
  delete this->CountReachZero;
}


// ----------------------------------------------------------------------------
// Called by main thread.
void vidMonitor::Increase()
{
  this->Lock->Lock();
  ++this->Count;
  cout << "vidMonitor::Increase(): count=" << this->Count << endl;
  this->Lock->Unlock();
}

// ----------------------------------------------------------------------------
// Called by sub-threads.
void vidMonitor::Decrease()
{
  this->Lock->Lock();
  --this->Count;
  cout << "vidMonitor::Decrease(): count=" << this->Count << endl;
  if(this->Count==0)
    {
    this->CountReachZero->Signal();
    }
  else
    {
    if(this->Count<0)
      {
      cerr << "Error!" << endl;
      }
    }
  this->Lock->Unlock();
}

// ----------------------------------------------------------------------------
// Called by main thread (blocking).
void vidMonitor::WaitForZero()
{
  this->Lock->Lock();
  while(this->Count>0)
    {
    this->CountReachZero->Wait(*this->Lock);
    }
  this->Lock->Unlock();
}

// ----------------------------------------------------------------------------
void vidMonitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
