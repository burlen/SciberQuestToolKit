/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Numerics_h
#define Numerics_h

#include<Eigen/Core>
#include<Eigen/QR>
using namespace Eigen;

#include "Tuple.hxx"

//=============================================================================
template <typename T>
class CentralStencil
{
public:
  CentralStencil(int ni, int nj, int nk, int nComps, T *v)
       :
      Ni(ni),Nj(nj),Nk(nk),NiNj(ni*nj),
      NComps(nComps),
      Vilo(0),Vihi(0),Vjlo(0),Vjhi(0),Vklo(0),Vkhi(0),
      V(v)
       {}

  void SetCenter(int i, int j, int k)
    {
    this->Vilo=this->NComps*(k*this->NiNj+j*this->Ni+(i-1));
    this->Vihi=this->NComps*(k*this->NiNj+j*this->Ni+(i+1));
    this->Vjlo=this->NComps*(k*this->NiNj+(j-1)*this->Ni+i);
    this->Vjhi=this->NComps*(k*this->NiNj+(j+1)*this->Ni+i);
    this->Vklo=this->NComps*((k-1)*this->NiNj+j*this->Ni+i);
    this->Vkhi=this->NComps*((k+1)*this->NiNj+j*this->Ni+i);
    }

  // center
  // T *Operator()()
  //   {
  //   return this->V+this->Vilo+this->NComps;
  //   }

  // i direction
  T ilo(int comp)
    {
    return this->V[this->Vilo+comp];
    }
  T ihi(int comp)
    {
    return this->V[this->Vihi+comp];
    }
  // j direction
  T jlo(int comp)
    {
    return this->V[this->Vjlo+comp];
    }
  T jhi(int comp)
    {
    return this->V[this->Vjhi+comp];
    }
  // k-direction
  T klo(int comp)
    {
    return this->V[this->Vklo+comp];
    }
  T khi(int comp)
    {
    return this->V[this->Vkhi+comp];
    }


private:
  CentralStencil();
private:
  int Ni,Nj,Nk,NiNj;
  int NComps;
  int Vilo,Vihi,Vjlo,Vjhi,Vklo,Vkhi;
  T *V;
};

//*****************************************************************************
template<typename T>
void sort(T *a, int l, int r)
{
  for (int i=l; i<r; ++i)
    {
    for (int j=i; j>l; --j)
      {
      if (a[j]>a[j-1])
        {
        double tmp=a[j-1];
        a[j-1]=a[j];
        a[j]=tmp;
        }
      }
    }
}

// I  -> number of points
// V  -> vector field
// mV -> Magnitude
//*****************************************************************************
template <typename T>
void Magnitude(int *I, T *V, T *mV)
{
  for (int k=0; k<I[2]; ++k) {
    for (int j=0; j<I[1]; ++j) {
      for (int i=0; i<I[0]; ++i) 
        {
        const int p  = k*I[0]*I[1]+j*I[0]+i;
        const int vi = 3*p;
        const int vj = vi + 1;
        const int vk = vi + 2;
        mV[p]=sqrt(V[vi]*V[vi]+V[vj]*V[vj]+V[vk]*V[vk]);
        }
      }
    }
}

