/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
// .NAME vtkOOCFieldTracer - Streamline generator
// .SECTION Description
//
// Scalable field line tracer using RK45 Adds capability to
// terminate trace upon intersection with one of a set of
// surfaces.
// TODO verify that VTK rk45 implementation increases step size!!

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


class VTK_EXPORT vtkOOCFieldTracer : public vtkPolyDataAlgorithm
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
  static double ConvertToLength(double interval,int unit,double cellLength);

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

  //BTX
  // units
  enum 
    {
    ARC_LENGTH=1,
    CELL_FRACTION=2
    };
 //ETX
};

#endif
