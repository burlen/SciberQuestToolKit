#include<iostream>
using std::ostream;
using std::cerr;
using std::endl;
#include<fstream>
using std::ofstream;
#include<sstream>
using std::ostringstream;
#include<string>
using std::string;
#include<cstring>
#include<cstdlib>
#include<cmath>
#include<fcntl.h>
#include<limits>

#include "Tuple.hxx"

//*****************************************************************************
bool IsNan(float b_x, float b_y, float b_z)
{
  return isnan(b_x) || isnan(b_y) || isnan(b_z);
}

//*****************************************************************************
void Init(float b_0, float *b_x, float *b_y, float *b_z, int n)
{
  for (int i=0; i<n; ++i)
    {
    b_x[i]=b_0;
    b_y[i]=b_0;
    b_z[i]=b_0;
    }
}

//*****************************************************************************
float theta(float x, float y)
{
  float theta=atan(fabs(y)/fabs(x));
  float pi=3.14159265358979;

   if (x<=0 && y>=0) // 2nd
    {
    theta=pi-theta;
    }
  else
  if (x<=0 && y<=0) // 3rd
    {
    theta+=pi;
    }
  else
  if (x>=0 && y<=0) // 4th
    {
    theta=2*pi-theta;
    }

  return theta;
}

//*****************************************************************************
bool Find(
      int *domain,
      int *patch,
      float *b_x,
      float *b_y,
      float *b_z,
      float val)
{
  bool has=false;

  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  //int nk=domain[5]-domain[4]+1;

  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        int q=k*ni*nj+j*ni+i;
        if (b_x[q]==val || b_y[q]==val || b_z[q]==val)
          {
          has=true;
          cerr
            << __LINE__ << " FOUND val=" << val
            << " at " << Tuple<int>(i,j,k) << endl;
          }
        }
      }
    }
  return has;
}

//*****************************************************************************
bool HasNans(int *domain, int *patch, float *b_x, float *b_y, float *b_z)
{
  bool has=false;

  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  //int nk=domain[5]-domain[4]+1;

  // translate so center of vortex is on center of patch
  float itr=ni/2.0; 
  float jtr=nj/2.0;

  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        int q=k*ni*nj+j*ni+i;
        float x=i-itr;
        float y=j-jtr;
        float z=k;
        if (IsNan(b_x[q],b_y[q],b_z[q]))
          {
          has=true;
          cerr
            << __LINE__ << " ERROR NAN." << endl
            << "I=" << Tuple<int>(i,j,k) << endl
            << "X=" << Tuple<float>(x,y,z) << endl;
          }
        }
      }
    }
  return has;
}


