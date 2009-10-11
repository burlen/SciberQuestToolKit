/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkOOCFieldTracer.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
// .NAME vtkOOCFieldTracer - Streamline generator
// .SECTION Description
// vtkOOCFieldTracer is a filter that integrates a vector field to generate
// streamlines. The integration is performed using a specified integrator,
// by default Runge-Kutta2. 
// 
// vtkOOCFieldTracer produces polylines as the output, with each cell (i.e.,
// polyline) representing a streamline. The attribute values associated
// with each streamline are stored in the cell data, whereas those
// associated with streamline-points are stored in the point data.
//
// vtkOOCFieldTracer supports forward (the default), backward, and combined
// (i.e., BOTH) integration. The length of a streamline is governed by 
// specifying a maximum value either in physical arc length or in (local)
// cell length. Otherwise, the integration terminates upon exiting the
// flow field domain, or if the particle speed is reduced to a value less
// than a specified terminal speed, or when a maximum number of steps is 
// completed. The specific reason for the termination is stored in a cell 
// array named ReasonForTermination.
//
// Note that normalized vectors are adopted in streamline integration,
// which achieves high numerical accuracy/smoothness of flow lines that is
// particularly guranteed for Runge-Kutta45 with adaptive step size and
// error control). In support of this feature, the underlying step size is
// ALWAYS in arc length unit (LENGTH_UNIT) while the 'real' time interval 
// (virtual for steady flows) that a particle actually takes to trave in a 
// single step is obtained by dividing the arc length by the LOCAL speed. 
// The overall elapsed time (i.e., the life span) of the particle is the 
// sum of those individual step-wise time intervals.
//
// The quality of streamline integration can be controlled by setting the
// initial integration step (InitialIntegrationStep), particularly for 
// Runge-Kutta2 and Runge-Kutta4 (with a fixed step size), and in the case
// of Runge-Kutta45 (with an adaptive step size and error control) the
// minimum integration step, the maximum integration step, and the maximum
// error. These steps are in either LENGTH_UNIT or CELL_LENGTH_UNIT while
// the error is in physical arc length. For the former two integrators,
// there is a trade-off between integration speed and streamline quality.
//
// The integration time, vorticity, rotation and angular velocity are stored
// in point data arrays named "IntegrationTime", "Vorticity", "Rotation" and
// "AngularVelocity", respectively (vorticity, rotation and angular velocity
// are computed only when ComputeVorticity is on). All point data attributes
// in the source dataset are interpolated on the new streamline points.
//
// vtkOOCFieldTracer supports integration through any type of dataset. Thus if
// the dataset contains 2D cells like polygons or triangles, the integration
// is constrained to lie on the surface defined by 2D cells.
//
// The starting point, or the so-called 'seed', of a streamline may be set
// in two different ways. Starting from global x-y-z "position" allows you
// to start a single trace at a specified x-y-z coordinate. If you specify
// a source object, traces will be generated from each point in the source
// that is inside the dataset.
//
// .SECTION See Also
// vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver 
// vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkTemporalStreamTracer
// vtkInterpolatedVelocityField
//  

#ifndef __vtkOOCFieldTracer_h
#define __vtkOOCFieldTracer_h

#include "vtkPolyDataAlgorithm.h"

#include "vtkInitialValueProblemSolver.h" // Needed for constants
#include "TerminationCondition.h"


class vtkCompositeDataSet;
class vtkDataArray;
class vtkDoubleArray;
class vtkExecutive;
class vtkGenericCell;
class vtkIdList;
class vtkIntArray;
class vtkInterpolatedVelocityField;
class vtkOOCReader;
class vtkMultiProcessController;
//BTX
class FieldLine;
//ETX


