/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "vtkSQImageGhosts.h"

#include "CartesianExtent.h"
#include "postream.h"
#include "Numerics.hxx"
#include "MPIRawArrayIO.hxx"


#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/string>
using vtkstd::string;

#include <mpi.h>

#define vtkSQImageGhostsDEBUG

//=============================================================================
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
  int Execute(int rank, int nComps, T *srcData, T *destData);

private:
  int SrcRank;
  CartesianExtent SrcExt;

  int DestRank;
  CartesianExtent DestExt;

  CartesianExtent IntExt;
};

//-----------------------------------------------------------------------------
template<typename T>
int GhostTransaction::Execute(int rank, int nComps, T *srcData, T *destData)
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

//*****************************************************************************
ostream &operator<<(ostream &os, const GhostTransaction &gt)
{
  os
    << "(" << gt.GetSourceRank() << ", " << gt.GetSourceExtent() << ")->"
    << "(" << gt.GetDestinationRank() << ", " << gt.GetDestinationExtent() << ") "
    << gt.GetIntersectionExtent();
  return os;
}


vtkCxxRevisionMacro(vtkSQImageGhosts, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQImageGhosts);

//-----------------------------------------------------------------------------
vtkSQImageGhosts::vtkSQImageGhosts()
    :
  WorldSize(1),
  WorldRank(0),
  NGhosts(0),
  Mode(MODE_3D)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "===============================vtkSQImageGhosts::vtkSQImageGhosts" << endl;
  #endif

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    vtkErrorMacro("MPI has not been initialized. Restart ParaView using mpiexec.");
    }

  MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);


  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQImageGhosts::~vtkSQImageGhosts()
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "===============================vtkSQImageGhosts::~vtkSQImageGhosts" << endl;
  #endif
}

//-----------------------------------------------------------------------------
CartesianExtent vtkSQImageGhosts::Grow(const CartesianExtent &inputExt)
{
  CartesianExtent outputExt(inputExt);

  switch(this->Mode)
  {
  case MODE_2D_XY:
    outputExt.Grow(0,this->NGhosts);
    outputExt.Grow(1,this->NGhosts);
    break;
  case MODE_2D_XZ:
    outputExt.Grow(0,this->NGhosts);
    outputExt.Grow(2,this->NGhosts);
    break;
  case MODE_2D_YZ:
    outputExt.Grow(1,this->NGhosts);
    outputExt.Grow(2,this->NGhosts);
    break;
  case MODE_3D:
    outputExt.Grow(this->NGhosts);
    break;
  default:
    vtkErrorMacro("Invalid mode " << this->Mode << ".");
  }

  outputExt &= this->ProblemDomain;

  return outputExt;
}

//-------------------------------------------------------------------------
int vtkSQImageGhosts::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // We require preceding filters to refrain from creating ghost cells.

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = 0;

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}


// //-----------------------------------------------------------------------------
// int vtkSQImageGhosts::FillInputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   #ifdef vtkSQImageGhostsDEBUG
//   pCerr() << "===============================vtkSQImageGhosts::FillInputPortInformation" << endl;
//   #endif
//
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }
//
// //-----------------------------------------------------------------------------
// int vtkSQImageGhosts::FillOutputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   #ifdef vtkSQImageGhostsDEBUG
//   pCerr() << "===============================vtkSQImageGhosts::FillOutputPortInformation" << endl;
//   #endif
//
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }

