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
      DataSize(0)
  {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
  }

  ~SC11DemoReader(){ this->Close(); }

  int Open(
        const char *brickPath,
        const char *brickExt,
        const char *scalarName,
        int *extent)
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

    // compute the slice offset
    struct stat s;
    fstat(this->Handle, &s);
    this->DataSize=s.st_size/sizeof(float);

    int nx[3];
    CartesianExtent fileExtent(extent);
    fileExtent.Size(nx);
    size_t nxy=nx[0]*nx[1];
    int nz=this->DataSize/nxy;
    int r=this->DataSize%nxy;
    if (r)
      {
      // expect evenly divisible into xy slabs.
      sqErrorMacro(
        pCerr(),
        << "Invalid file size " << s.st_size << " for " << this->FileName);
      }

    int *nzs=0;
    int *zofs=0;
    int zof;
    if (this->WorldRank==0)
      {
      nzs=(int*)malloc(this->WorldSize*sizeof(int));
      MPI_Gather(&nz,1,MPI_INT,nzs,1,MPI_INT,0,MPI_COMM_WORLD);

      zofs=(int*)malloc(this->WorldSize*sizeof(int));
      zofs[0]=0;
      for (int i=1; i<this->WorldSize; ++i)
        {
        zofs[i]=zofs[i-1]+nzs[i-1]-1;
        }
      MPI_Scatter(zofs,1,MPI_INT,&zof,1,MPI_INT,0,MPI_COMM_WORLD);
      free(nzs);
      free(zofs);
      }
    else
      {
      MPI_Gather(&nz,1,MPI_INT,nzs,1,MPI_INT,0,MPI_COMM_WORLD);
      MPI_Scatter(zofs,1,MPI_INT,&zof,1,MPI_INT,0,MPI_COMM_WORLD);
      }

    // local extent
    this->Extent=fileExtent;
    this->Extent.GetData()[4]=zof;
    this->Extent.GetData()[5]=zof+nz;

    // mmap file
    this->Data=(float*)mmap(0,s.st_size,PROT_READ,MAP_PRIVATE,this->Handle,0);
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

  void Close()
  {
    if (this->Data)
      {
      munmap(this->Data, this->DataSize*sizeof(float));
      this->Data=0;
      this->DataSize=0;
      }
    if (this->Handle>=0)
      {
      close(this->Handle);
      this->Handle=-1;
      }
  }

  float *GetData(){ return this->Data; }
  size_t GetDataSize(){ return this->DataSize; }

  int *GetExtent(){ return this->Extent.GetData(); }

  void Print(ostream &os)
  {
    os
      << "WorldRank=" << this->WorldRank << endl
      << "WorldSize=" << this->WorldSize << endl
      << "FileName=" <<  this->FileName << endl
      << "Handle=" << this->Handle << endl
      << "Data=" << this->Data << endl
      << "DataSize=" << this->DataSize << endl
      << "Extent=" << this->Extent << endl;
  }

private:
  int WorldRank;
  int WorldSize;
  string FileName;
  int Handle;
  float *Data;
  size_t DataSize;
  CartesianExtent Extent; // Extent of my piece.
};

#endif

