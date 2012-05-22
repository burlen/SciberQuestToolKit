/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/

#include "vtkSC11DemoExtentTranslator.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "Tuple.hxx"
#include "postream.h"

#ifndef SQTK_WITHOUT_MPI
#include <mpi.h>
#endif

//#define vtkSC11DemoExtentTranslatorDEBUG

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSC11DemoExtentTranslator, "$Revision: 0.0 $");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSC11DemoExtentTranslator);

//-----------------------------------------------------------------------------
void vtkSC11DemoExtentTranslator::SetUpdateExtent(CartesianExtent &ext)
{
  #ifndef SQTK_WITHOUT_MPI
  int worldSize;
  int worldRank;
  MPI_Comm_size(MPI_COMM_WORLD,&worldSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&worldRank);

  int *allUpdateExt=(int*)malloc(6*worldSize*sizeof(int));

  MPI_Allgather(
        ext.GetData(),
        6,
        MPI_INT,
        allUpdateExt,
        6,
        MPI_INT,
        MPI_COMM_WORLD);

  this->UpdateExtent.resize(worldSize);
  for (int i=0; i<worldSize; ++i)
    {
    int ii=6*i;
    this->UpdateExtent[i].Set(allUpdateExt+ii);
    }
  #endif
}

//-----------------------------------------------------------------------------
int vtkSC11DemoExtentTranslator::PieceToExtentThreadSafe(
      int piece,
      int numPieces,
      int ghostLevel,
      int *wholeExtent,
      int *resultExtent,
      int splitMode,
      int byPoints)
{
  #if defined vtkSC11DemoExtentTranslatorDEBUG
  pCerr()
    << "===============================vtkSC11DemoExtentTranslator::PieceToExtentThreadSafe" << endl
    << "piece=" << piece << endl
    << "numPieces=" << numPieces << endl
    << "ghostLevel=" << ghostLevel << endl
    << "wholeExtent=" << Tuple<int>(wholeExtent,6) << endl
    << "resultExtent=" << Tuple<int>(resultExtent,6) << endl
    << "splitMode=" << splitMode << endl
    << "byPoints=" << byPoints << endl;
  #endif

  if ((this->UpdateExtent.size()<=piece) || this->UpdateExtent[piece].Empty())
    {
    #if defined vtkSC11DemoExtentTranslatorDEBUG
    pCerr() << "Fallback to vtkPVExtentTranslator::PieceToExtentThreadSafe" << endl;
    #endif
    return
      vtkPVExtentTranslator::PieceToExtentThreadSafe(
            piece,
            numPieces,
            ghostLevel,
            wholeExtent,
            resultExtent,
            splitMode,
            byPoints);
    }

  this->UpdateExtent[piece].GetData(resultExtent);

  #if defined vtkSC11DemoExtentTranslatorDEBUG
  pCerr() << "Set update extent of piece " << piece << " to " << Tuple<int>(resultExtent,6) << endl;
  #endif

  return 1;
}