// //-----------------------------------------------------------------------------
// int vtkSQImageGhosts::RequestDataObject(
//     vtkInformation* /* request */,
//     vtkInformationVector** inInfoVec,
//     vtkInformationVector* outInfoVec)
// {
//   #ifdef vtkSQImageGhostsDEBUG
//   pCerr() << "===============================vtkSQImageGhosts::RequestDataObject" << endl;
//   #endif
// 
// 
//   vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
//   vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
//   const char *inputType=inData->GetClassName();
// 
//   vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
//   vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());
// 
//   if ( !outData || !outData->IsA(inputType))
//     {
//     outData=inData->NewInstance();
//     outInfo->Set(vtkDataObject::DATA_TYPE_NAME(),inputType);
//     outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
//     outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), inData->GetExtentType());
//     outData->SetPipelineInformation(outInfo);
//     outData->Delete();
//     }
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkSQImageGhosts::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "===============================vtkSQImageGhosts::RequestInformation" << endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  this->NGhosts = outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  this->NGhosts = 1;

  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        this->ProblemDomain.GetData());

  // The problem domain is unchanged. We don't provide ghosts
  // outside of the problem domain, rather we expect downstream
  // filters to shrink the problem domain and use valid cells.
  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        this->ProblemDomain.GetData(),
        6);

  // Identify lower dimensional input and handle special cases.
  // Everything but 3D is a special case.
  int minExt = 2*this->NGhosts+1;
  int inExt[3];
  this->ProblemDomain.Size(inExt);
  // 0D and 1D are disallowed
  if ((inExt[0]<minExt) && (inExt[1]<minExt)
    ||(inExt[0]<minExt) && (inExt[2]<minExt)
    ||(inExt[1]<minExt) && (inExt[2]<minExt))
    {
    vtkErrorMacro("This filter does not support less than 2D.");
    return 1;
    }
  //  Identify 2D cases
  if (inExt[0]<minExt)
    {
    this->SetMode(MODE_2D_YZ);
    }
  else
  if (inExt[1]<minExt)
    {
    this->SetMode(MODE_2D_XZ);
    }
  else
  if (inExt[2]<minExt)
    {
    this->SetMode(MODE_2D_XY);
    }
  // It's 3D
  else
    {
    this->SetMode(MODE_3D);
    }

  #ifdef vtkSQImageGhostsDEBUG
  pCerr()
    << "NGhosts=" << this->NGhosts << endl
    << "Mode=" << this->Mode << endl
    << "WHOLE_EXTENT=" << this->ProblemDomain << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQImageGhosts::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "===============================vtkSQImageGhosts::RequestData" << endl;
  #endif

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataSet *inData
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataSet *outData
    = dynamic_cast<vtkDataSet*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Guard against empty input.
  if (!inData || !outData)
    {
    vtkErrorMacro(
      << "Empty input(" << inData << ") or output(" << outData << ") detected.");
    return 1;
    }

  // We need extent based data here.
  int isImage=inData->IsA("vtkImageData");
  int isRecti=0;//inData->IsA("vtkrectilinearGrid");
  if (!isImage && !isRecti)
    {
    vtkErrorMacro(
      << "This filter is designed for vtkImageData and subclasses."
      << "You are trying to use it with " << inData->GetClassName() << ".");
    return 1;
    }

  vtkImageData *outIm = dynamic_cast<vtkImageData*>(outData);

  // Get the input and output extents.
  CartesianExtent inputExt;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inputExt.GetData());

  CartesianExtent outputExt = this->Grow(inputExt);
  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData(),
        6);

  outIm->SetExtent(outputExt.GetData());

  // gather input extents
  vector<CartesianExtent> inputExts(this->WorldSize);
  int *buffer=new int [6*this->WorldSize];
  MPI_Allgather(
        inputExt.
        GetData(),
        6,
        MPI_INT,
        buffer,
        6,
        MPI_INT,
        MPI_COMM_WORLD);

  for (int i=0; i<this->WorldSize; ++i)
    {
    inputExts[i].Set(buffer+6*i);
    if (this->WorldRank==0)
      {
      cerr << "Rank" << i << " owns " << inputExts[i] << endl;
      }
    }
  delete [] buffer;

  // set up transactions
  vector<GhostTransaction> transactions;
  for (int i=0; i<this->WorldSize; ++i)
    {
    CartesianExtent destExt = this->Grow(inputExts[i]);

    for (int j=0; j<this->WorldSize; ++j)
      {
      if (i==j) continue;

      CartesianExtent srcExt = inputExts[j];
      CartesianExtent intExt = srcExt;
      intExt &= destExt;

      if (!intExt.Empty())
        {
        transactions.push_back(GhostTransaction(j,srcExt, i,destExt, intExt));
        if (this->WorldRank==0)
          {
          cerr << transactions.back() << endl;
          }
        }
      }
    }

  int nArrays = inData->GetPointData()->GetNumberOfArrays();
  int nTransactions = transactions.size();
  size_t nOutputTups = outputExt.Size();
  for (int i=0; i<nArrays; ++i)
    {
    vtkDataArray *inArray = inData->GetPointData()->GetArray(i);
    int nComps = inArray->GetNumberOfComponents();
    void *pIn = inArray->GetVoidPointer(0);

    vtkDataArray *outArray = inArray->NewInstance();
    outArray->SetName(inArray->GetName());
    outArray->SetNumberOfComponents(nComps);
    outArray->SetNumberOfTuples(nOutputTups);
    void *pOut = outArray->GetVoidPointer(0);

    // copy the valid data directly
      switch(inArray->GetDataType())
        {
        vtkTemplateMacro(
            Copy<VTK_TT>(
              inputExt.GetData(),
              outputExt.GetData(),
              (VTK_TT*)pIn,
              (VTK_TT*)pOut,
              nComps));
        }


    // execute the transactions to copy ghosts from remote processes
    for (int j=0; j<nTransactions; ++j)
      {
      GhostTransaction &trans = transactions[j];

      switch(inArray->GetDataType())
        {
        vtkTemplateMacro(
            trans.Execute<VTK_TT>(
              this->WorldRank,
              nComps,
              (VTK_TT*)pIn,
              (VTK_TT*)pOut));
        }
      }

    }


  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQImageGhosts::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "===============================vtkSQImageGhosts::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}


