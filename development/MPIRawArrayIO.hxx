/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
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
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error opeing file: " << fileName << endl;
    cerr << eStr << endl;
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

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  MPI_Type_commit(&fileView);
  MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native", // FIXME option to select, portable or native.
      MPI_INFO_NULL); // FIXME make use of hints.

  // memory view
  MPI_Datatype memView;
  MPI_Type_contiguous(nCells,nativeType,&memView);
  MPI_Type_commit(&memView);


  // Write
  MPI_Status ok;
  iErr=MPI_File_write_all(file,data,1,memView,&ok);
  MPI_File_close(&file);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error writing file: " << fileName << endl;
    cerr << eStr << endl;
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
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error opeing file: " << fileName << endl;
    cerr << eStr << endl;
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

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  MPI_Type_commit(&fileView);
  MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native", // FIXME option to select, portable or native.
      MPI_INFO_NULL); // FIXME make use of hints.

  // memory view
  MPI_Datatype memView;
  MPI_Type_contiguous(nCells,nativeType,&memView);
  MPI_Type_commit(&memView);

  // Read
  MPI_Status status;
  iErr=MPI_File_read_all(file,data,1,memView,&status);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  MPI_File_close(&file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error reading file: " << fileName << endl;
    cerr << eStr << endl;
    return 0;
    }

  return 1;
}

//*****************************************************************************
template <typename T>
int ReadDataArray(
        MPI_File file,
        const vtkAMRBox &domain, // entire region, dataset extents
        const vtkAMRBox &decomp, // region to be read, block extents
        T *data)                 // pointer to a buffer to read into.
{
  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};

  // Locate our data.
  int domainDims[3];
  domain.GetNumberOfCells(domainDims);
  int decompDims[3];
  decomp.GetNumberOfCells(decompDims);
  int decompStart[3];
  decomp.GetLoCorner(decompStart);

  unsigned long long nCells=decomp.GetNumberOfCells();

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  MPI_Type_commit(&fileView);
  MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native", // TODO option to select, portable or native.
      MPI_INFO_NULL); // TODO could make better use of hints.

  // memory view
  MPI_Datatype memView;
  MPI_Type_contiguous(nCells,nativeType,&memView);
  MPI_Type_commit(&memView);

  // Read
  MPI_Status status;
  iErr=MPI_File_read_all(file,data,1,memView,&status);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error reading file." << endl;
    cerr << eStr << endl;
    return 0;
    }

  return 1;
}

#endif
