/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __vtkSQOnDemandPlaneSource_h
#define __vtkSQOnDemandPlaneSource_h

/// Plane sources that provide data on demand
/**
*/
class vtkSQOnDemandPlaneSource : public vtkSQOnDemandSource
{
public:
  static vtkSQPlaneSource *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkSQOnDemandPlaneSource, vtkObject);

  /**
  Return the total number of cells available.
  */
  virtual vtkIdType GetNumberOfCells()
    {
    return this->Resolution[0]*this->Resolution[1];
    }

  /**
  Return the number of points required for the named
  cell. For homogeneous datasets its always the same.
  */
  virtual int GetNumberOfCellPoints(vtkIdType id){ return 4;}

  /**
  Copy the points from a cell into the provided buffer,
  buffer is expected to be large enought to hold all of the
  points. Return is zero if no error occured.
  */
  virtual int GetCellPoints(vtkIdType cid, float *pts);

  /**
  Set/Get plane cell resolution.
  */
  void SetResolution(int *r);
  void SetResolution(int rx, int ry);
  vtkGetVector2Macro(Resolution,int);
  /**
  Set/Get plane coordinates, origin,point1,point2.
  */
  void SetOrigin(double *o);
  void SetOrigin(double x, double y, double z);
  vtkGetVector3Macro(Origin,double);

  void SetPoint1(double *o);
  void SetPoint1(double x, double y, double z);
  vtkGetVector3Macro(Point1,double);

  void SetPoint2(double *o);
  void SetPoint2(double x, double y, double z);
  vtkGetVector3Macro(Point2,double);

  /**
  Call this after any change to coordinates and resolution.
  */
  void ComputeDeltas();

protected:
  vtkOnDemandPlaneSource();
  virtual ~vtkOnDemandPlaneSource();



private:
  int Resolution[2];
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Dx[3];
  double Dy[3];
};

#endif
