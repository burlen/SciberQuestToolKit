#include "BOVReader.h"
#include "BOVTimeStepImage.h"

#include <sstream>
using namespace std;

#include "vtkAlgorithm.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPI.h"

#include "MPIRawArrayIO.hxx"

#ifdef WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

#include "PrintUtils.h"

//-----------------------------------------------------------------------------
const BOVReader &BOVReader::operator=(const BOVReader &other)
{
  if (this==&other)
    {
    return *this;
    }

  this->Blocks=other.Blocks;
  this->Comm=other.Comm;
  this->NGhost=other.NGhost;
  this->NProcs=other.NProcs;
  this->ProcId=other.ProcId;

  this->SetMetaData(other.GetMetaData());

  return *this;
}

//-----------------------------------------------------------------------------
void BOVReader::Clear()
{
  // TODO what about MetaData??
  this->ClearDecomp();
  // this->ClearArraysToRead();
  this->ProcId=0;
  this->NProcs=1;
  this->NGhost=1;
  this->Comm=MPI_COMM_NULL;
}

//-----------------------------------------------------------------------------
void BOVReader::SetMetaData(const BOVMetaData *metaData)
{
  if (this->MetaData!=metaData)
    {
    if (this->MetaData)
      {
      delete this->MetaData;
      this->MetaData=NULL;
      }
    if (metaData)
      {
      this->MetaData=metaData->Duplicate();
      }
    }
}

//-----------------------------------------------------------------------------
int BOVReader::SetController(vtkMultiProcessController *cont)
{
  // Extract information re: our running environment
  this->ProcId=cont->GetLocalProcessId();
  this->NProcs=cont->GetNumberOfProcesses();
  this->Comm=MPI_COMM_NULL;

  vtkMPICommunicator *vcom
    = dynamic_cast<vtkMPICommunicator *>(cont->GetCommunicator());
  if (!vcom)
    {
    #ifndef NDEBUG
    cerr << __LINE__ 
         << " Error: No communicator available, MPI is required."
         << endl;
    #endif
    // User is running builtin, we are going to start MPI ourselves.
    // This may or may not be a good idea depending on MPI implementation.
    int argc=1;
    char *app="vtkBOVReader";
    char *argv[1]={app};
    MPI_Init(&argc,(char ***)&argv);
    this->Comm=MPI_COMM_WORLD;
    return 0;
    }
  // this->Comm=*vcom->GetMPIComm()->GetHandle();
  this->Comm=MPI_COMM_WORLD;

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadScalarArray(BOVScalarImageIterator *it, vtkImageData *grid)
{
  const vtkAMRBox &block=this->MetaData->GetDecomp();
  const size_t nCells=block.GetNumberOfCells();
  // Create a VTK array and insert it into the point data.
  // assume we have real*4, and that it's no already cached.  FIXME!!!
  vtkFloatArray *fa=vtkFloatArray::New();
  fa->SetNumberOfComponents(1);
  fa->SetNumberOfTuples(nCells); // dual grid
  fa->SetName(it->GetName());
  grid->GetPointData()->AddArray(fa);
  fa->Delete();
  float *pfa=fa->GetPointer(0);
  // read
  return ReadDataArray(it->GetFile(),this->MetaData->GetDomain(),block,pfa);
}

//-----------------------------------------------------------------------------
int BOVReader::ReadVectorArray(BOVVectorImageIterator *it, vtkImageData *grid)
{
  const vtkAMRBox &block=this->MetaData->GetDecomp();
  const size_t nCells=block.GetNumberOfCells();

  // Create a VTK array.
  vtkFloatArray *fa=vtkFloatArray::New();
  fa->SetNumberOfComponents(3);
  fa->SetNumberOfTuples(nCells); // remember dual grid
  fa->SetName(it->GetName());
  grid->GetPointData()->AddArray(fa);
  fa->Delete();
  float *pfa=fa->GetPointer(0);

  // Allocate a buffer for reads
  float *xbuf=static_cast<float *>(malloc(nCells*sizeof(float)));
  float *ybuf=static_cast<float *>(malloc(nCells*sizeof(float)));
  float *zbuf=static_cast<float *>(malloc(nCells*sizeof(float)));

  // Read the block.
  int ok=-1;
  ok&=ReadDataArray(it->GetXFile(),this->MetaData->GetDomain(),block,xbuf);
  ok&=ReadDataArray(it->GetYFile(),this->MetaData->GetDomain(),block,ybuf);
  ok&=ReadDataArray(it->GetZFile(),this->MetaData->GetDomain(),block,zbuf);
  if (!ok)
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: ReadDataArray failed." << endl;
    #endif
    return 0;
    }

  // TODO make sure this is vectorized!!
  // Copy the vector components into the VTK array.
  float *pxbuf=xbuf;
  float *pybuf=ybuf;
  float *pzbuf=zbuf;
  for (size_t i=0; i<nCells; ++i)
    {
    *pfa=*pxbuf; ++pfa; ++pxbuf;
    *pfa=*pybuf; ++pfa; ++pybuf;
    *pfa=*pzbuf; ++pfa; ++pzbuf;
    }
  // Free up the buffers
  free(xbuf);
  free(ybuf);
  free(zbuf);

  return 1;
}

