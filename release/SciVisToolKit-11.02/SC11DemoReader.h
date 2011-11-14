/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/

#ifndef SC11DemoReader_h
#define SC11DemoReader_h


#include "CartesianExtent.h"
#include "SQMacros.h"
#include "postream.h"

#include <string>
using std::string;
#include <sstream>
using std::ostringstream;

#include <mpi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

class SC11DemoReader
{
public:
  SC11DemoReader()
          :
      WorldRank(0),
      WorldSize(1),
      FileName(""),
      Handle(-1),
      Data(0),
      Size(0),
      SlabSize(0),
      DataSize(0),
      Nxy(0),
      Nz(0)
  {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
  }

  ~SC11DemoReader(){ this->Close(); }

  int Open(
        const char *brickPath,
        const char *brickExt,
        const char *scalarName,
        CartesianExtent fileExtent)
  {
    if (this->Data)
      {
      this->Close();
      }

    ostringstream oss;
    oss
      << brickPath
      << PATH_SEP
      << scalarName
      << setw(2) << setfill('0') << this->WorldRank
      << brickExt;
    this->FileName=oss.str();

    // open
    this->Handle=open(this->FileName.c_str(),O_RDONLY);
    if (this->Handle<0)
      {
      int eno=errno;
      sqErrorMacro(
        pCerr(),
        << "Failed to open brick file:" << this->FileName << endl
        << strerror(eno));
      return -1;
      }

    // calculate the in memory extent
    struct stat s;
    fstat(this->Handle, &s);
    this->Size=s.st_size;                    // dataset size in bytes
    this->DataSize=this->Size/sizeof(float); // dataset size in floats

    pCerr() << "FileExtent=" << fileExtent << endl;

    int nx[3];
    fileExtent.Size(nx);
    this->Nxy=nx[0]*nx[1];                   // n pts per slab on disk and in memory
    this->SlabSize=this->Nxy*sizeof(float);  // slab size in bytes on disk and in memory
    this->Nz=this->DataSize/this->Nxy;       // n slabs on disk, *NOT* in memory

    //pCerr() << "SanityCheck=" << ((this->DataSize%this->Nxy)==0) << endl;

    unsigned long ncz=this->Nz;
    unsigned long *nczs=0;
    unsigned long *czofs=0;
    unsigned long czof;
    if (this->WorldRank==0)
      {
      nczs=(unsigned long*)malloc(this->WorldSize*sizeof(unsigned long));
      MPI_Gather(&ncz,1,MPI_UNSIGNED_LONG,nczs,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);

      czofs=(unsigned long*)malloc(this->WorldSize*sizeof(unsigned long));
      czofs[0]=0;
      for (int i=1; i<this->WorldSize; ++i)
        {
        czofs[i]=czofs[i-1]+nczs[i-1];
        }
      MPI_Scatter(czofs,1,MPI_UNSIGNED_LONG,&czof,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
      free(nczs);
      free(czofs);
      }
    else
      {
      MPI_Gather(&ncz,1,MPI_UNSIGNED_LONG,nczs,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
      MPI_Scatter(czofs,1,MPI_UNSIGNED_LONG,&czof,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
      }

    this->MemoryExtent=fileExtent;
    this->MemoryExtent[1]-=1;           // move cell data to points
    this->MemoryExtent[3]-=1;           // move cell data to points
    this->MemoryExtent[4]=czof;         // keep number of cells
    this->MemoryExtent[5]=czof+ncz-1;

    //cerr << "MemoryExtent=" << this->MemoryExtent << endl;

    // mmap file
    this->Data=(float*)mmap(0,this->Size,PROT_READ,MAP_PRIVATE,this->Handle,0);
    if ((void*)this->Data==(void*)-1)
      {
      int eno=errno;
      sqErrorMacro(
        pCerr(), << "Mmap failed. " << strerror(eno));
      this->Data=0;
      return -1;
      }

    return 0;
  }

  ///
  void Close()
  {
    if (this->Data)
      {
      munmap(this->Data,this->Size);
      this->Data=0;
      this->Size=0;
      this->SlabSize=0;
      this->DataSize=0;
      this->Nxy=0;
      this->Nz=0;
      this->MemoryExtent.Clear();
      }
    if (this->Handle>=0)
      {
      close(this->Handle);
      this->Handle=-1;
      }
  }

  ///
  float *GetData(){ return this->Data; }

  ///
  size_t GetSize(){ return this->Size; }
  ///
  size_t GetDataSize(){ return this->DataSize; }

  ///
  CartesianExtent &GetMemoryExtent(){ return this->MemoryExtent; }

  ///
  void CopyTo(vtkFloatArray *fa)
  {
    CartesianExtent pointExt(this->MemoryExtent);
    pointExt.CellToNode();
    size_t nPoints=pointExt.Size();

    //cerr
    //  << "SanityCheck="
    //  << ((nPoints*sizeof(float))==(this->Size+this->SlabSize))
    //  << endl;

    fa->SetNumberOfComponents(1);
    fa->SetNumberOfTuples(nPoints);
    float *pFa=fa->GetPointer(0);
    // hack to work around the way the data is split on disk.
    // first slab is duplicated. This is what would occur if you
    // ran the cell to point filter on cell data, however the
    // following is hella faster.
    memcpy(pFa,this->Data,this->SlabSize);
    pFa+=this->Nxy;
    memcpy(pFa,this->Data,this->Size);
  }

  void Print(ostream &os)
  {
    os
      << "WorldRank=" << this->WorldRank << endl
      << "WorldSize=" << this->WorldSize << endl
      << "FileName=" <<  this->FileName << endl
      << "Handle=" << this->Handle << endl
      << "Data=" << this->Data << endl
      << "Size=" << this->Size << endl
      << "SlabSize=" << this->SlabSize << endl
      << "DataSize=" << this->DataSize << endl
      << "Nxy=" << this->Nxy << endl
      << "Nz=" << this->Nz << endl
      << "MemoryExtent=" << this->MemoryExtent << endl;
  }

private:
  int WorldRank;
  int WorldSize;
  string FileName;
  int Handle;
  float *Data;
  unsigned long Size;       // number of bytes in mapped region
  unsigned long SlabSize;   // number of bytes in a slab
  unsigned long DataSize;   // number of floats in mapped region
  unsigned long Nxy;        // point slab size in floats
  unsigned long Nz;         // point slab count
  CartesianExtent MemoryExtent;   // Extent of my piece.
};

#endif

