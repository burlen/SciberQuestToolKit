/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "CartesianDecomp.h"

#include "CartesianDataBlock.h"
#include "CartesianDataBlockIODescriptor.h"
#include "Tuple.hxx"
#include "SQMacros.h"

#include <mpi.h>

// #define CartesianDecompDEBUG

// Search the decomposition for the block conaining the point. Return
// non-zero if an error occured or the point was contained by any block
// in the decomposition.
//*****************************************************************************
int DecompSearch(
      CartesianDecomp *decomp, // decomp to search
      int *dext,               // decomp coordinate range to search
      int q,                   // coordinate direction
      const double *pt,        // find index of block comntaining this point
      int *I                   // return the index (should be initialize 0).
      )
{
  const int qq=2*q;

  // get the median in q coordinate direction
  I[q]=(dext[qq]+dext[qq+1])/2;

  CartesianBounds &bounds=decomp->GetBlock(I)->GetBounds();

  if ( pt[q]>=bounds[qq]
    && pt[q]<=bounds[qq+1] )
    {
    // point is inside some block on I[q] coordinate
    return 0;
    }
  else
  if ( pt[q]<bounds[qq] )
    {
    // reduce search space to the lower half of the range.
    dext[qq+1]=I[q]-1;

    // failed to locate the point, in any block along qth coordinate.
    if (dext[qq+1]<0)
      {
      return 1;
      }
    }
  else
  // if ( pt[q]>bounds[qq+1] )
    {
    // reduce search space to upper half of the range.
    dext[qq]=I[q]+1;

    // failed to locate the point, in any block along qth coordinate.
    if (dext[qq]>(decomp->GetDecompDimensions()[q]))
      {
      return 1;
      }
    }

  // continue the search in the reduced space.
  return DecompSearch(decomp,dext,q,pt,I);
}


//-----------------------------------------------------------------------------
CartesianDecomp::CartesianDecomp()
{
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=0;

  this->SetOrigin(0.0,0.0,0.0);
  this->SetSpacing(0.0,0.0,0.0);

  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;

  this->NGhosts=1;
}

//-----------------------------------------------------------------------------
CartesianDecomp::~CartesianDecomp()
{
  this->ClearDecomp();
  this->ClearIODescriptors();
}

//-----------------------------------------------------------------------------
void CartesianDecomp::ClearDecomp()
{
  size_t nBlocks=this->Decomp.size();
  for (size_t i=0; i<nBlocks; ++i)
    {
    delete this->Decomp[i];
    }
  this->Decomp.clear();
}

//-----------------------------------------------------------------------------
void CartesianDecomp::ClearIODescriptors()
{
  size_t nBlocks=this->IODescriptors.size();
  for (size_t i=0; i<nBlocks; ++i)
    {
    delete this->IODescriptors[i];
    }
  this->IODescriptors.clear();
}


