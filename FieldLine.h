/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef FieldLine_h
#define FieldLine_h

//=============================================================================
class FieldLine
{
public:
  FieldLine(double p[3], int seedId=0)
      :
    FwdTrace(0),
    BwdTrace(0),
    SeedId(seedId),
    FwdTerminator(0),
    BwdTerminator(0)
    {
    this->Seed[0]=p[0];
    this->Seed[1]=p[1];
    this->Seed[2]=p[2];

    this->FwdTrace=vtkFloatArray::New();
    this->FwdTrace->SetNumberOfComponents(3);
    this->FwdTrace->Allocate(128);
    this->BwdTrace=vtkFloatArray::New();
    this->BwdTrace->SetNumberOfComponents(3);
    this->BwdTrace->Allocate(128);
    }
  FieldLine(const FieldLine &other)
    {
    *this=other;
    }
  ~FieldLine()
    {
    this->FwdTrace->Delete();
    this->BwdTrace->Delete();
    }
  const FieldLine &operator=(const FieldLine &other)
    {
    if (&other==this)
      {
      return *this;
      }
    this->Seed[0]=other.Seed[0];
    this->Seed[1]=other.Seed[1];
    this->Seed[2]=other.Seed[2];

    this->SeedId=other.SeedId;

    this->FwdTerminator=other.FwdTerminator;
    this->BwdTerminator=other.BwdTerminator;

    this->FwdTrace->Delete();
    this->FwdTrace=other.FwdTrace;
    this->FwdTrace->Register(0);
    this->BwdTrace->Delete();
    this->BwdTrace=other.BwdTrace;
    this->BwdTrace->Register(0);

    return *this;
    }
  void PushPoint(int dir,float *p)
    {
    assert((dir>=0)&&(dir<=1));
    vtkFloatArray *line=dir==0?BwdTrace:FwdTrace;
    line->InsertNextTuple(p);
    }
  void PushPoint(int dir,double *p)
    {
    assert((dir>=0)&&(dir<=1));
    vtkFloatArray *line=dir==0?BwdTrace:FwdTrace;
    line->InsertNextTuple(p);
    }
  void SetTerminator(int dir, int code)
    {
    assert((dir>=0)&&(dir<=1));
    int *term=dir==0?&this->BwdTerminator:&this->FwdTerminator;
    *term=code;
    }

  int GetForwardTerminator() const
    {
    return this->FwdTerminator;
    }
  int GetBackwardTerminator() const
    {
    return this->BwdTerminator;
    }
  int GetSeedId() const
    {
    return this->SeedId;
    }
  float *GetSeedPoint()
    {
    return this->Seed;
    }
  const float *GetSeedPoint() const
    {
    return this->Seed;
    }
  void GetSeedPoint(float p[3]) const
    {
    p[0]=this->Seed[0];
    p[1]=this->Seed[1];
    p[2]=this->Seed[2];
    }
  void GetSeedPoint(double p[3]) const
    {
    p[0]=this->Seed[0];
    p[1]=this->Seed[1];
    p[2]=this->Seed[2];
    }
  vtkIdType GetNumberOfPoints()
    {
    return
    this->FwdTrace->GetNumberOfTuples()+this->BwdTrace->GetNumberOfTuples()-1;
    // less one because seed point is duplicated in both.
    }
  vtkIdType CopyPointsTo(float *pts)
    {
    // Copy the bwd running field line, reversing its order
    // so it ends on the seed point.
    vtkIdType nPtsBwd=this->BwdTrace->GetNumberOfTuples();
    float *pbtr=this->BwdTrace->GetPointer(0);
    pbtr+=3*nPtsBwd-3;
    for (vtkIdType i=0; i<nPtsBwd; ++i,pts+=3,pbtr-=3)
      {
      pts[0]=pbtr[0];
      pts[1]=pbtr[1];
      pts[2]=pbtr[2];
      }
    // Copy the forward running field line, skip the
    // seed point as we already coppied it.
    vtkIdType nPtsFwd=this->FwdTrace->GetNumberOfTuples();
    float *pftr=this->FwdTrace->GetPointer(0);
    pftr+=3; // skip
    for (vtkIdType i=1; i<nPtsFwd; ++i,pts+=3,pftr+=3)
      {
      pts[0]=pftr[0];
      pts[1]=pftr[1];
      pts[2]=pftr[2];
      }
    return nPtsBwd+nPtsFwd-1;
    }

private:
  FieldLine();

private:
  vtkFloatArray *FwdTrace;
  vtkFloatArray *BwdTrace;
  float Seed[3];
  int SeedId;
  int FwdTerminator;
  int BwdTerminator;
};

#endif
