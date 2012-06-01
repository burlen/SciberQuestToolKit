/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __vtkSQWaveletFilter_h
#define __vtkSQWaveletFilter_h

#include <set>
using std::set;
#include <string>
using std::string;

#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;

// .DESCRIPTION
//
class vtkSQWaveletFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQWaveletFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQWaveletFilter *New();

  // Description:
  // Array selection.
  void AddInputArray(const char *name);
  void ClearInputArrays();

  //BTX
  enum {
    MODE_3D=0,
    MODE_2D_XY,
    MODE_2D_XZ,
    MODE_2D_YZ
    };
  //ETX
  // Description:
  // Set the mode to 2 or 3D.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);

  // Description:
  // When set the inverse wavelet transformation is performed.
  vtkSetMacro(Invert,int);
  vtkGetMacro(Invert,int);

  //BTX
  enum {
    WAVELET_BASIS_NONE=0,
    WAVELET_BASIS_HAAR=1
    };
  //ETX
  // Description:
  // Select a basis for the transform.
  void SetWaveletBasis(int basis);
  vtkGetMacro(WaveletBasis,int);

protected:
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQWaveletFilter();
  virtual ~vtkSQWaveletFilter();

  // Description:
  // number of ghost cells to request is determined at run time
  // when a basis function is selected.
  vtkSetMacro(NumberOfGhostCells,int);
  vtkGetMacro(NumberOfGhostCells,int);

private:
  int WorldSize;
  int WorldRank;
  //
  set<string> InputArrays;
  //
  int WaveletBasis;
  int Invert;
  //
  int Mode;
  int NumberOfGhostCells;

private:
  vtkSQWaveletFilter(const vtkSQWaveletFilter &); // Not implemented
  void operator=(const vtkSQWaveletFilter &); // Not implemented
};

#endif
