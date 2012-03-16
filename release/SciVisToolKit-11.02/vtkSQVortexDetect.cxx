/*=====================

  Program:   Visualization Toolkit
  Module:    vtkSQVortexDetect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=====================*/
#include "vtkSQVortexDetect.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTensor.h"


#include <cmath>

#include <string>
using std::string;

#include <algorithm>
using std::max;

#include<Eigen/Eigenvalues>
using namespace Eigen;

namespace {

// ****************************************************************************
void ComputeVectorGradient(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkDataSet *input,
      vtkIdType nCells,
      vtkDataArray *V,
      vtkDoubleArray *gradV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  gradV->SetNumberOfComponents(9);
  gradV->SetNumberOfTuples(nCells);
  string name="grad-";
  name+=V->GetName();
  gradV->SetName(name.c_str());
  double *pGradV=gradV->GetPointer(0);

  vtkDoubleArray *cellV=vtkDoubleArray::New();
  cellV->SetNumberOfComponents(3);
  cellV->Allocate(3*VTK_CELL_SIZE);

  vtkGenericCell *cell=vtkGenericCell::New();

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    input->GetCell(cellId,cell);

    double coords[3];
    cell->GetParametricCenter(coords);

    V->GetTuples(cell->PointIds,cellV);

    double *pCellV=cellV->GetPointer(0);

    double grad[9];
    cell->Derivatives(0,coords,pCellV,3,grad);

    for (int i=0; i<9; ++i)
      {
      pGradV[i]=grad[i];
      }
    pGradV+=9;
    }

  cell->Delete();
  cellV->Delete();
}

// ****************************************************************************
void ComputeVorticity(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkIdType nCells,
      vtkDoubleArray *gradV,
      vtkDoubleArray *vortV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  double *pGradV=gradV->GetPointer(0);

  vortV=vtkDoubleArray::New();
  vortV->SetNumberOfComponents(3);
  vortV->SetNumberOfTuples(nCells);
  string name="vort-";
  name+=gradV->GetName();
  vortV->SetName(name.c_str());
  double *pVortV=vortV->GetPointer(0);

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    pVortV[0]=pGradV[7]-pGradV[5];
    pVortV[1]=pGradV[2]-pGradV[6];
    pVortV[2]=pGradV[3]-pGradV[1];

    pVortV+=3;
    pGradV+=9;
    }
}

// ****************************************************************************
void ComputeFTLE(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkIdType nCells,
      vtkDoubleArray *gradV,
      vtkDoubleArray *ftleV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  double *pGradV=gradV->GetPointer(0);

  ftleV->SetNumberOfComponents(1);
  ftleV->SetNumberOfTuples(nCells);
  ftleV->SetName("ftle");
  double *pFtleV=ftleV->GetPointer(0);

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    Matrix<double,3,3> J;
    J <<
      pGradV[0], pGradV[1], pGradV[2],
      pGradV[3], pGradV[4], pGradV[5],
      pGradV[6], pGradV[7], pGradV[8];

    Matrix<double,3,3> JJT;
    JJT=J*J.transpose();

    // compute eigen values
    Matrix<double,3,1> e;
    SelfAdjointEigenSolver<Matrix<double,3,3> >solver(JJT,false);
    e=solver.eigenvalues();

    double lam;
    lam=max(e(0,0),e(1,0));
    lam=max(lam,e(2,0));

    pFtleV[0]=log(sqrt(lam));

    pFtleV+=1;
    pGradV+=9;
    }
}

// ****************************************************************************
template <typename T>
void ComputeMagnitude(size_t nTups, size_t nComps,  T *V, T *mV)
{
  for (size_t i=0; i<nTups; ++i)
    {
    double mag=0.0;
    size_t ii=nComps*i;
    for (size_t q=0; q<nComps; ++q)
      {
      double v=V[ii+q];
      mag+=v*v;
      }
    mV[i]=sqrt(mag);
    }
}

};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQVortexDetect);

//-----------------------------------------------------------------------------
vtkSQVortexDetect::vtkSQVortexDetect()
      :
  PassInput(0),
  SplitComponents(0),
  ResultMagnitude(0),
  ComputeGradient(0),
  ComputeVorticity(0),
  ComputeHelicity(0),
  ComputeNormalizedHelicity(0),
  ComputeQ(0),
  ComputeLambda2(0),
  ComputeDivergence(0),
  ComputeFTLE(0),
  ComputeEigenvalueDiagnostic(0)
{
  #ifdef vtkSQVortexDetectDEBUG
  pCerr() << "=====vtkSQVortexDetect::vtkSQVortexDetect" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // by default process active point vectors
  this->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS);
}