//*****************************************************************************
template <typename T>
void FaceDiv(int *I, double *dX, T *V, T *mV, T *div)
{
  // *hi variables are numbe of cells in the out cell centered
  // array. The in array is a point centered array of face data
  // with the last face left off.
  const int pihi=I[0]+1;
  const int pjhi=I[1]+1;
  // const int pkhi=I[2]+1;

  for (int k=0; k<I[2]; ++k)
    {
    for (int j=0; j<I[1]; ++j)
      {
      for (int i=0; i<I[0]; ++i)
        {
        const int c=k*I[0]*I[1]+j*I[0]+i;
        const int p=k*pihi*pjhi+j*pihi+i;

        const int vilo = 3 * (k*pihi*pjhi+j*pihi+ i   );
        const int vihi = 3 * (k*pihi*pjhi+j*pihi+(i+1));
        const int vjlo = 3 * (k*pihi*pjhi+   j *pihi+i) + 1;
        const int vjhi = 3 * (k*pihi*pjhi+(j+1)*pihi+i) + 1;
        const int vklo = 3 * (   k *pihi*pjhi+j*pihi+i) + 2;
        const int vkhi = 3 * ((k+1)*pihi*pjhi+j*pihi+i) + 2;

        //cerr << "(" << vilo << ", " << vihi << ", " << vjlo << ", " << vjhi << ", " << vklo << ", " << vkhi << ")" << endl;

        // const double modV=mV[cId];
        // (sqrt(V[vilo]*V[vilo] + V[vjlo]*V[vjlo] + V[vklo]*V[vklo])
        // + sqrt(V[vihi]*V[vihi] + V[vjhi]*V[vjhi] + V[vkhi]*V[vkhi]))/2.0;

        div[c] =(V[vihi]-V[vilo])/dX[0]/mV[p];
        div[c]+=(V[vjhi]-V[vjlo])/dX[1]/mV[p];
        div[c]+=(V[vkhi]-V[vklo])/dX[2]/mV[p];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// W      -> vector curl
//*****************************************************************************
template <typename T>
void Rotation(int *input, int *output, double *dX, T *V, T *W)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  const int _ninj=_ni*_nj;

  // stencil deltas
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil into the input array
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //      __   ->
        //  w = \/ x V
        W[vi]=(V[vjhi+2]-V[vjlo+2])/dx[1]-(V[vkhi+1]-V[vklo+1])/dx[2];
        W[vj]=(V[vkhi  ]-V[vklo  ])/dx[2]-(V[vihi+2]-V[vilo+2])/dx[0];
        W[vk]=(V[vihi+1]-V[vilo+1])/dx[0]-(V[vjhi  ]-V[vjlo  ])/dx[1];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// H      -> helicity
//*****************************************************************************
template <typename T>
void Helicity(int *input, int *output, double *dX, T *V, T *H)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  const int _ninj=_ni*_nj;

  // stencil deltas
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //      __   ->
        //  w = \/ x V
        const double w[3]={
              (V[vjhi+2]-V[vjlo+2])/dx[1]-(V[vkhi+1]-V[vklo+1])/dx[2],
              (V[vkhi  ]-V[vklo  ])/dx[2]-(V[vihi+2]-V[vilo+2])/dx[0],
              (V[vihi+1]-V[vilo+1])/dx[0]-(V[vjhi  ]-V[vjlo  ])/dx[1]};
        //        ->  ->
        // H_n =  V . w
        H[pi]=(V[vi]*w[0]+V[vj]*w[1]+V[vk]*w[2]);
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// H      -> normalized helicity(out)
//*****************************************************************************
template <typename T>
void NormalizedHelicity(int *input, int *output, double *dX, T *V, T *H)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  const int _ninj=_ni*_nj;

  // stencil deltas
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //  ->
        // |V|
        const double modV
          = sqrt(V[vi]*V[vi]+V[vj]*V[vj]+V[vk]*V[vk]);

        //      __   ->
        //  w = \/ x V
        const double w[3]={
              (V[vjhi+2]-V[vjlo+2])/dx[1]-(V[vkhi+1]-V[vklo+1])/dx[2],
              (V[vkhi  ]-V[vklo  ])/dx[2]-(V[vihi+2]-V[vilo+2])/dx[0],
              (V[vihi+1]-V[vilo+1])/dx[0]-(V[vjhi  ]-V[vjlo  ])/dx[1]};

        const double modW=sqrt(w[0]*w[0]+w[1]*w[1]+w[2]*w[2]);

        //         ->  ->     -> ->
        // H_n = ( V . w ) / |V||w|
        H[pi]=(V[vi]*w[0]+V[vj]*w[1]+V[vk]*w[2])/(modV*modW);
        // Cosine of the angle between v and w. Angle between v and w is small
        // near vortex, H_n = +-1.

        // cerr
        //   << "H=" << H[pi] << " "
        //   << "modV= " << modV << " "
        //   << "modW=" << modW << " "
        //   << "w=" << Tuple<double>((double *)w,3) << " "
        //   << "V=" << Tuple<T>(&V[vi],3)
        //   << endl;
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// L      -> eigenvalues (lambda) of the corrected pressure hessian
//*****************************************************************************
template <typename T>
void Lambda(int *input, int *output, double *dX,T *V, T *L)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  const int _ninj=_ni*_nj;

  // stencil deltas
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        // J: gradient velocity tensor, (jacobian)
        Matrix<double,3,3> J;
        J <<
          (V[vihi]-V[vilo])/dx[0], (V[vihi+1]-V[vilo+1])/dx[0], V[vihi+2]-V[vilo+2]/dx[0],
          (V[vjhi]-V[vjlo])/dx[1], (V[vjhi+1]-V[vjlo+1])/dx[1], V[vjhi+2]-V[vjlo+2]/dx[1],
          (V[vkhi]-V[vklo])/dx[2], (V[vkhi+1]-V[vklo+1])/dx[2], V[vkhi+2]-V[vklo+2]/dx[2];

        // construct pressure corrected hessian
        Matrix<double,3,3> S=0.5*(J+J.transpose());
        Matrix<double,3,3> W=0.5*(J-J.transpose());
        Matrix<double,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<double,3,1> e;
        SelfAdjointEigenSolver<Matrix<double,3,3> >solver(HP,false);
        e=solver.eigenvalues();

        L[vi]=e(0,0);
        L[vj]=e(1,0);
        L[vk]=e(2,0);

        L[vi]=(L[vi]>=-1E-5&&L[vi]<=1E-5?0.0:L[vi]);
        L[vj]=(L[vj]>=-1E-5&&L[vj]<=1E-5?0.0:L[vj]);
        L[vk]=(L[vk]>=-1E-5&&L[vk]<=1E-5?0.0:L[vk]);

        sort(&L[vi],0,3);
        // cerr << L[vi] << ", "  << L[vj] << ", " << L[vk] << endl;


        // TODO the following code will run faster but I haven't validated it
        /*
        double j11,j12,j13,j21,j22,j23,j31,j32,j33;
        j11=0.5*dvx/dx[0]; j12=0.5*dvy/dx[0]; j13=0.5*dvz/dx[0];
        j21=0.5*dvx/dx[1]; j22=0.5*dvy/dx[1]; j23=0.5*dvz/dx[1];
        j31=0.5*dvx/dx[2]; j32=0.5*dvy/dx[2]; j33=0.5*dvz/dx[2];
        // factor of 1/2 applied here

        // S: strain rate tensor, symetric part of velocity gradient tensor
        double s11,s12,s13,s21,s22,s23,s33;
        s11=j11+j11; s12=j12+j21; s13=j13+j31;
                      s22=j22+j22; s23=j23+j32;
                                  s33=j33+j33;
        // S^2
        double ss11,ss12,ss13,ss22,ss23,ss33;
        ss11=s11*s11+s12*s12+s13*s13; ss12=s11*s12+s12*s22+s13*s23; ss13=s11*s13+s12*s23+s13*s33;
                                      ss22=s12*s12+s22*s22+s23*s23; ss23=s12*s13+s22*s23+s23*s33;
                                                                    ss33=s13*s13+s23*s23+s33*s33;

        // W: spin tensor, anti-symetric part of velocity gradient vector
        double w12,w13,w23;
        w12=j12-j21; w13=j13-j31;
                      w23=j23-j32;
        // W^2
        double ww11,ww12,ww13,ww22,ww23,ww33;
        ww11=w12*w12+w13*w13;  ww12=w13*w23;         ww13=w12*w23;
                                ww22=w12*w12+w23*w23; ww23=w12*w13;
                                                      ww33=w13*w13+w23*w23;

        double sw11,sw12,sw13,sw22,sw23,sw33;
        sw11=ss11+ww11; sw12=ss12+ww12; sw13=ss13+ww13;
                        sw22=ss22+ww22; sw23=ss23+ww23;
                                        sw33=ss33+ww33;

        // compute the eigenvalues of S^2+W^2
        Matrix<double,3,3> M;
        M <<
          sw11, sw12, sw13,
          sw12, sw22, sw23,
          sw13, sw23, sw33;
        Matrix<double,3,1> e;
        SelfAdjointEigenSolver<Matrix<double,3,3> >SAES(M,false);
        e=SAES.eigenvalues();*/
        //
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// L      -> second eigenvalues (lambda-2) of the corrected pressure hessian
//*****************************************************************************
template <typename T>
void Lambda2(int *input, int *output, double *dX,T *V, T *L2)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  const int _ninj=_ni*_nj;

  // stencil deltas
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        // J: gradient velocity tensor, (jacobian)
        Matrix<double,3,3> J;
        J <<
          (V[vihi]-V[vilo])/dx[0], (V[vihi+1]-V[vilo+1])/dx[0], V[vihi+2]-V[vilo+2]/dx[0],
          (V[vjhi]-V[vjlo])/dx[1], (V[vjhi+1]-V[vjlo+1])/dx[1], V[vjhi+2]-V[vjlo+2]/dx[1],
          (V[vkhi]-V[vklo])/dx[2], (V[vkhi+1]-V[vklo+1])/dx[2], V[vkhi+2]-V[vklo+2]/dx[2];

        // construct pressure corrected hessian
        Matrix<double,3,3> S=0.5*(J+J.transpose());
        Matrix<double,3,3> W=0.5*(J-J.transpose());
        Matrix<double,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<double,3,1> e;
        SelfAdjointEigenSolver<Matrix<double,3,3> >solver(HP,false);
        e=solver.eigenvalues();

        // extract lambda-2
        sort(e.data(),0,3);
        L2[pi]=e(1,0);
        L2[pi]=(L2[pi]>=-1E-5&&L2[pi]<=1E-5?0.0:L2[pi]);

        // TODO the following code will run faster but I haven't validated it
        /*
        double j11,j12,j13,j21,j22,j23,j31,j32,j33;
        j11=0.5*dvx/dx[0]; j12=0.5*dvy/dx[0]; j13=0.5*dvz/dx[0];
        j21=0.5*dvx/dx[1]; j22=0.5*dvy/dx[1]; j23=0.5*dvz/dx[1];
        j31=0.5*dvx/dx[2]; j32=0.5*dvy/dx[2]; j33=0.5*dvz/dx[2];
        // factor of 1/2 applied here

        // S: strain rate tensor, symetric part of velocity gradient tensor
        double s11,s12,s13,s21,s22,s23,s33;
        s11=j11+j11; s12=j12+j21; s13=j13+j31;
                      s22=j22+j22; s23=j23+j32;
                                  s33=j33+j33;
        // S^2
        double ss11,ss12,ss13,ss22,ss23,ss33;
        ss11=s11*s11+s12*s12+s13*s13; ss12=s11*s12+s12*s22+s13*s23; ss13=s11*s13+s12*s23+s13*s33;
                                      ss22=s12*s12+s22*s22+s23*s23; ss23=s12*s13+s22*s23+s23*s33;
                                                                    ss33=s13*s13+s23*s23+s33*s33;

        // W: spin tensor, anti-symetric part of velocity gradient vector
        double w12,w13,w23;
        w12=j12-j21; w13=j13-j31;
                      w23=j23-j32;
        // W^2
        double ww11,ww12,ww13,ww22,ww23,ww33;
        ww11=w12*w12+w13*w13;  ww12=w13*w23;         ww13=w12*w23;
                                ww22=w12*w12+w23*w23; ww23=w12*w13;
                                                      ww33=w13*w13+w23*w23;

        double sw11,sw12,sw13,sw22,sw23,sw33;
        sw11=ss11+ww11; sw12=ss12+ww12; sw13=ss13+ww13;
                        sw22=ss22+ww22; sw23=ss23+ww23;
                                        sw33=ss33+ww33;

        // compute the eigenvalues of S^2+W^2
        Matrix<double,3,3> M;
        M <<
          sw11, sw12, sw13,
          sw12, sw22, sw23,
          sw13, sw23, sw33;
        Matrix<double,3,1> e;
        SelfAdjointEigenSolver<Matrix<double,3,3> >SAES(M,false);
        e=SAES.eigenvalues();*/
        //
        }
      }
    }
}

#endif
