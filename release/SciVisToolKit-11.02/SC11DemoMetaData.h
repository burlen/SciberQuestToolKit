/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/

#ifndef SC11DemoMetaData_h
#define SC11DemoMetaData_h

#include "CartesianExtent.h"
#include "BinaryStream.hxx"
#include "FsUtils.h"
#include "SQMacros.h"
#include "postream.h"

#include <mpi.h>
#include <string>
using std::string;
#include <fstream>
using std::ifstream;

//=============================================================================
class SC11DemoMetaData
{
public:
  SC11DemoMetaData()
          :
      WorldRank(0),
      WorldSize(1),
      NTimeSteps(0)
  {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
  }
  ~SC11DemoMetaData(){ this->Close(); }

  int Open(const char *fileName)
  {
    this->Close();

    if (this->WorldRank==0)
      {
      // Open
      ifstream metaFile(fileName);
      if (!metaFile.is_open())
        {
        sqErrorMacro(cerr,"Could not open " << fileName << ".");
        goto RANK_0_PARSE_ERROR;
        }
      // expect the bricks to be in the same directory
      this->FileName=fileName;
      this->PathToBricks=StripFileNameFromPath(fileName);

      // Read
      metaFile.seekg(0,ios::end);
      size_t metaFileLen=metaFile.tellg();
      metaFile.seekg(0,ios::beg);
      char *metaDataBuffer=static_cast<char *>(malloc(metaFileLen+1));
      metaDataBuffer[metaFileLen]='\0';
      metaFile.read(metaDataBuffer,metaFileLen);
      string metaData(metaDataBuffer);
      free(metaDataBuffer); // TODO use string's memory directly

      // Parse
      //ToLower(metaData);

      int nx,ny,nz;
      if ( ParseValue(metaData,0,"nx=",nx)==string::npos
        || ParseValue(metaData,0,"ny=",ny)==string::npos
        || ParseValue(metaData,0,"nz=",nz)==string::npos )
        {
        sqErrorMacro(cerr,
             << "Parsing " << fileName
             << " dimensions not found. Expected nx=N, ny=M, nz=P.");
        goto RANK_0_PARSE_ERROR;
        }

      // strategy:
      // cell centered data on disk needs to be
      // put on a point centered grid, we reduce
      // by one in x and y, shift and duplicate a
      // slab in z for each piece.
      this->FileExtent.Set(0,nx-1,0,ny-1,0,nz-1);
      this->MemoryExtent.Set(this->FileExtent);
      this->MemoryExtent[1]-=1;
      this->MemoryExtent[3]-=1;

      cerr << "FileExtent=" << this->FileExtent << endl;
      cerr << "MemoryExtent=" << this->MemoryExtent << endl;

      int nSteps;
      if (ParseValue(metaData,0,"nsteps=",nSteps)==string::npos)
        {
        // if no steps are provided assume 1
        nSteps=1;
        }
      this->NTimeSteps=1;

      string scalar;
      size_t at=0;
      ParseValue(metaData,at,"scalar:",scalar);
      if (at==string::npos)
        {
        sqErrorMacro(cerr,"scalar not present in metadata.");
        goto RANK_0_PARSE_ERROR;
        }
      this->ScalarName=scalar;

      // push the metadata
      BinaryStream str;
      this->Pack(str);
      int nBytes=str.GetSize();
      MPI_Bcast(&nBytes,1,MPI_INT,0,MPI_COMM_WORLD);
      MPI_Bcast(str.GetData(),nBytes,MPI_CHAR,0,MPI_COMM_WORLD);
      return 0;
      }
    else
      {
      // recv the metadata
      int nBytes;
      MPI_Bcast(&nBytes,1,MPI_INT,0,MPI_COMM_WORLD);
      if (nBytes>0)
        {
        BinaryStream str;
        str.Resize(nBytes);
        MPI_Bcast(str.GetData(),nBytes,MPI_CHAR,0,MPI_COMM_WORLD);
        this->UnPack(str);
        return 0;
        }
      else
        {
        return -1;
        }
      }

    return 0;

RANK_0_PARSE_ERROR:
    // this prevents a deadlock in the case that the
    // file is invalid , does not exist , or other
    // i/o error occurs.
    int nBytes=0;
    MPI_Bcast(&nBytes,1,MPI_INT,0,MPI_COMM_WORLD);
    return -1;
  }

  void Close()
  {
    this->FileName="";
    this->PathToBricks="";
    this->BrickExtension="";
    this->ScalarName="";
    this->FileExtent.Clear();
    this->NTimeSteps=0;
  }

  const char *GetFileName(){ return this->FileName.c_str(); }
  const char *GetPathToBricks(){ return this->PathToBricks.c_str(); }
  const char *GetBrickExtension(){ return this->BrickExtension.c_str(); }
  const char *GetScalarName(){ return this->ScalarName.c_str(); }
  CartesianExtent &GetFileExtent(){ return this->FileExtent; }
  CartesianExtent &GetMemoryExtent(){ return this->MemoryExtent; }
  int GetNumberOfTimeSteps(){ return this->NTimeSteps; }
  double GetTimeForStep(int i){ return (double)i; }

  void Print(ostream &os)
  {
    os
      << "FileName=" << this->FileName << endl
      << "PathToBricks=" << this->PathToBricks << endl
      << "BrickExtension=" << this->BrickExtension << endl
      << "ScalarName=" << this->ScalarName << endl
      << "FileExtent=" << this->FileExtent << endl
      << "MemoryExtent=" << this->MemoryExtent << endl;
  }

private:
  void Pack(BinaryStream &os)
  {
    os.Pack(this->FileName);
    os.Pack(this->PathToBricks);
    os.Pack(this->ScalarName);
    os.Pack(this->FileExtent.GetData(),6);
    os.Pack(this->MemoryExtent.GetData(),6);
    os.Pack(this->NTimeSteps);
  }

  void UnPack(BinaryStream &os)
  {
    os.UnPack(this->FileName);
    os.UnPack(this->PathToBricks);
    os.UnPack(this->ScalarName);
    os.UnPack(this->FileExtent.GetData(),6);
    os.UnPack(this->MemoryExtent.GetData(),6);
    os.UnPack(this->NTimeSteps);
  }

private:
  int WorldRank;
  int WorldSize;
  string FileName;          // Path to meta data file
  string PathToBricks;      // Path to brick files
  string BrickExtension;    // file extension for bricks
  string ScalarName;        //
  CartesianExtent FileExtent;   // cell extent of the entire dataset as it is on disk
  CartesianExtent MemoryExtent; // cell extent of the entire dataset as it is in memory
  int NTimeSteps;           //
};

#endif

