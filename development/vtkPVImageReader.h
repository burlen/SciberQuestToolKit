/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVImageReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageReader - Superclass of transformable binary file readers.
// .SECTION Description
// vtkPVImageReader provides methods needed to read a region from a file.
// It supports both transforms and masks on the input data, but as a result
// is more complicated and slower than its parent class vtkPVImageReader2.

// .SECTION See Also
// vtkBMPReader vtkPNMReader vtkTIFFReader

#ifndef __vtkPVImageReader_h
#define __vtkPVImageReader_h

#include "vtkImageReader2.h"

class vtkTransform;

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_IO_EXPORT vtkPVImageReader : public vtkImageReader2
{
public:
  static vtkPVImageReader *New();
  vtkTypeRevisionMacro(vtkPVImageReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Set/get the data VOI. You can limit the reader to only
  // read a subset of the data. 
  vtkSetVector6Macro(DataVOI,int);
  vtkGetVector6Macro(DataVOI,int);

  //BTX
  #if defined vtkPVImageReaderTIME
  // Description:
  // Added for testing purposes.
  virtual int CanReadFile(const char* vtkNotUsed(fname));
  #endif
  //ETX

  // Description:
  // Set/Get the Data mask.
  vtkGetMacro(DataMask,unsigned short);
  void SetDataMask(int val) 
    {
      if (val == this->DataMask)
        {
        return;
        }
      this->DataMask = static_cast<unsigned short>(val);
      this->Modified();
    }
  
  // Description:
  // Set/Get transformation matrix to transform the data from slice space
  // into world space. This matrix must be a permutation matrix. To qualify,
  // the sums of the rows must be + or - 1.
  virtual void SetTransform(vtkTransform*);
  vtkGetObjectMacro(Transform,vtkTransform);

  // Warning !!!
  // following should only be used by methods or template helpers, not users
  void ComputeInverseTransformedExtent(int inExtent[6],
                                       int outExtent[6]);
  void ComputeInverseTransformedIncrements(vtkIdType inIncr[3],
                                           vtkIdType outIncr[3]);

  int OpenAndSeekFile(int extent[6], int slice);
  
  // Description:
  // Set/get the scalar array name for this data set.
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);
  
protected:
  vtkPVImageReader();
  ~vtkPVImageReader();

  unsigned short DataMask;  // Mask each pixel with ...

  vtkTransform *Transform;

  void ComputeTransformedSpacing (double Spacing[3]);
  void ComputeTransformedOrigin (double origin[3]);
  void ComputeTransformedExtent(int inExtent[6],
                                int outExtent[6]);
  void ComputeTransformedIncrements(vtkIdType inIncr[3],
                                    vtkIdType outIncr[3]);

  int DataVOI[6];
  
  char *ScalarArrayName;

  int ProcId;              // rank of this process
  int NProcs;              // number of processes
  char HostName[5];        // short host name where this process runs
  
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  void ExecuteData(vtkDataObject *data);
private:
  vtkPVImageReader(const vtkPVImageReader&);  // Not implemented.
  void operator=(const vtkPVImageReader&);  // Not implemented.
};

#endif