class VTK_GRAPHICS_EXPORT vtkOOCFieldTracer : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkOOCFieldTracer,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkOOCFieldTracer *New();

  // Description:
  // Specify the dataset with the vector field to analyze.
  void AddVectorInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearVectorInputConnections();
  // Description:
  // Specify a set of seed points to use.
  void AddSeedPointInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearSeedPointInputConnections();
  // Description:
  // Specify a set of surfaces to use.
  void AddTerminatorInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearTerminatorInputConnections();


  // Description:
  // Specify a uniform integration step unit for MinimumIntegrationStep, 
  // InitialIntegrationStep, and MaximumIntegrationStep. NOTE: The valid
  // units are LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2).
  void SetStepUnit(int unit);
  vtkGetMacro(StepUnit,int);

  // Description:
  // Specify the Initial step size used for line integration.
  vtkSetMacro(InitialStep,double);
  vtkGetMacro(InitialStep,double);

  // Description:
  // Specify the Minimum step size used for line integration.
  vtkSetMacro(MinStep,double);
  vtkGetMacro(MinStep,double);

  // Description:
  // Specify the Maximum step size used for line integration.
  vtkSetMacro(MaxStep,double);
  vtkGetMacro(MaxStep,double);

  // Description
  // Specify the maximum error tolerated throughout streamline integration.
  vtkSetMacro(MaxError,double);
  vtkGetMacro(MaxError,double);

  // Description
  // Specify the maximum number of steps for integrating a streamline.
  vtkSetMacro(MaxNumberOfSteps,vtkIdType);
  vtkGetMacro(MaxNumberOfSteps,vtkIdType);

  // Description:
  // Specify the maximum length of a streamline expressed in LENGTH_UNIT.
  vtkSetMacro(MaxLineLength,vtkIdType);
  vtkGetMacro(MaxLineLength,vtkIdType);

  // Description
  // Specify the terminal speed value, below which integration is terminated.
  vtkSetMacro(TerminalSpeed,double);
  vtkGetMacro(TerminalSpeed,double);

  // Description:
  // Control of the OOC read size. This parameter may have different
  // meaning in the context of different readers.
  vtkSetMacro(OOCNeighborhoodSize,int);
  vtkGetMacro(OOCNeighborhoodSize,int);

protected:
  vtkOOCFieldTracer();
  ~vtkOOCFieldTracer();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  int RequestUpdateExtent(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

private:
  // Description:
  // Trace one field line from the given seed point, using the given out-of-core
  // reader.
  void OOCIntegrateOne(vtkOOCReader *oocR,const char *fieldName,FieldLine *line,TerminationCondition *tcon);

  // Description:
  // Convert from cell fractional unit into length.
  void ClipStep(
      double& step,
      int stepSign,
      double& minStep,
      double& maxStep,
      double cellLength,
      double lineLength);
  // Description:
  // Convert from cell fractional unit into length.
  void ConvertIntervals(
        double& step,
        double& minStep,
        double& maxStep,
        int direction,
        double cellLength);
  // Description:
  // Convert from cell fractional unit into length.
  static double ConvertToLength(double interval,int unit,double cellLength);

  int SetupOutput(vtkInformation* inInfo,vtkInformation* outInfo);

  void InitializeSeeds(vtkDataArray*& seeds,vtkIdList*& seedIds,vtkIntArray*&dirs,vtkDataSet *source);

  vtkOOCFieldTracer(const vtkOOCFieldTracer&);  // Not implemented.
  void operator=(const vtkOOCFieldTracer&);  // Not implemented.

private:
  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  // Parameters controlling integration,
  int StepUnit;
  double InitialStep;
  double MinStep;
  double MaxStep;
  double MaxError;
  vtkIdType MaxNumberOfSteps;
  double MaxLineLength;
  double TerminalSpeed;
  vtkMultiProcessController *Controller;

  static const double EPSILON;

  // Parameter to adjust the size of data reads.
  int OOCNeighborhoodSize;

  // This object is used to stop integration when field line
  // crosses one of a given set of surfaces. It also has logic
  // for detecting when a field line leaves a region defined
  // by a box.
  TerminationCondition *TermCon;
};


#endif