//-----------------------------------------------------------------------------
void CartesianDecomp::SetFileExtent(
      int ilo,
      int ihi,
      int jlo,
      int jhi,
      int klo,
      int khi)
{
  this->FileExtent.Set(ilo,ihi,jlo,jhi,klo,khi);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetFileExtent(const int ext[6])
{
  this->FileExtent.Set(ext);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetFileExtent(const CartesianExtent &ext)
{
  this->FileExtent=ext;
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetOrigin(double x0, double y0, double z0)
{
  double X0[3]={x0,y0,z0};
  this->SetOrigin(X0);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetOrigin(const double x0[3])
{
  this->X0[0]=x0[0];
  this->X0[1]=x0[1];
  this->X0[2]=x0[2];
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetSpacing(double dx, double dy, double dz)
{
  double dX[3]={dx,dy,dz};
  this->SetSpacing(dX);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetSpacing(const double dX[3])
{
  this->DX[0]=dX[0];
  this->DX[1]=dX[1];
  this->DX[2]=dX[2];
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetBounds(
      double xlo,
      double xhi,
      double ylo,
      double yhi,
      double zlo,
      double zhi)
{
  this->Bounds.Set(xlo,xhi,ylo,yhi,zlo,zhi);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetBounds(const double bounds[6])
{
  this->Bounds.Set(bounds);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetBounds(const CartesianBounds &bounds)
{
  this->Bounds=bounds;
}

//-----------------------------------------------------------------------------
void CartesianDecomp::ComputeBounds()
{
  this->Extent.GetBounds(this->X0,this->DX,this->Bounds.GetData());
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetExtent(
      int ilo,
      int ihi,
      int jlo,
      int jhi,
      int klo,
      int khi)
{
  this->Extent.Set(ilo,ihi,jlo,jhi,klo,khi);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetExtent(const int ext[6])
{
  this->Extent.Set(ext);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetExtent(const CartesianExtent &ext)
{
  this->Extent=ext;
}

//-----------------------------------------------------------------------------
int CartesianDecomp::SetDecompDims(int nBlocks)
{
  if (nBlocks==0)
    {
    sqErrorMacro(cerr,"0 is an invald number of blocks.");
    return 0;
    }

  // create a partitioning yeilding the desired number
  // of blocks distributed over 3 dimensions.
  int decompDims[3]={0};

  MPI_Dims_create(nBlocks,3,decompDims);

  this->SetDecompDims(decompDims);

  return 1;
}

//-----------------------------------------------------------------------------
int CartesianDecomp::SetDecompDims(int ni,int nj,int nk)
{
  int decompDims[3]={ni,nj,nk};

  return this->SetDecompDims(decompDims);
}

//-----------------------------------------------------------------------------
int CartesianDecomp::SetDecompDims(const int decompDims[3])
{
  if (decompDims[0]<1)
    {
    sqErrorMacro(cerr,"Decomp dims cannot be zero.");
    return 0;
    }

  // special case, if user explicitly passes both
  // y and z dims zero, we generate a decomposition
  // using the x dimension as the number of blocks.
  if (decompDims[1]<1 && decompDims[2]<1)
    {
    return this->SetDecompDims(decompDims[0]);
    }
  // in any other case a zero entry in any of the dims
  // is an error.
  else
  if (decompDims[0]<1 || decompDims[1]<1 || decompDims[2]<1)
    {
    sqErrorMacro(cerr,
        << "Invald decomp dims requested "
        << Tuple<int>(decompDims,3) << ".");
    return 0;
    }

  this->DecompDims[0]=decompDims[0];
  this->DecompDims[1]=decompDims[1];
  this->DecompDims[2]=decompDims[2];

  // save ni*nj for faster indexing.
  this->DecompDims[3]=this->DecompDims[0]*this->DecompDims[1];

  return 1;
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetPeriodicBC(int px, int py, int pz)
{
  int periodic[3]={px,py,pz};
  this->SetPeriodicBC(periodic);
}

//-----------------------------------------------------------------------------
void CartesianDecomp::SetPeriodicBC(const int periodic[3])
{
  this->PeriodicBC[0]=periodic[0];
  this->PeriodicBC[1]=periodic[1];
  this->PeriodicBC[2]=periodic[2];
}

//-----------------------------------------------------------------------------
int CartesianDecomp::DecomposeDomain()
{
  int nCells[3];
  this->Extent.Size(nCells);

  if ( this->DecompDims[0]>=nCells[0]
    || this->DecompDims[1]>=nCells[1]
    || this->DecompDims[2]>=nCells[2] )
    {
    sqErrorMacro(cerr,
      << "Too many blocks "
      << Tuple<int>(this->DecompDims,3)
      << " requested for extent "
      << this->Extent
      << ".");
    return 0;
    }

  // free any resources allocated in a previous decomposition.
  this->ClearDecomp();
  this->ClearIODescriptors();

  size_t nBlocks
    = this->DecompDims[0]*this->DecompDims[1]*this->DecompDims[2];
  this->Decomp.resize(nBlocks,0);
  this->IODescriptors.resize(nBlocks,0);

  int smBlockSize[3]={0};
  int nLarge[3]={0};
  for (int q=0; q<3; ++q)
    {
    smBlockSize[q]=nCells[q]/this->DecompDims[q];
    nLarge[q]=nCells[q]%this->DecompDims[q];
    }

  #ifdef CartesianDecompDEBUG
  cerr
    << "DecompDims=" << Tuple<int>(this->DecompDims,3) << endl
    << "nCells=" << Tuple<int>(nCells,3) << endl
    << "smBlockSize=" << Tuple<int>(smBlockSize,3) << endl
    << "nLarge=" << Tuple<int>(nLarge,3) << endl;
  #endif

  CartesianExtent fileExt(this->FileExtent);
  fileExt.CellToNode(); // dual grid

  int idx=0;

  // allocate and initialize each block in the new decomposition.
  for (int k=0; k<this->DecompDims[2]; ++k)
    {
    for (int j=0; j<this->DecompDims[1]; ++j)
      {
      for (int i=0; i<this->DecompDims[0]; ++i)
        {
        CartesianDataBlock *block=new CartesianDataBlock;

        block->SetId(i,j,k,idx);
        int *I=block->GetId();

        CartesianExtent &ext=block->GetExtent();

        for (int q=0; q<3; ++q)
          {
          int lo=2*q;
          int hi=lo+1;

          // compute extent
          if (I[q]<nLarge[q])
            {
            ext[lo]=I[q]*(smBlockSize[q]+1);
            ext[hi]=ext[lo]+smBlockSize[q];
            }
          else
            {
            ext[lo]=I[q]*smBlockSize[q]+nLarge[q];
            ext[hi]=ext[lo]+smBlockSize[q]-1;
            }
          }

        // compute bounds
        double *bounds=block->GetBounds().GetData();
        ext.GetBounds(this->X0,this->DX,bounds);

        #ifdef CartesianDecompDEBUG
        cerr << *block << endl;
        #endif

        // create an io descriptor.
        CartesianExtent blockExt(ext);
        blockExt.CellToNode(); //dual grid
        CartesianDataBlockIODescriptor *descr
            = new CartesianDataBlockIODescriptor(
                blockExt,fileExt,this->PeriodicBC,this->NGhosts);

        this->Decomp[idx]=block;
        this->IODescriptors[idx]=descr;
        ++idx;
        }
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
CartesianDataBlock *CartesianDecomp::GetBlock(const double *pt)
{
  int I[3]={0};

  int dext[6]={
      0,this->DecompDims[0]-1,
      0,this->DecompDims[1]-1,
      0,this->DecompDims[2]-1};

  if ( DecompSearch(this,dext,0,pt,I)
    || DecompSearch(this,dext,1,pt,I)
    || DecompSearch(this,dext,2,pt,I) )
    {
    sqErrorMacro(cerr,
        "Point "
        << Tuple<double>(pt,3)
        << " not found in " 
        << this->Bounds << ".");
    return 0;
    }

  return this->GetBlock(I);
}

// //-----------------------------------------------------------------------------
// CartesianDataBlockIODescriptor *CartesianDecomp::GetBlockIODescriptor(int idx)
// {
//   if (this->IODescriptors[i]==0)
//     {
//     this->IODescriptors[i]
//       = new CartesianDataBlockIODescriptor(
//           this->GetBlock(idx),this->Extent,this->PeriodicBC,this->NGhosts);
//     }
// 
//   return this->IODescriptors[i];
// }


