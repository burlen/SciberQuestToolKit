/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBOVReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBOVReader -- Connects the VTK pipeline to BOVReader class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVReader so that "brick of values" datasets, including time series,
// can be read in parallel.
//
// .SECTION See Also
// BOVReader

#ifndef vtkBOVReader_h
#define vtkBOVReader_h

//#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkImageAlgorithm.h"

// define this for cerr status.
// #define vtkBOVReaderDEBUG

//BTX
class BOVReader;
class vtkInformationStringKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
// class vtkCallbackCommand;
//ETX

class VTK_EXPORT vtkBOVReader : public vtkImageAlgorithm
{
public:
  static vtkBOVReader *New();
  vtkTypeRevisionMacro(vtkBOVReader,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description:
  // Get/Set the file to read.
  void SetFileName(const char *file);
  vtkGetStringMacro(FileName);
  // Description
  // Determine if the file can be read.
  int CanReadFile(const char *file);

  // Description:
  // Array selection.
  void SetPointArrayStatus(const char *name, int status);
  int GetPointArrayStatus(const char *name);
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int idx);

  // Description:
  // Subseting interface.
  void SetSubset(int ilo,int ihi, int jlo, int jhi, int klo, int khi);
  void SetSubset(const int *s);
  vtkGetVector6Macro(Subset,int);
  // Description:
  // For PV UI. Range domains only work with arrays of size 2.
  void SetISubset(int ilo, int ihi);
  void SetJSubset(int jlo, int jhi);
  void SetKSubset(int klo, int khi);
  vtkGetVector2Macro(ISubsetRange,int);
  vtkGetVector2Macro(JSubsetRange,int);
  vtkGetVector2Macro(KSubsetRange,int);

  // Description:
  // Activate a meta read where no arrays are read.
  // The meta data incuding file name is passed
  // downstream via pipeline information objects. The
  // reader may be used in either mode, however take care
  // that RequestInformation is called after.
  vtkSetMacro(MetaRead,int);
  vtkGetMacro(MetaRead,int);

  // Description:
  // Sets modified if array selection changes.
  static void SelectionModifiedCallback( 
      vtkObject*,
      unsigned long,
      void* clientdata,
      void* );
protected:
  // Description:
  // Read the dataset.
  int RequestData(
      vtkInformation *req, 
      vtkInformationVector **input,
      vtkInformationVector *output);

  // Description:
  // Extract meta-data from the file that is to be read.
  int RequestInformation(
      vtkInformation* req, 
      vtkInformationVector**,
      vtkInformationVector* info);

  vtkBOVReader();
  ~vtkBOVReader();

private:
  vtkBOVReader(const vtkBOVReader &); // Not implemented
  void operator=(const vtkBOVReader &); // Not implemented
  //
  void Clear();

private:
  BOVReader *Reader;       // Implementation
  char *FileName;          // Name of data file to load.
  bool FileNameChanged;    // Flag indicating that the dataset needs to be opened
  int Subset[6];           // Subset to read
  int ISubsetRange[2];     // bounding extents of the subset
  int JSubsetRange[2];
  int KSubsetRange[2];
  int MetaRead;            // flag indicating type of read meta or actual
  int ProcId;              // rank of this process
  int NProcs;              // number of processes
  char HostName[5];        // short host name where this process runs
};

#endif
