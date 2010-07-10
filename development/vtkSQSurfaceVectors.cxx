/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQSurfaceVectors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQSurfaceVectors.h"

#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"

//*****************************************************************************
template<typename T>
void AccumulateNormal(T p1[3], T p2[3], T p3[3], T n[3])
{
  T v1[3]={
      p2[0]-p1[0],
      p2[1]-p1[1],
      p2[2]-p1[2]
      };

  T v2[3]={
      p3[0]-p1[0],
      p3[1]-p1[1],
      p3[2]-p1[2]
      };

  T v3[3];
  vtkMath::Cross(v1,v2,v3);

  n[0]+=v3[0];
  n[1]+=v3[1];
  n[2]+=v3[2];
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQSurfaceVectors);

//-----------------------------------------------------------------------------
vtkSQSurfaceVectors::vtkSQSurfaceVectors()
{
  this->ConstraintMode = MODE_PARALLEL;
}

//-----------------------------------------------------------------------------
vtkSQSurfaceVectors::~vtkSQSurfaceVectors()
{
}

// //-----------------------------------------------------------------------------
// int vtkSQSurfaceVectors::RequestUpdateExtent(
//       vtkInformation * vtkNotUsed(request),
//       vtkInformationVector **inputVector,
//       vtkInformationVector *outputVector)
// {
//   // get the info objects
//   vtkInformation* outInfo = outputVector->GetInformationObject(0);
//   vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
// 
//   inInfo->Set(
//     vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
//     outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())+1);
// 
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkSQSurfaceVectors::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataSet *input
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  // sanity -- empty input
  if (input==0)
    {
    vtkErrorMacro("Empty input.");
    return 1;
    }

  vtkDataSet *output 
    = dynamic_cast<vtkDataSet*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // sanity -- empty output
  if (output==0)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  // sanity -- surface has points
  vtkIdType nPoints=input->GetNumberOfPoints();
  if (nPoints==0)
    {
    vtkErrorMacro("No points.");
    return 1;
    }

  // copy geometry and data
  output->ShallowCopy(input);

  // locate all available vectors
  vector<vtkDataArray *>vectors;
  int nArrays=input->GetPointData()->GetNumberOfArrays();
  for (int i=0; i<nArrays; ++i)
    {
    vtkDataArray *array=input->GetPointData()->GetArray(i);
    if (array->GetNumberOfComponents()==3)
      {
      vectors.push_back(array);
      }
    }

  vtkIdList* cIds = vtkIdList::New();
  vtkIdList* ptIds = vtkIdList::New();

  // for each vector on the input project onto the surface
  int nVectors=vectors.size();
  for (int i=0; i<nVectors; ++i)
    {
    vtkDataArray *proj=vectors[i]->NewInstance();
    proj->SetNumberOfComponents(3);
    proj->SetNumberOfTuples(nPoints);
    proj->SetName(vectors[i]->GetName());
    output->GetPointData()->AddArray(proj);
    proj->Delete();

    vtkDoubleArray *perp=vtkDoubleArray::New();
    perp->SetNumberOfTuples(nPoints);
    osstringstream os;
    os << vectors[i]->GetName() << "_perp";
    perp->SetName(os.str().c_str());
    output->GetPointData()->AddArray(perp);
    perp->Delete();

    for (vtkIdType pid=0; pid<nPoints; ++pid)
      {
      input->GetPointCells(pid,cIds);
      int nCIds=cIds->GetNumberOfIds();

      // Compute the point normal.
      double n[3]={0.0};
      for (int j=0; j<nCIds; ++j)
        {
        switch (input->GetCellType(cIds->GetId(j)))
          {
          case VTK_PIXEL:
          case VTK_POLYGON:
          case VTK_TRIANGLE:
          case VTK_QUAD:
            {
            input->GetCellPoints(cid,ptIds);
            input->GetPoint(ptIds->GetId(0),p1);
            input->GetPoint(ptIds->GetId(1),p2);
            input->GetPoint(ptIds->GetId(2),p3);

            AccumulateNormal(p1,p2,p3,n);
            }
            break;

          default:
            vtkErrorMacro("Unsuported cell type " << cellType << ".");
            return 1;
            break;
          }
        }
      //normal[0]/=nCids;
      //normal[1]/=nCids;
      //normal[2]/=nCids;
      vtkMath::Normalize(n);

      double v[3];
      inVectors->GetTuple(pid,v);

      double v_perp=vtkMath::Dot(n,v);
      perp->InsertValue(pid,v_perp);

      switch (this->ConstraintMode)
        {
        case MODE_PARALLEL:
          // remove perp component
          v[0]=v[0]-(n[0]*v_perp);
          v[1]=v[1]-(n[1]*v_perp);
          v[2]=v[2]-(n[2]*v_perp);

        case MDOE_PERP:
          // remove parallel components
          v[0]=n[0]*v_perp;
          v[1]=n[1]*v_perp;
          v[2]=n[2]*v_perp;

        default:
          vtkErrorMacro("Bad ConstraintMode.");
          return 1;
        }
      proj->InsertTuple(pid,v);
      }
    }
  cellIds->Delete();
  ptIds->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQSurfaceVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ConstraintMode:" << this->ConstraintMode << endl;
}

