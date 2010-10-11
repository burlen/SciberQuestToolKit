/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkSmartPointer.h"

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkXMLPDataSetWriter.h"

#include "SQMacros.h"
#include "postream.h"
#include "XMLUtils.h"
#include "vtkSQBOVReader.h"
#include "vtkSQFieldTracer.h"
#include "vtkSQPlaneSource.h"
#include "vtkSQVolumeSource.h"
#include "vtkSQHemisphereSource.h"

//#include "vtkPointData.h"
//#include "vtkCellData.h"
//#include "vtkPolyData.h"

#include <fstream>
using std::ofstream;
using std::ifstream;
#include <sstream>
//using std::istringstream;
using std::ostringstream;
#include <iostream>
using std::cerr;
using std::endl;
#include <iomanip>
using std::setfill;
using std::setw;
#include <vector>
using std::vector;
#include <string>
using std::string;

#include <mpi.h>

#define SQ_EXIT_ERROR 1
#define SQ_EXIT_SUCCESS 0

//*****************************************************************************
int IndexOf(double value, double *values, int first, int last)
{
  int mid=(first+last)/2;
  if (values[mid]==value)
    {
    return mid;
    }
  else
  if (mid!=first && values[mid]>value)
    {
    return IndexOf(value,values,first,mid-1);
    }
  else
  if (mid!=last && values[mid]<value)
    {
    return IndexOf(value,values,mid+1,last);
    }
  return -1;
}


/**
For post processing use with MagnetosphereTopoBatch.
Generates a PVD file over the specified time range. If any
constituent files are missing then a warning is printed.
*/
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  int worldSize=1;
  int worldRank=0;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&worldSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&worldRank);

  if (worldRank==0)
    {
    if (argc<4)
      {
      cerr
        << "Error: Command tail." << endl
        << " 1) /path/to/runConfig.xml" << endl
        << " 2) startTime" << endl
        << " 3) endTime" << endl
        << endl;
      return SQ_EXIT_ERROR;
      }
    }
  else
    {
    // only rank 0 does anything.
    MPI_Finalize();
    return SQ_EXIT_SUCCESS;
    }

  char *configName=argv[1];
  double startTime=atof(argv[2]);
  double endTime=atof(argv[3]);

  // read the configuration file.
  int iErr=0;
  vtkPVXMLElement *elem=0;
  vtkSmartPointer<vtkPVXMLParser> parser=vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(configName);
  if (parser->Parse()==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }

  // check for the semblance of a valid configuration hierarchy
  vtkPVXMLElement *root=parser->GetRootElement();
  if (root==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }

  string requiredType("MagnetosphereTopologyBatch");
  const char *foundType=root->GetName();
  if (foundType==0 || foundType!=requiredType)
    {
    sqErrorMacro(pCerr(),
        << "This is not a valid " << requiredType
        << " XML hierarchy.");
    return SQ_EXIT_ERROR;
    }

  // reader
  iErr=0;
  elem=GetRequiredElement(root,"vtkSQBOVReader");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  const char *bovFileName;
  iErr=GetRequiredAttribute(elem,"bov_file_name",&bovFileName);
  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(1);
  r->SetFileName(bovFileName);

  // querry available times from the reader.
  int nTimes=r->GetNumberOfTimeSteps();
  vector<double> times(nTimes);
  r->GetTimeSteps(&times[0]);
  r->Delete();

  int startTimeIdx=IndexOf(startTime,&times[0],0,nTimes-1);
  if (startTimeIdx<0)
    {
    sqErrorMacro(pCerr(),"Invalid start time " << startTimeIdx << ".");
    return SQ_EXIT_ERROR;
    }

  int endTimeIdx=IndexOf(endTime,&times[0],0,nTimes-1);
  if (endTimeIdx<0)
    {
    sqErrorMacro(pCerr(),"Invalid end time " << endTimeIdx << ".");
    return SQ_EXIT_ERROR;
    }

  // seed source for constituent file extention
  const char *outFileExt;
  if ((elem=GetOptionalElement(root,"vtkSQPlaneSource"))!=NULL)
    {
    // 2D source
    outFileExt=".pvtp";
    }
  else
  if ((elem=GetOptionalElement(root,"vtkSQVolumeSource"))!=NULL)
    {
    // 3D source
    outFileExt=".pvtu";
    }
  else
    {
    sqErrorMacro(pCerr(),"No seed source found.");
    return SQ_EXIT_ERROR;
    }

  // writer.
  iErr=0;
  elem=GetRequiredElement(root,"vtkXMLPDataSetWriter");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  const char *outFileBase;
  iErr=GetRequiredAttribute(elem,"out_file_base",&outFileBase);
  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }


  // write pvd file
  string pvdFileName;
  pvdFileName+=outFileBase;
  pvdFileName+=".pvd";

  ofstream pvds;
  pvds.open(pvdFileName.c_str());
  if (!pvds.good())
    {
    sqErrorMacro(pCerr(),"Failed to open " << pvdFileName << ".");
    return SQ_EXIT_ERROR;
    }

  pvds
    << "<?xml version=\"1.0\"?>" << endl
    << "<VTKFile type=\"Collection\">" << endl
    << "<Collection>" << endl;

  for (int idx=startTimeIdx,q=0; idx<=endTimeIdx; ++idx,++q)
    {
    double time=times[idx];

    ostringstream fns;
    fns
      << outFileBase << "_"
      << setfill('0') << setw(8) << time
      << outFileExt;

    string fn=fns.str();

    pvds
      << "<DataSet "
      << "timestep=\"" << time << "\" "
      << "group=\"\" part=\"0\" "
      << "file=\"" << fn.c_str() << "\""
      << "/>"
      << endl;

    // sanity - presumably the constituent files have been generated
    ifstream cfs;
    cfs.open(fn.c_str());
    if (!cfs.good())
      {
      sqErrorMacro(pCerr(),
        << "Warning: Failed to open constituent file : " << fn << ".");
      }
    else
      {
      cfs.close();
      }
    }

  pvds
    << "</Collection>" << endl
    << "</VTKFile>" << endl
    << endl;
  pvds.close();

  MPI_Finalize();
  return SQ_EXIT_SUCCESS;
}
