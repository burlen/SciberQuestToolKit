/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/

#ifndef vtkSC11DemoExtentTranslator_h
#define vtkSC11DemoExtentTranslator_h

#include "vtkPVExtentTranslator.h"

#include "CartesianExtent.h"

#include <vector>
using std::vector;

//==============================================================================
class vtkSC11DemoExtentTranslator : public vtkPVExtentTranslator
{
public:
  static vtkSC11DemoExtentTranslator *New();

  vtkTypeRevisionMacro(vtkSC11DemoExtentTranslator, vtkPVExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent){}

  virtual void SetWholeExtent(CartesianExtent &ext)
  {
    this->vtkExtentTranslator::SetWholeExtent(ext.GetData());
    this->WholeExtent.Set(ext);
  }

  virtual void SetUpdateExtent(CartesianExtent &ext);

  virtual int PieceToExtentThreadSafe(
        int piece,
        int numPieces,
        int ghostLevel,
        int *wholeExtent,
        int *resultExtent,
        int splitMode,
        int byPoints);

protected:
  vtkSC11DemoExtentTranslator(){}
  virtual ~vtkSC11DemoExtentTranslator(){}

private:
  //BTX
  CartesianExtent WholeExtent;
  vector<CartesianExtent> UpdateExtent;
  //ETX

private:
  vtkSC11DemoExtentTranslator(const vtkSC11DemoExtentTranslator&); // Not implemented
  void operator=(const vtkSC11DemoExtentTranslator&); // Not implemented
};

#endif

