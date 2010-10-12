/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQVolumeSourceCellGenerator.h"

//-----------------------------------------------------------------------------
vtkSQVolumeSourceCellGenerator::vtkSQVolumeSourceCellGenerator()
{
  this->Resolution[0]=
  this->Resolution[1]=1;

  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;

  this->Point1[0]=0.0;
  this->Point1[1]=1.0;
  this->Point1[2]=0.0;

  this->Point2[0]=1.0;
  this->Point2[1]=0.0;
  this->Point2[2]=0.0;

  this->Dx[0]=0.0;
  this->Dx[1]=1.0;
  this->Dx[2]=0.0;

  this->Dy[0]=1.0;
  this->Dy[1]=0.0;
  this->Dy[2]=0.0;
}


//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetResolution(int *r)
{
  this->Resolution[0]=r[0];
  this->Resolution[1]=r[1];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetResolution(int r1, int r2)
{
  this->Resolution[0]=r1;
  this->Resolution[1]=r2;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetOrigin(int *x)
{
  this->Origin[0]=x[0];
  this->Origin[1]=x[1];
  this->Origin[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetOrigin(int x, int y, int z)
{
  this->Origin[0]=x;
  this->Origin[1]=y;
  this->Origin[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint1(int *x)
{
  this->Point1[0]=x[0];
  this->Point1[1]=x[1];
  this->Point1[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint1(int x, int y, int z)
{
  this->Point1[0]=x;
  this->Point1[1]=y;
  this->Point1[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint2(int *x)
{
  this->Point2[0]=x[0];
  this->Point2[1]=x[1];
  this->Point2[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint2(int x, int y, int z)
{
  this->Point2[0]=x;
  this->Point2[1]=y;
  this->Point2[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::ComputeDeltas()
{
  if (this->Resolution[0]<1 || this->Resolution[1]<1 )
    {
    vtkErrorMacro(
      << "Invalid resolution ("
      << this->Resolution[0] << ", "
      << this->Resolution[1] << ").");
    return;
    }

  this->Dx[0]=(this->Point1[0]-this->Origin[0])/this->Resolution[0];
  this->Dx[1]=(this->Point1[1]-this->Origin[1])/this->Resolution[0];
  this->Dx[2]=(this->Point1[2]-this->Origin[2])/this->Resolution[0];

  this->Dx[0]=(this->Point2[0]-this->Origin[0])/this->Resolution[1];
  this->Dx[1]=(this->Point2[1]-this->Origin[1])/this->Resolution[1];
  this->Dx[2]=(this->Point2[2]-this->Origin[2])/this->Resolution[1];
}

//-----------------------------------------------------------------------------
int vtkSQVolumeSourceCellGenerator::GetCellPoints(vtkIdType cid, float *pts)
{
  int i,j;
  IndexToIJ(cid,this->Resolution[0],i,j);

  // lower left corner
  pts[0]=2.0*this->Origin[0]+i*this->Dx[0]+j*this->Dy[0];
  pts[1]=2.0*this->Origin[1]+i*this->Dx[1]+j*this->Dy[1];
  pts[2]=2.0*this->Origin[2]+i*this->Dx[2]+j*this->Dy[2];

  // offset relative to lower left cornern in delta units
  int offset[6]={
      0,0,0,
      1,0,0,
      1,1,0,
      0,1,0
      };

  for (int q=1; q<4; ++q)
    {
    qq=q*3;
    pts[qq  ]=pts[0]+offset[qq]*this->Dx[0]+offset[qq+1]*this->Dy[0];
    pts[qq+1]=pts[1]+offset[qq]*this->Dx[1]+offset[qq+1]*this->Dy[1];
    pts[qq+2]=pts[2]+offset[qq]*this->Dx[2]+offset[qq+1]*this->Dy[2];
    }

  return 4;
}
