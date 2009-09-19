#ifndef ArrayReader_hxx
#define ArrayReader_hxx

#include "mpi.h"

//=============================================================================
template<typename T> class DataTraits;

//=============================================================================
template <>
class DataTraits<float>
{
public:
  static MPI_Datatype Type(){ 
    return MPI_FLOAT;
    }
};

//=============================================================================
template <>
class DataTraits<double>
{
public:
  static MPI_Datatype Type(){
    return MPI_DOUBLE;
    }
};

//*****************************************************************************
template<typename T>
MPI_Status WriteDataArray(
        const char *fileName,    // File name to write.
        MPI_Comm &comm,          // MPI communicator handle
        const vtkAMRBox &domain, // entire region, dataset extents
        const vtkAMRBox &decomp, // region to be wrote, block extents
        T *data)                 // pointer to a buffer to write to disk.
{
  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};
  // Open the file
  MPI_File file;
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(fileName),
      MPI_MODE_RDONLY,
      MPI_INFO_NULL,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    #ifndef NDEBUG
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error opeing file: " << fileName << endl;
    cerr << eStr << endl;
    #endif
    return 0;
    }

  // Locate our data.
  int domainDims[3];
  domain.GetNumberOfCells(domainDims);
  int decompDims[3];
  decomp.GetNumberOfCells(decompDims);
  int decompStart[3];
  decomp.GetLoCorner(decompStart);

  unsigned long long nCells=decomp.GetNumberOfCells();

  // Create the subarray
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype subarray;
  MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &subarray);
  MPI_Type_commit(&subarray);
  // Set the file view
  MPI_File_set_view(
      file,
      0,
      nativeType,
      subarray,
      "native", // FIXME option to select, portable or native.
      MPI_INFO_NULL); // FIXME make use of hints.
  // Write
  MPI_Status ok;
  iErr=MPI_File_write_all(file,data,nCells,nativeType,&ok);
  MPI_File_close(&file);
  MPI_Type_free(&subarray);
  if (iErr!=MPI_SUCCESS)
    {
    #ifndef NDEBUG
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error reading file: " << fileName << endl;
    cerr << eStr << endl;
    #endif
    return 0;
    }

  return ok;
}

//*****************************************************************************
template <typename T>
int ReadDataArray(
        const char *fileName,    // File name to read.
        MPI_Comm &comm,          // MPI communicator handle
        const vtkAMRBox &domain, // entire region, dataset extents
        const vtkAMRBox &decomp, // region to be read, block extents
        T *data)                 // pointer to a buffer to read into.
{
  //  clock_t start=clock(); 
  //  int nProcs=1;
  //  MPI_Comm_size(comm,&nProcs);
  //  int rank=0;
  //  MPI_Comm_rank(comm,&rank);

  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};
  // Open the file
  MPI_File file;
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(fileName),
      MPI_MODE_RDONLY,
      MPI_INFO_NULL,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    #ifndef NDEBUG
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error opeing file: " << fileName << endl;
    cerr << eStr << endl;
    #endif
    return 0;
    }

  // if (rank==0) 
  //   {
  //   MPI_Info info;
  //   char key[MPI_MAX_INFO_KEY];
  //   char val[MPI_MAX_INFO_VAL];
  //   MPI_File_get_info(file,&info);
  //   int nKeys;
  //   MPI_Info_get_nkeys(info,&nKeys);
  //   for (int i=0; i<nKeys; ++i)
  //     {
  //     int flag;
  //     MPI_Info_get_nthkey(info,i,key);
  //     MPI_Info_get(info,key,MPI_MAX_INFO_VAL,val,&flag);
  //     cerr << key << "=" << val << endl;
  //     }
  //   }

  // Locate our data.
  int domainDims[3];
  domain.GetNumberOfCells(domainDims);
  int decompDims[3];
  decomp.GetNumberOfCells(decompDims);
  int decompStart[3];
  decomp.GetLoCorner(decompStart);

  unsigned long long nCells=decomp.GetNumberOfCells();

  // Create the subarray
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype subarray;
  MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &subarray);
  MPI_Type_commit(&subarray);
  // Set the file view
  MPI_File_set_view(
      file,
      0,
      nativeType,
      subarray,
      "native", // FIXME option to select, portable or native.
      MPI_INFO_NULL); // FIXME make use of hints.
  // Read
  MPI_Status status;
  iErr=MPI_File_read_all(file,data,nCells,nativeType,&status);
  MPI_File_close(&file);
  MPI_Type_free(&subarray);
  if (iErr!=MPI_SUCCESS)
    {
    #ifndef NDEBUG
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error reading file: " << fileName << endl;
    cerr << eStr << endl;
    #endif
    return 0;
    }
  //  clock_t end=clock();
  //  char hostname[HOST_NAME_MAX];
  //  gethostname(hostname,HOST_NAME_MAX);
  //  hostname[4]='\0';
  //  double elapsed=static_cast<double>((end-start))/CLOCKS_PER_SEC;
  //  cerr << hostname << " " << elapsed << endl; 

  return 1;
}

#endif