//-----------------------------------------------------------------------------
BOVTimeStepImage *BOVReader::OpenTimeStep(int stepNo)
{
  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Cannot opena timestep because the dataset is not open." << endl;
    #endif
    return 0;
    }
  //BOVTimeStepImage *image=new BOVTimeStepImage(this->Comm,stepNo,this->GetMetaData());
  //cerr << *image << endl;
  //return image;
  return new BOVTimeStepImage(this->Comm,stepNo,this->GetMetaData());
}

//-----------------------------------------------------------------------------
void BOVReader::CloseTimeStep(BOVTimeStepImage *handle)
{
  delete handle;
  handle=0;
}


//-----------------------------------------------------------------------------
int BOVReader::ReadTimeStep(BOVTimeStepImage *step, vtkImageData *grid, vtkAlgorithm *alg)
{
  if (!step)
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Null step image received. Aborting." << endl;
    #endif
    return 0;
    }
  if (!grid)
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Null output dataset received. Aborting." << endl; 
    #endif
    return 0;
    }

  double progInc=0.70/step->GetNumberOfImages();
  double prog=0.25;
  if(alg)alg->UpdateProgress(prog);

  // scalars
  BOVScalarImageIterator sIt(step);
  for (;sIt.Ok(); sIt.Next())
    {
    int ok=this->ReadScalarArray(&sIt,grid);
    if (!ok)
      {
      #ifndef NDEBUG
      cerr << __LINE__ << " Failed to read scalar array " << sIt.GetName() << "." << endl;
      #endif
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }
  // vectors
  BOVVectorImageIterator vIt(step);
  for (;vIt.Ok(); vIt.Next())
    {
    int ok=this->ReadVectorArray(&vIt,grid);
    if (!ok)
      {
      #ifndef NDEBUG
      cerr << __LINE__ << " Failed to read vector array " << vIt.GetName() << "." << endl;
      #endif
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadMetaTimeStep(int stepIdx, vtkImageData *grid, vtkAlgorithm *alg)
{
  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Cannot read because the dataset is not open." << endl;
    #endif
    return 0;
    }

  if (grid)
    {
    ostringstream seriesExt;
    seriesExt << "_" << stepIdx << "." << this->MetaData->GetBrickFileExtension();
    // In a meta read we won't read the arrays, rather we'll put a place holder
    // for each in the output. Downstream array can then be selected.
    const size_t nPoints=grid->GetNumberOfPoints();
    const size_t nArrays=this->MetaData->GetNumberOfArrays();
    double progInc=0.75/nArrays;
    double prog=0.25;
    if(alg)alg->UpdateProgress(prog);
    for (size_t i=0; i<nArrays; ++i)
      {
      const char *arrayName=this->MetaData->GetArrayName(i);
      // skip inactive arrays
      if (!this->MetaData->IsArrayActive(arrayName))
        {
        prog+=progInc;
        if(alg)alg->UpdateProgress(prog);
        continue;
        }
      vtkFloatArray *array=vtkFloatArray::New();
      array->SetName(arrayName);
      // scalar
      if (this->MetaData->IsArrayScalar(arrayName))
        {
        array->SetNumberOfTuples(nPoints);
        array->FillComponent(0,-5.5);
        }
      // vector
      else
      if (this->MetaData->IsArrayVector(arrayName))
        {
        array->SetNumberOfComponents(3);
        array->SetNumberOfTuples(nPoints);
        array->FillComponent(0,-5.5);
        array->FillComponent(1,-5.5);
        array->FillComponent(2,-5.5);
        }
      // other ?
      else
        {
        #ifndef NDEBUG
        cerr << __LINE__ << " Error: bad array type for array " << arrayName << "." << endl; 
        #endif
        }
      grid->GetPointData()->AddArray(array);
      array->Delete();

      prog+=progInc;
      if(alg)alg->UpdateProgress(prog);
      }
    }
  else
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Empty VTK dataset passed, aborting." << endl; 
    #endif
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void BOVReader::Print(ostream &os)
{
  os << "BOVReader: " << this << endl;
  os << "\tBlocks: " << endl << this->Blocks << endl;
  os << "\tComm: " << this->Comm << endl;
  os << "\tNGhost: " << this->NGhost << endl;
  os << "\tProcId: " << this->ProcId << endl;
  os << "\tNProcs: " << this->ProcId << endl;

  this->MetaData->Print(os);
}

// // Simple domian decomposition, returns nProc slabs in laregst dimension.
// // If nProcs is larger than the number of cells in the largest dimension
// // of the domian then a number of boxes in the decomposition will be empty.
// //*****************************************************************************
// int SlabDecomp(
//       const int nProcs,
//       const vtkAMRBox &domain,
//       const int nGhost,
//       vector<vtkAMRBox> &decomp)
// {
//   int nCells[3]={0};
//   domain.GetNumberOfCells(nCells);
// 
//   // Assume this is in k direction, correct if it is not.
//   int splitIdx=2;
//   if (nCells[0]>=nCells[1] && nCells[0]>=nCells[2]) // i
//     {
//     splitIdx=0;
//     }
//   else
//   if (nCells[1]>=nCells[0] && nCells[1]>=nCells[2]) // j
//     {
//     splitIdx=1;
//     }
// 
//   int OK=1;
//   if (nProcs>nCells[splitIdx])
//     {
//     #ifndef NDEBUG
//     cerr << __LINE__ << " Warning: empty domains in decompositon." << endl;
//     #endif
//     OK=0;
//     }
// 
//   int side=nCells[splitIdx]/nProcs;
//   int nLarge=nCells[splitIdx]%nProcs; // These have width side+1
//   int nRegular=nProcs-nLarge;         // These have width side
// 
//   // Compute first slab, to get the process started
//   int slabLo[3];
//   int slabHi[3]; 
//   domain.GetDimensions(slabLo,slabHi);
//   slabLo[splitIdx]=slabLo[splitIdx];
//   slabHi[splitIdx]=slabLo[splitIdx]+side-1;
//   for (int i=0; i<nProcs; ++i)
//     {
//     // If this is a larger slab, then add one cell to width.
//     if (i>=nRegular)
//       {
//       slabHi[splitIdx]+=1;
//       }
//     // Make the slab.
//     vtkAMRBox slab(slabLo,slabHi);
//     slab.SetDataSetOrigin(domain.GetDataSetOrigin());
//     slab.SetGridSpacing(domain.GetGridSpacing());
//     slab.Grow(nGhost);
//     // Trim the ghosts, if they are outsde the domain.
//     slab&=domain;
//     // Save it.
//     decomp.push_back(slab);
//     // Compute next slab.
//     slabLo[splitIdx]=slabHi[splitIdx]+1;
//     slabHi[splitIdx]=slabLo[splitIdx]+side-1;
//     }
//   return OK;
// }

// //-----------------------------------------------------------------------------
// int BOVReader::DecomposeDomain(vtkMultiBlockDataSet *mbds)
// {
//   if (!this->MetaData)
//     {
//     #ifndef NDEBUG
//     cerr << __LINE__ << " Error: No meta data." << endl;
//     #endif
//     return 0;
//     }
// 
//   this->ClearDecomp();
// 
//   // Subset the domain.
//   vtkAMRBox subset=this->MetaData->GetSubset();
//   subset&=this->MetaData->GetDomain();
//   // Decompose the subset.
//   int status=SlabDecomp(this->NProcs, subset, this->NGhost, this->Blocks);
//   // Configure the vtk object.
//   mbds->SetNumberOfBlocks(this->NProcs);
//   if (!this->Blocks[this->ProcId].Empty())
//     {
//     const vtkAMRBox &block=this->Blocks[this->ProcId];
//     vtkImageData *id=vtkImageData::New();
//     double X0[3]; 
//     block.GetBoxOrigin(X0);
//     id->SetOrigin(X0);
//     double DX[3];
//     block.GetGridSpacing(DX);
//     id->SetSpacing(DX);
//     int nCells[3];
//     block.GetNumberOfCells(nCells);
//     id->SetDimensions(nCells); // We are using the dual grid.
//     mbds->SetBlock(this->ProcId,id);
//     id->Delete();
//     }
// 
//   return status;
// }
// 

// //-----------------------------------------------------------------------------
// int BOVReader::ReadScalarArray(
//         const char *fileName,
//         const char *scalarName,
//         vtkImageData *grid)
// {
//   if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
//     {
//     #ifndef NDEBUG
//     cerr << __LINE__ << " Error: Cannot read because the dataset is not open." << endl;
//     #endif
//     return 0;
//     }
//   const vtkAMRBox &block=this->MetaData->GetDecomp();
//   const size_t nCells=block.GetNumberOfCells();
//   // Create a VTK array and insert it into the point data.
//   // assume we have real*4, and that it's no already cached.  FIXME!!!
//   vtkFloatArray *fa=vtkFloatArray::New();
//   fa->SetNumberOfComponents(1);
//   fa->SetNumberOfTuples(nCells); // dual grid
//   fa->SetName(scalarName);
//   grid->GetPointData()->AddArray(fa);
//   fa->Delete();
//   float *pfa=fa->GetPointer(0);
//   // Open the file (if not cached).
//   MPI_File file=this->ArrayFileCache.Open(this->Comm,fileName);
//   // Read the block.
//   return
//     ReadDataArray(file,this->MetaData->GetDomain(),block,pfa);
//   // File is close by the cache.
// }
// 
// //-----------------------------------------------------------------------------
// int BOVReader::ReadVectorArray(
//     const char *xFileName,
//     const char *yFileName,
//     const char *zFileName,
//     const char *vectorName,
//     vtkImageData *grid)
// {
//   if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
//     {
//     #ifndef NDEBUG
//     cerr << __LINE__ << " Error: Cannot read because the dataset is not open." << endl;
//     #endif
//     return 0;
//     }
// 
//   const vtkAMRBox &block=this->MetaData->GetDecomp();
//   const size_t nCells=block.GetNumberOfCells();
//   // Create a VTK array and insert it into the cached data.
//   // assume we have real*4, and that it's no already cached.  FIXME!!!
//   vtkFloatArray *fa=vtkFloatArray::New();
//   fa->SetNumberOfComponents(3);
//   fa->SetNumberOfTuples(nCells); // remember dual grid
//   fa->SetName(vectorName);
//   grid->GetPointData()->AddArray(fa);
//   fa->Delete();
//   float *pfa=fa->GetPointer(0);
//   // Allocate a buffer for reads
//   float *xbuf=static_cast<float *>(malloc(nCells*sizeof(float)));
//   float *ybuf=static_cast<float *>(malloc(nCells*sizeof(float)));
//   float *zbuf=static_cast<float *>(malloc(nCells*sizeof(float)));
// 
//   int ok=-1;
//   // Open the file (if not cached).
//   // Read the block.
//   // DON'T close the file! The cache handles it for us.
//   MPI_File xFile=this->ArrayFileCache.Open(this->Comm,xFileName);
//   ok&=ReadDataArray(xFile,this->MetaData->GetDomain(),block,xbuf);
// 
//   MPI_File yFile=this->ArrayFileCache.Open(this->Comm,yFileName);
//   ok&=ReadDataArray(yFile,this->MetaData->GetDomain(),block,ybuf);
// 
//   MPI_File zFile=this->ArrayFileCache.Open(this->Comm,zFileName);
//   ok&=ReadDataArray(zFile,this->MetaData->GetDomain(),block,zbuf);
// 
//   if (!ok)
//     {
//     #ifndef NDEBUG
//     cerr << __LINE__ << " Error: ReadDataArray failed." << endl;
//     #endif
//     return 0;
//     }
// 
//   // TODO make sure this is vectorized!!
//   // Copy the vector components into the VTK array.
//   float *pxbuf=xbuf;
//   float *pybuf=ybuf;
//   float *pzbuf=zbuf;
//   for (size_t i=0; i<nCells; ++i)
//     {
//     *pfa=*pxbuf; ++pfa; ++pxbuf;
//     *pfa=*pybuf; ++pfa; ++pybuf;
//     *pfa=*pzbuf; ++pfa; ++pzbuf;
//     }
//   // Free up the buffers
//   free(xbuf);
//   free(ybuf);
//   free(zbuf);
// 
//   return 1;
// }