//-----------------------------------------------------------------------------
int vtkSQVortexDetect::Initialize(vtkPVXMLElement *root)
{
  #ifdef vtkSQVortexDetectDEBUG
  pCerr() << "=====vtkSQVortexDetect::Initialize" << endl;
  #endif

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQVortexDetect::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQVortexDetectDEBUG
  pCerr() << "=====vtkSQVortexDetect::RequestData" << endl;
  #endif

  vtkInformation *inInfo = inInfos[0]->GetInformationObject(0);
  vtkInformation *outInfo = outInfos->GetInformationObject(0);

  vtkDataSet *input
     = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    vtkErrorMacro("Null input.");
    return 1;
    }

  vtkIdType nCells=input->GetNumberOfCells();
  if (nCells<1)
    {
    vtkErrorMacro("No cells on input.");
    return 1;
    }

  vtkDataSet *output
     = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    vtkErrorMacro("Null output.");
    return 1;
    }

  output->CopyStructure(input);
  if (this->PassInput)
    {
    output->CopyAttributes(input);
    }

  vtkDataArray *V=this->GetInputArrayToProcess(0,inInfos);

  // Gradient.
  vtkDoubleArray *gradV=vtkDoubleArray::New();
  ::ComputeVectorGradient(this,0.0,0.1,input,nCells,V,gradV);
  if (this->ComputeGradient)
    {
    output->GetCellData()->AddArray(gradV);
    }
  this->UpdateProgress(0.1);

  if (this->ComputeVorticity || this->ComputeHelicity || this->ComputeNormalizedHelicity)
    {
    // Vorticity.
    vtkDoubleArray *vortV=vtkDoubleArray::New();
    ::ComputeVorticity(this,0.1,0.2,nCells,gradV,vortV);
    if (this->ComputeVorticity)
      {
      output->GetCellData()->AddArray(vortV);
      }
    this->UpdateProgress(0.2);

    // Helicity.
    if (this->ComputeHelicity)
      {
      }
    this->UpdateProgress(0.3);

    // Normalized Helicity.
    if (this->ComputeNormalizedHelicity)
      {
      }

    vortV->Delete();
    }
  this->UpdateProgress(0.4);

  // Q Criteria
  if (this->ComputeQ)
    {
    }
  this->UpdateProgress(0.5);

  // Lambda-2.
  if (this->ComputeLambda2)
    {
    }
  this->UpdateProgress(0.6);

  // Divergence.
  if (this->ComputeDivergence)
    {
    }
  this->UpdateProgress(0.7);

  // EigenvalueDiagnostic.
  if (this->ComputeEigenvalueDiagnostic)
    {
    }
  this->UpdateProgress(0.8);

  // FTLE
  if (this->ComputeFTLE)
    {
    vtkDoubleArray *ftleV=vtkDoubleArray::New();
    ::ComputeFTLE(this,0.8,0.9,nCells,gradV,ftleV);
    output->GetCellData()->AddArray(ftleV);
    ftleV->Delete();
    }
  this->UpdateProgress(0.9);

  // magnitidue
  if (this->ResultMagnitude)
    {
    int nOutArrays=output->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<nOutArrays; ++i)
      {
      vtkDataArray *da=output->GetCellData()->GetArray(i);
      size_t daNc=da->GetNumberOfComponents();
      if (daNc==1)
        {
        continue;
        }
      vtkDataArray *mda=da->NewInstance();
      size_t daNt=da->GetNumberOfTuples();
      mda->SetNumberOfTuples(daNt);
      string name="mag-";
      name+=da->GetName();
      mda->SetName(name.c_str());
      output->GetPointData()->AddArray(mda);
      mda->Delete();
      switch(V->GetDataType())
        {
        vtkTemplateMacro(
          ::ComputeMagnitude<VTK_TT>(
              daNt,
              daNc,
              (VTK_TT*)da->GetVoidPointer(0),
              (VTK_TT*)mda->GetVoidPointer(0)));
        }
      }
    }
  this->UpdateProgress(1.0);

  gradV->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQVortexDetect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

/*
vtk code for eigen values.

  double ux=pV[0];
  double uy=pV[1];
  double uz=pV[2];
  double vx=pV[3];
  double vy=pV[4];
  double vz=pV[5];
  double wx=pV[6];
  double wy=pV[7];
  double wz=pV[8];

  double JJT0[3], JJT1[3], JJT2[3];
  double *JJT[3]={JJT0, JJT1, JJT2};

  JJT[0][0] = ux*ux+vx*vx+wx*wx;
  JJT[0][1] = ux*uy+vx*vy+wx*wy;
  JJT[0][2] = ux*uz+vx*vz+wx*wz;
  JJT[1][0] = JJT[0][1];
  JJT[1][1] = uy*uy+vy*vy+uz*wz;
  JJT[1][2] = uy*uz+vy*vz+wy*wz;
  JJT[2][0] = JJT[0][2];
  JJT[2][1] = JJT[1][2];
  JJT[2][2] = uz*uz+vz*vz+wz*wz;
*/
