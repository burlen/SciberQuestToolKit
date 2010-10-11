/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQMetaSource.h"

#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSQMetaSource, "$Revision: 0.0 $");

//-----------------------------------------------------------------------------
vtkInformationKeyMacro(vtkSQMetaSource,SOURCE,ObjectBase);

//-----------------------------------------------------------------------------
void vtkSQMetaSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent.GetNextIndent());
}
