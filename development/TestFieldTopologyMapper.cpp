
#include "vtkSphereSource.h"
#include "vtkPlaneSource.h"
#include "vtkSQBOVReader.h"
#include "vtkSQFieldTracer.h"
#include "vtkSQVolumetricSource.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"

#include <iostream>
using std::cerr;

#include <mpi.h>

int main(int argc, char **argv)
{
  MPI_Init(&argc,&argv);

  if (argc<2)
    {
    cerr << "Error: No file name at $1." << endl;
    return 1;
    }

  if (argc<3)
    {
    cerr << "Error: No seed selection (1,2) at $2." << endl;
    return 1;
    }
  int seedSelection=atoi(argv[2]);
  enum {
    PLANE=1,
    VOLUME=2
    };

  // ooc reader
  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(1);
  r->SetFileName(argv[1]);
  r->SetPointArrayStatus("vi",1);

  // terminator
  vtkSphereSource *s1=vtkSphereSource::New();
  s1->SetCenter(3.5,6.0,1.5);
  s1->SetRadius(1.0);
  s1->SetThetaResolution(32);
  s1->SetPhiResolution(32);

  vtkSphereSource *s2=vtkSphereSource::New();
  s2->SetCenter(3.5,5.0,2.0);
  s2->SetRadius(0.5);
  s2->SetThetaResolution(32);
  s2->SetPhiResolution(32);

  vtkSphereSource *s3=vtkSphereSource::New();
  s3->SetCenter(3.5,1.0,6.0);
  s3->SetRadius(0.8);
  s3->SetThetaResolution(32);
  s3->SetPhiResolution(32);

  vtkSphereSource *s4=vtkSphereSource::New();
  s4->SetCenter(3.5,1.5,5.0);
  s4->SetRadius(0.25);
  s4->SetThetaResolution(32);
  s4->SetPhiResolution(32);

  // seed points
  vtkAlgorithm *sp=0;
  switch (seedSelection)
    {
    case PLANE:
      {
      vtkPlaneSource *p=vtkPlaneSource::New();
      p->SetOrigin(1.0,3.5,0.25);
      p->SetPoint1(6.0,3.5,0.25);
      p->SetPoint2(1.0,3.5,4.75);
      p->SetXResolution(100);
      p->SetYResolution(100);
      sp=p;
      }
      break;

    case VOLUME:
      {
      vtkSQVolumetricSource *v=vtkSQVolumetricSource::New();
      v->SetOrigin(1.0,3.0,0.25);
      v->SetPoint1(6.0,3.0,0.25);
      v->SetPoint2(1.0,4.0,0.25);
      v->SetPoint3(1.0,3.0,4.75);
      v->SetNCells(60,12,60);
      sp=v;
      }
      break;

    default:
      cerr << "Error: invalid seed point selection " << seedSelection << endl;
      return 1;
      break;
    }

  // field topology mapper
  vtkSQFieldTracer *ftm=vtkSQFieldTracer::New();
  ftm->SetMode(vtkSQFieldTracer::MODE_TOPOLOGY);
  ftm->SetIntegratorType(vtkSQFieldTracer::INTEGRATOR_RK45);
  ftm->SetUseDynamicScheduler(1);
  ftm->SetSqueezeColorMap(1);
  ftm->AddInputConnection(0,r->GetOutputPort(0));
  ftm->AddInputConnection(1,sp->GetOutputPort(0));
  ftm->AddInputConnection(2,s1->GetOutputPort(0));
  ftm->AddInputConnection(2,s2->GetOutputPort(0));
  ftm->AddInputConnection(2,s3->GetOutputPort(0));
  ftm->AddInputConnection(2,s4->GetOutputPort(0));
  ftm->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"vi");

  #pragma message("TODO -- Set update extent properly. Valgrind reports many errors.")

  r->Delete();
  sp->Delete();
  s1->Delete();
  s2->Delete();
  s3->Delete();
  s4->Delete();

  ftm->Update();
  vtkDataSet *map=ftm->GetOutput();
  map->GetCellData()->SetActiveScalars("IntersectColor");

  vtkDataSetSurfaceFilter *surf=vtkDataSetSurfaceFilter::New();
  surf->SetInputConnection(0,ftm->GetOutputPort(0));
  ftm->Delete();

  vtkPolyDataMapper *pdm=vtkPolyDataMapper::New();
  pdm->SetInputConnection(0,surf->GetOutputPort(0));
  pdm->SetColorModeToMapScalars();
  pdm->SetScalarModeToUseCellData();
  pdm->SetScalarRange(map->GetCellData()->GetArray("IntersectColor")->GetRange());
  surf->Delete();

  vtkActor *act=vtkActor::New();
  act->SetMapper(pdm);
  pdm->Delete();

  vtkRenderer *ren=vtkRenderer::New();
  ren->AddActor(act);
  act->Delete();

  vtkCamera *cam=ren->GetActiveCamera();
  cam->SetFocalPoint(3.5,3.5,3.5);
  cam->SetPosition(3.5,15.0,3.5);
  cam->ComputeViewPlaneNormal();
  cam->SetViewUp(0.0,0.0,1.0);
  cam->OrthogonalizeViewUp();
  ren->ResetCamera();

  vtkRenderWindow *rwin=vtkRenderWindow::New();
  rwin->AddRenderer(ren);
  ren->Delete();

  vtkRenderWindowInteractor *rwi=vtkRenderWindowInteractor::New();
  rwi->SetRenderWindow(rwin);
  rwin->Delete();
  rwin->Render();
  rwi->Start();
  rwi->Delete();

  MPI_Finalize();

  return 0;
}

