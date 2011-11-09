/*=========================================================================

  Program:   ParaView
  Module:    vtkSC11DemoAnnotation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSC11DemoAnnotation.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include "PrintUtils.h"


#include <fstream>
using std::fstream;
#include <string>
using std::string;
#include <set>
using std::set;
#include <sstream>
using std::ostringstream;
#include <iomanip>
using std::setw;
using std::setfill;
using std::setprecision;
using std::right;
using std::left;
#include <algorithm>

#include <mpi.h>

vtkStandardNewMacro(vtkSC11DemoAnnotation);
//----------------------------------------------------------------------------
vtkSC11DemoAnnotation::vtkSC11DemoAnnotation()
        :
    WorldRank(0),
    WorldSize(1),
    BWComm(MPI_COMM_NULL),
    InBWComm(0),
    DummyFileListDomain(0),
    BWDataFile(0),
    TimeDataFile(0)
{
  this->SetNumberOfOutputPorts(2);

  MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);

  // gather host names
  const int hostNameLen=256;
  char hostName[hostNameLen]={'\0'};
  char *hostNames=0;
  gethostname(hostName,hostNameLen);
  if (this->WorldRank==0)
    {
    hostNames=(char*)malloc(hostNameLen*this->WorldSize);
    }
  MPI_Gather(
        hostName,
        hostNameLen,
        MPI_CHAR,
        hostNames,
        hostNameLen,
        MPI_CHAR,
        0,
        MPI_COMM_WORLD);

  // broadcast ranks that need to provide bw data
  vector<int> commRanks;
  int nCommRanks=0;
  if (this->WorldRank==0)
    {
    set<string> hostSet;
    set<string>::iterator it;
    set<string>::iterator npos;

    for (int i=0; i<this->WorldSize; ++i)
      {
      npos=hostSet.end();
      string host=hostNames+(i*hostNameLen);
      it=hostSet.find(host);
      if (it==npos)
        {
        hostSet.insert(host);
        commRanks.push_back(i);
        ++nCommRanks;
        this->Hosts.push_back(host);
        }
      }
    }
  MPI_Bcast(&nCommRanks,1,MPI_INT,0,MPI_COMM_WORLD);
  if (this->WorldRank!=0)
    {
    commRanks.resize(nCommRanks);
    }
  MPI_Bcast(&commRanks[0],nCommRanks,MPI_INT,0,MPI_COMM_WORLD);

  cerr << "nCommRanks=" << nCommRanks << endl;
  cerr << "CommRanks=" << commRanks << endl;

  // build a communicator for the gathering of bw data
  MPI_Group worldGroup;
  MPI_Group newGroup;
  MPI_Comm_group(MPI_COMM_WORLD,&worldGroup);
  MPI_Group_incl(worldGroup,nCommRanks,&commRanks[0],&newGroup);
  MPI_Comm_create(MPI_COMM_WORLD,newGroup,&this->BWComm);
  MPI_Group_free(&worldGroup);
  MPI_Group_free(&newGroup);
  vector<int>::iterator it=find(commRanks.begin(),commRanks.end(),this->WorldRank);
  if (it!=commRanks.end())
    {
    this->InBWComm=1;
    cerr << this->WorldRank << " is in " << endl;
    }
}

//----------------------------------------------------------------------------
vtkSC11DemoAnnotation::~vtkSC11DemoAnnotation()
{
  this->SetDummyFileListDomain(0);
  this->SetBWDataFile(0);
  this->SetTimeDataFile(0);

  if (this->InBWComm)
    {
    MPI_Comm_free(&this->BWComm);
    }
}

//----------------------------------------------------------------------------
int vtkSC11DemoAnnotation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSC11DemoAnnotation::FillOutputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSC11DemoAnnotation::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  int col1Width=0;
  if (this->WorldRank==0) col1Width=this->Hosts[0].size()+3;
  const int col2Width=8;
  const int col3Width=10;
  const int width=col1Width+col2Width+col3Width;
  const int half1Width=width/2+5;
  const int half2Width=width-half1Width;

  // only 1 rank per host needs to access the bw data
  // for that host this prevents counting multiple
  // times.
  string bwData;
  if (this->InBWComm)
    {
    if (this->BWDataFile)
      {
      ifstream bwFile(this->BWDataFile);
      if (!bwFile.good())
        {
        vtkErrorMacro("Failed to open " << this->BWDataFile);
        }

      int bwCommSize;
      MPI_Comm_size(this->BWComm,&bwCommSize);

      double bw=0;
      double bwTotal=0;
      bwFile >> bw;
      bwFile.close();
      vector<double> allBw;
      if (this->WorldRank==0)
        {
        allBw.resize(bwCommSize);
        }
      MPI_Gather(&bw,1,MPI_DOUBLE,&allBw[0],1,MPI_DOUBLE,0,this->BWComm);
      if (this->WorldRank==0)
        {
        ostringstream oss;
        oss
          << setw(1) << "+" << setw(width) << setfill('-') << "-" << setw(1) << "+" <<  endl
          << setw(1) << "|" << setw(half1Width) << setfill(' ') << right << "Bandwidth" << setw(half2Width+1) << right << "|"  << endl
          << setw(1) << "+" << setw(width) << setfill('-') << "-" << setw(1) << "+" << endl;
        for (int i=0; i<bwCommSize; ++i)
          {
          oss
            << setw(1) << "|" << setw(col1Width) << setfill(' ') << left << this->Hosts[i]
            << setw(col2Width) << setfill(' ') << right << setprecision(5) << allBw[i]
            << setw(col3Width) << setfill(' ') << left << " GB/sec"
            << setw(1) << "|"  << endl;
          bwTotal+=allBw[i];
          }
        oss
          << setw(1) << "+" <<  setw(width) << setfill('-') << "-" << setw(1) << "+" <<  endl
          << setw(1) << "|" << setw(col1Width) << setfill(' ') << " "
          << setw(col2Width) << setfill(' ') << right << setprecision(5) << bwTotal
          << setw(col3Width) << setfill(' ') << left <<  " GB/sec"
          << setw(1) << "|" << endl
          << setw(1) << "+" <<  setw(width) << setfill('-') << "-" << setw(1) << "+" <<  endl;

        bwData=oss.str();
        }
      }
    }

  // only rank 0 needs time data
  char buffer[1024];
  string timeData("t=");
  if (this->WorldRank==0)
    {
    memset(buffer,0,1024);
    if (this->TimeDataFile)
      {
      ifstream timeFile(this->TimeDataFile);
      if (timeFile.good())
        {
        timeFile.getline(buffer,1024);
        timeFile.close();

        ostringstream oss;
        oss
          << setw(1) << "+" <<  setw(width) << setfill('-') << "-" << setw(1) << "+" <<  endl
          << setw(1) << "|" <<  setw(width) << setfill(' ') << buffer << setw(1) << "|" <<  endl
          << setw(1) << "+" <<  setw(width) << setfill('-') << "-" << setw(1) << "+" <<  endl;

        timeData=oss.str();
        }
      else
        {
        vtkErrorMacro("Failed to open " << this->TimeDataFile);
        }
      }
    }

  vtkInformation *outInfo;
  vtkTable *output;
  vtkStringArray *data;

  // output port 1 -> bw data
  outInfo=outputVector->GetInformationObject(0);
  output
    = dynamic_cast<vtkTable*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  data=vtkStringArray::New();
  data->SetName("BW Data");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(bwData.c_str());
  output->AddColumn(data);
  data->Delete();

  // output port 2 -> time data
  outInfo=outputVector->GetInformationObject(1);
  output
    = dynamic_cast<vtkTable*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  data=vtkStringArray::New();
  data->SetName("Time Data");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(timeData.c_str());
  output->AddColumn(data);
  data->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSC11DemoAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