//*****************************************************************************
int InitMagneticIslands(
      int *domain,
      int *patch,
      float *b_x,
      float *b_y,
      float *b_z,
      float *X0,
      float *dX,
      float kk,
      float kk_y,
      float kk_z,
      float b_g,
      float theta,
      float eps,
      float x_s,
      int n_isl)
{
  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  //int nk=domain[5]-domain[4]+1;

  float x[3]={0.0};

  x[2]=X0[2]+dX[2]*patch[4];
  for (int k=patch[4]; k<=patch[5]; ++k,x[2]+=dX[2])
    {
    x[1]=X0[1]+dX[1]*patch[2];
    for (int j=patch[2]; j<=patch[3]; ++j,x[1]+=dX[1])
      {
      x[0]=X0[0]+dX[0]*patch[0];
      for (int i=patch[0]; i<=patch[1]; ++i,x[0]+=dX[0])
        {
        int q=k*ni*nj+j*ni+i;

        float xxs=x[0]-x_s;
        float kyz=kk_y*x[1]+kk_z*x[2];
        float tcc=cos(kyz)*tanh(xxs)/cosh(xxs);

        b_x[q]=eps*kk*sin(kyz)/cosh(xxs);
        b_y[q]=b_g-eps*sin(theta)*tcc;
        b_z[q]=tanh(x[0])-eps*cos(theta)*tcc;

        //cerr << "x=" << Tuple<float>(x,3) << " B=" << Tuple<float>(b_x[q],b_y[q],b_z[q]) << endl;
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  if (argc<8)
    {
    cerr
      << "Error:" << endl
      << "usage: " << argv[0] << endl
      << "  1) nx" << endl
      << "  2) k" << endl
      << "  3) theta" << endl
      << "  4) B_g" << endl
      << "  5) eps" << endl
      << "  6) N" << endl
      << "  7) /path/to/dataset" << endl
      << "  8) stepNo" << endl
      << endl;
    return 1;
    }

  int nx=atoi(argv[1]);
  float k=atof(argv[2]);
  float theta=atof(argv[3])*M_PI/180.0;
  float b_g=atof(argv[4]);
  float eps=atof(argv[5]);
  int n_i=atoi(argv[6]);
  char *path=argv[7];
  int stepNo=0;
  if (argc>8)
    {
    stepNo=atoi(argv[8]);
    }

  int ny,nz;
  ny=nz=n_i*nx;

  int domain[6]={0,nx-1,0,ny-1,0,nz-1};
  float X0[3]={-M_PI,-M_PI*n_i,-M_PI*n_i};
  float dX[3]={
    2.0*M_PI/(nx-1),
    2.0*M_PI*n_i/(ny-1),
    2.0*M_PI*n_i/(nz-1)};

  float k_y=k*sin(theta);
  float k_z=k*cos(theta);
  float x_s=-atanh(tan(theta)*b_g);

  ostringstream comment;
  comment
    << "# Magnetic Islands " << endl
    << "# nX  = " << Tuple<float>(nx,ny,nz) << endl
    << "# theta = " << theta << endl
    << "# k   = " << k << endl
    << "# k_y = " << k_y << endl
    << "# k_z = " << k_z << endl
    << "# B_g = " << b_g << endl
    << "# eps = " << eps << endl
    << "# x_s = " << x_s << endl
    << endl;
  cerr << comment.str() << endl;

  int n=nx*ny*nz;
  float *b_x=(float *)malloc(n*sizeof(float));
  float *b_y=(float *)malloc(n*sizeof(float));
  float *b_z=(float *)malloc(n*sizeof(float));

  float sentinel=-555.555;
  Init(sentinel,b_x,b_y,b_z,n);

  InitMagneticIslands(
        domain,
        domain,
        b_x,b_y,b_z,
        X0,dX,
        k,k_y,k_z,
        b_g,
        theta,
        eps,
        x_s,
        n_i);

  // check for numerical problems.
  HasNans(domain,domain,b_x,b_y,b_z);
  Find(domain,domain,b_x,b_y,b_z,sentinel);

  // write out the dataset
  n*=sizeof(float);
  int mode=S_IRUSR|S_IWUSR|S_IRGRP;
  int flags=O_WRONLY|O_CREAT|O_TRUNC;

  umask(S_IROTH|S_IWOTH); 

  ostringstream fn;
  fn << path << "/";
  mkdir(fn.str().c_str(),S_IRWXU|S_IXGRP|S_IXOTH);

  fn.str("");
  fn << path << "/bx_" << stepNo << ".gda";
  int xfd=open(fn.str().c_str(),flags,mode);
  write(xfd,b_x,n);
  close(xfd);

  fn.str("");
  fn << path << "/by_" << stepNo << ".gda";
  int yfd=open(fn.str().c_str(),flags,mode);
  write(yfd,b_y,n);
  close(yfd);

  fn.str("");
  fn << path << "/bz_" << stepNo << ".gda";
  int zfd=open(fn.str().c_str(),flags,mode);
  write(zfd,b_z,n);
  close(zfd);

  fn.str("");
  fn << path << "/" << path << ".bov";
  ofstream os(fn.str().c_str());
  os
    << comment.str()
    << endl
    << "nx=" << nx    << ", ny=" << ny    << ", nz=" << nz    << endl
    << "x0=" << X0[0] << ", y0=" << X0[1] << ", z0=" << X0[2] << endl
    << "dx=" << dX[0] << ", dy=" << dX[1] << ", dz=" << dX[2] << endl
    << endl;
  os.close();

  // link for ooc meta-reader.
  ostringstream lt;
  lt << path << ".bov";
  string ln(fn.str().c_str());
  ln+="m";
  symlink(lt.str().c_str(),ln.c_str());

  free(b_x);
  free(b_y);
  free(b_z);

  return 0;
}

// g++ -g -Wall -I ../ genMagneticIsland.cpp -o genMagneticIsland
