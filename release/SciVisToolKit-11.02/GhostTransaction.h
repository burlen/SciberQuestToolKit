/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __GhostTransaction_h
#define __GhostTransaction_h

#include "CartesianExtent.h"
#include "MPIRawArrayIO.hxx"
#include <mpi.h>

/// GhostTransaction - Helper class to handle communication of ghost data
/**
Extents are always ionternally described in cell space. During
execution point data can be specified and extents are converted
to point based extents.
*/
class GhostTransaction
{
public:
  GhostTransaction()
      :
    SrcRank(0),
    DestRank(0)
  {}

  GhostTransaction(
        int srcRank,
        const CartesianExtent &srcExt,
        int destRank,
        const CartesianExtent &destExt,
        const CartesianExtent &intExt)
      :
    SrcRank(srcRank),
    SrcExt(srcExt),
    DestRank(destRank),
    DestExt(destExt),
    IntExt(intExt)
  {}

  ~GhostTransaction(){}

  void SetSourceRank(int rank){ this->SrcRank=rank; }
  int GetSourceRank() const { return this->SrcRank; }

  void SetSourceExtent(CartesianExtent &srcExt){ this->SrcExt=srcExt; }
  CartesianExtent &GetSourceExtent(){ return this->SrcExt; }
  const CartesianExtent &GetSourceExtent() const { return this->SrcExt; }

  void SetDestinationRank(int rank){ this->DestRank=rank; }
  int GetDestinationRank() const { return this->DestRank; }

  void SetDestinationExtent(CartesianExtent &destExt){ this->DestExt=destExt; }
  CartesianExtent &GetDestinationExtent(){ return this->DestExt; }
  const CartesianExtent &GetDestinationExtent() const { return this->DestExt; }

  void SetIntersectionExtent(CartesianExtent &intExt){ this->IntExt=intExt; }
  CartesianExtent &GetIntersectionExtent(){ return this->IntExt; }
  const CartesianExtent &GetIntersectionExtent() const { return this->IntExt; }

  template<typename T>
  int Execute(
        int rank,
        int nComps,
        T *srcData,
        T *destData,
        bool pointData,
        int mode);

private:
  int SrcRank;
  CartesianExtent SrcExt;

  int DestRank;
  CartesianExtent DestExt;

  CartesianExtent IntExt;
};

ostream &operator<<(ostream &os, const GhostTransaction &gt);

//-----------------------------------------------------------------------------
template<typename T>
int GhostTransaction::Execute(
       int rank,
       int nComps,
       T *srcData,
       T *destData,
       bool pointData,
       int mode)
{
  int iErr=0;

  if (rank==this->SrcRank)
    {
    // sender
    CartesianExtent srcExt=this->SrcExt;
    srcExt.Shift(0,-this->SrcExt[0]);
    srcExt.Shift(1,-this->SrcExt[2]);
    srcExt.Shift(2,-this->SrcExt[4]);

    CartesianExtent intExt=this->IntExt;
    intExt.Shift(0,-this->SrcExt[0]);
    intExt.Shift(1,-this->SrcExt[2]);
    intExt.Shift(2,-this->SrcExt[4]);

    if (pointData)
      {
      srcExt = CartesianExtent::CellToNode(srcExt,mode);
      intExt = CartesianExtent::CellToNode(intExt,mode);
      }

    MPI_Datatype subarray;
    CreateCartesianView<T>(
          srcExt,
          intExt,
          nComps,
          subarray);

    iErr=MPI_Send(
          srcData,
          1,
          subarray,
          this->DestRank,
          1,
          MPI_COMM_WORLD);

    MPI_Type_free(&subarray);
    }
  else
  if (rank==this->DestRank)
    {
    // reciever
    CartesianExtent destExt=this->DestExt;
    destExt.Shift(0,-this->DestExt[0]);
    destExt.Shift(1,-this->DestExt[2]);
    destExt.Shift(2,-this->DestExt[4]);

    CartesianExtent intExt=this->IntExt;
    intExt.Shift(0,-this->DestExt[0]);
    intExt.Shift(1,-this->DestExt[2]);
    intExt.Shift(2,-this->DestExt[4]);

    if (pointData)
      {
      destExt = CartesianExtent::CellToNode(destExt,mode);
      intExt = CartesianExtent::CellToNode(intExt,mode);
      }

    MPI_Datatype subarray;
    CreateCartesianView<T>(
          destExt,
          intExt,
          nComps,
          subarray);

    iErr=MPI_Recv(
          destData,
          1,
          subarray,
          this->SrcRank,
          1,
          MPI_COMM_WORLD,
          MPI_STATUS_IGNORE);

    MPI_Type_free(&subarray);
    }

  return iErr;
}

#endif

