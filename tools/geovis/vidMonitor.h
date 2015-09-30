/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidMonitor - Hansen (not Hoare) Monitor.
// .SECTION Description
// A monitor is a single lock with zero or more condition variables 
// ref: http://en.wikipedia.org/wiki/Monitor_(synchronization)
// ref: http://www.cs.utexas.edu/users/lorenzo/corsi/cs372h/07S/notes/Lecture12.pdf


#ifndef vidMonitor_h
#define vidMonitor_h

#include "vtkObject.h"

class vtkSimpleMutexLock;
class vtkSimpleConditionVariable;

class vidMonitor : public vtkObject
{
public:
  static vidMonitor *New();
  vtkTypeRevisionMacro(vidMonitor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Called by main thread.
  void Increase();
  // Called by sub-threads.
  void Decrease();
  // Called by main thread (blocking).
  void WaitForZero();
  
protected:
  vidMonitor();
  ~vidMonitor();
  int Count;
  vtkSimpleMutexLock *Lock;
  vtkSimpleConditionVariable *CountReachZero;
private:
  vidMonitor(const vidMonitor&);  // Not implemented.
  void operator=(const vidMonitor&);  // Not implemented.
};


#endif // #ifndef vidMonitor_h
