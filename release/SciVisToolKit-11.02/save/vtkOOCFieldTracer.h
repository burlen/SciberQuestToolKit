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

#include "vtkDataSetAlgorithm.h"
#include <vector>
using std::vector;

class vtkUnstructuredGrid;
class vtkOOCReader;
class vtkMultiProcessController;
class vtkInitialValueProblemSolver;
class vtkPointSet;
//BTX
class FieldLine;
class TerminationCondition;
//ETX


class VTK_EXPORT vtkOOCFieldTracer : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkOOCFieldTracer,vtkDataSetAlgorithm);
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
  vtkSetMacro(MaxLineLength,double);
  vtkGetMacro(MaxLineLength,double);

  // Description
  // Specify the terminal speed value, below which integration is terminated.
  vtkSetMacro(NullThreshold,double);
  vtkGetMacro(NullThreshold,double);

  // Description:
  // Control of the OOC read size. This parameter may have different
  // meaning in the context of different readers.
  vtkSetMacro(OOCNeighborhoodSize,int);
  vtkGetMacro(OOCNeighborhoodSize,int);

  // Description:
  // Set the run mode of the filter. If this flag is set then a topology 
  // map is produced in place of field lines. This allows this filter to
  // serve as two ParaView filters, the OOCFieldTracer and the OOCTopologyMapper.
  vtkSetMacro(TopologyMode,int);
  vtkGetMacro(TopologyMode,int);

  // Description:
  // If on then color map produced will only contain used colors. NOTE: requires
  // a global communication,
  vtkSetMacro(SqueezeColorMap,int);
  vtkGetMacro(SqueezeColorMap,int);

protected:
  vtkOOCFieldTracer();
  ~vtkOOCFieldTracer();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  int RequestUpdateExtent(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  int RequestDataObject(vtkInformation *info,vtkInformationVector** input,vtkInformationVector* output);

private:
  //BTX
  // Description:
  // Given a set of polygons (seedSource) that is assumed duplicated across
  // all process in the communicator, extract an equal number of polygons
  // on each process and compute seed points at the center of each local
  // poly. The computed points are stored in new FieldLine structures. It is
  // the callers responsibility to delete these structures.
  // Return 0 if an error occurs. Upon successful completion the number
  // of seed points is returned.
  int CellsToSeeds(
        int procId,
        int nProcs,
        vtkPointSet *seedSource,
        vector<FieldLine*> &lines);
  // Description:
  // Given a set of polygons (seedSource) that is assumed duplicated across
  // all process in the communicator, extract an equal number of polygons
  // on each process and compute seed points at the center of each local
  // poly. In addition copy the local polys (seedOut). The computed points
  // are stored in new FieldLine structures. It is the callers responsibility
  // to delete these structures.
  // Return 0 if an error occurs. Upon successful completion the number
  // of seed points is returned.
  int CellsToSeeds(
        int procId,
        int nProcs,
        vtkPolyData *seedSource,
        vtkPolyData *seedOut,
        vector<FieldLine*> &lines);
  int CellsToSeeds(
        int procId,
        int nProcs,
        vtkUnstructuredGrid *seedSource,
        vtkUnstructuredGrid *seedOut,
        vector<FieldLine*> &lines);
  // Description:
  // Helper to call the right methods.
  vtkIdType DispatchCellsToSeeds(
        int procId,
        int nProcs,
        vtkDataSet *Source,
        vtkDataSet *Out,
        vector<FieldLine *> &lines);

  // Description:
  // Trace one field line from the given seed point, using the given out-of-core
  // reader. As segments are generated they are tested using the stermination 
  // condition and terminated imediately. The last neighborhood read is stored
  // in the nhood parameter. It is up to the caller to delete this.
  void OOCIntegrateOne(
        vtkOOCReader *oocR,
        const char *fieldName,
        FieldLine *line,
        TerminationCondition *tcon,
        vtkDataSet *&nhood);
  // Description:
  // USe the set of field lines to construct a vtk polydata set. Field line structures
  // are deleted as theya re coppied.
  int FieldLinesToPolydata(
        vector<FieldLine*> &lines,
        vtkIdType nPtsTotal,
        vtkPolyData *fieldLines);
  //ETX

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
  vtkInitialValueProblemSolver* Integrator;
  vtkMultiProcessController *Controller;

  // Parameters controlling integration,
  int StepUnit;
  double InitialStep;
  double MinStep;
  double MaxStep;
  double MaxError;
  vtkIdType MaxNumberOfSteps;
  double MaxLineLength;
  double NullThreshold;

  static const double EPSILON;

  // Reader related
  int OOCNeighborhoodSize;

  // Output controls
  int TopologyMode;
  int SqueezeColorMap;


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
