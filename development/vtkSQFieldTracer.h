/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQFieldTracer - Streamline generator
// .SECTION Description
//
// Scalable field line tracer using RK45 Adds capability to
// terminate trace upon intersection with one of a set of
// surfaces.
// TODO verify that VTK rk45 implementation increases step size!!

#ifndef __vtkSQFieldTracer_h
#define __vtkSQFieldTracer_h

#include "vtkDataSetAlgorithm.h"

#include<map>
using std::map;
using std::pair;

class vtkUnstructuredGrid;
class vtkOOCReader;
class vtkMultiProcessController;
class vtkInitialValueProblemSolver;
class vtkPointSet;
//BTX
class FieldLine;
class CellIdBlock;
class FieldTraceData;
class TerminationCondition;
//ETX


class VTK_EXPORT vtkSQFieldTracer : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQFieldTracer,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQFieldTracer *New();

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
  // Set the run mode of the filter.
  // The enumeration is as follows:
  //    0  TOPOLOGY filter produces field topology map
  //    1  STREAM   filter produces stream lines
  //    2  POINCARE filter produces a poincare map
  // This allows this filter to serve as multiple ParaView filters, 
  // the OOCFieldTracer, OOCDTopologyMapper, and OOCDPoincareMapper
  // NOTE This only works if Mode is set before the filter runs.
  // PV gets confused if you try to change Mode later.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);

  // Description:
  // If on then color map produced will only contain used colors. 
  // NOTE: requires a global communication,
  vtkSetMacro(SqueezeColorMap,int);
  vtkGetMacro(SqueezeColorMap,int);

  // Description:
  // Sets the work unit (in number of seed points) for slave processes.
  vtkSetClampMacro(WorkerBlockSize,int,1,VTK_INT_MAX);
  vtkGetMacro(WorkerBlockSize,int);

  // Description:
  // Sets the work unit (in number of seed points) for the master. This should
  // be much less than the slave block size, so that the master can respond
  // timely to slave requests.
  vtkSetClampMacro(MasterBlockSize,int,1,VTK_INT_MAX);
  vtkGetMacro(MasterBlockSize,int);

  // Description:
  vtkSetMacro(UseDynamicScheduler,int);
  vtkGetMacro(UseDynamicScheduler,int);

protected:
  vtkSQFieldTracer();
  ~vtkSQFieldTracer();

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
  // Helper calls the right integration scheduler.
  int Integrate(
      vtkDataSet *source,
      vtkDataSet *out,
      const char *fieldName,
      vtkOOCReader *oocr,
      FieldTraceData *topoMap);

  // Description:
  // Integrate over all local cells. This assumes that each process has a unique
  // subset of the work (i.e. seed source cells are statically distributed),
  // The cache should initially  be null and after the caller should delete the
  // cache.
  int IntegrateStatic(
      vtkDataSet *source,
      vtkDataSet *out,
      const char *fieldName,
      vtkOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTraceData *topoMap);

  // Description:
  // Distribute the work load according to a master-slave self scheduling scheme. All
  // seed cells must be present on all process, work is dished out by process 0 in 
  // contiguous blocks of cell ids.
  // The ooc reader cache should initially  be null and after the caller should delete
  // the cache.
  int IntegrateDynamic(
      int procId,
      int nProcs,
      vtkDataSet *source,
      vtkDataSet *out,
      const char *fieldName,
      vtkOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTraceData *topoMap);

  // Description:
  // Integrate field lines seeded from a block of consecutive cell ids.
  int IntegrateBlock(
        CellIdBlock *sourceIds,
        FieldTraceData *topoMap,
        const char *fieldName,
        vtkOOCReader *oocr,
        vtkDataSet *&oocrCache);


  // Description:
  // Trace one field line from the given seed point, using the given out-of-core
  // reader. As segments are generated they are tested using the stermination 
  // condition and terminated imediately. The last neighborhood read is stored
  // in the nhood parameter. It is up to the caller to delete this.
  void IntegrateOne(
        vtkOOCReader *oocR,
        vtkDataSet *&oocRCache,
        const char *fieldName,
        FieldLine *line,
        TerminationCondition *tcon);
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

  // TODO use only arc-length
  // Description:
  // Convert from cell fractional unit into length.
  static double ConvertToLength(double interval,int unit,double cellLength);

  vtkSQFieldTracer(const vtkSQFieldTracer&);  // Not implemented.
  void operator=(const vtkSQFieldTracer&);  // Not implemented.

private:
  vtkInitialValueProblemSolver* Integrator;
  vtkMultiProcessController *Controller;

  // Parameter controlling load balance
  int UseDynamicScheduler;
  int WorkerBlockSize;
  int MasterBlockSize;

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
  TerminationCondition *TermCon;

  //BTX
  // Output controls
  enum
    {
    TOPOLOGY=0,
    STREAM=1,
    POINCARE=2
    };
  //ETX
  int Mode;
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
