/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVVectorImage.h"

//-----------------------------------------------------------------------------
BOVVectorImage::BOVVectorImage(
      MPI_Comm comm,
      MPI_Info hints,
      const char *xFileName,
      const char *yFileName,
      const char *zFileName,
      const char *name)
{
  this->X=new BOVScalarImage(comm,hints,xFileName,name);
  this->Y=new BOVScalarImage(comm,hints,yFileName);
  this->Z=new BOVScalarImage(comm,hints,zFileName);
}

//-----------------------------------------------------------------------------
BOVVectorImage::~BOVVectorImage()
{
  delete this->X;
  delete this->Y;
  delete this->Z;
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVVectorImage &vi)
{
  os << vi.GetName() << endl
     << "  " << vi.X->GetFileName() << " " << vi.X->GetFile() << endl
     << "  " << vi.Y->GetFileName() << " " << vi.X->GetFile() << endl
     << "  " << vi.Z->GetFileName() << " " << vi.X->GetFile() << endl;

  // only one of the file's hints
  MPI_File file=vi.GetXFile();
  if (file)
    {
    cerr << "  Hints:" << endl;
    int WorldRank;
    MPI_Comm_rank(MPI_COMM_WORLD,&WorldRank);
    if (WorldRank==0)
      {
      MPI_Info info;
      char key[MPI_MAX_INFO_KEY];
      char val[MPI_MAX_INFO_KEY];
      MPI_File_get_info(file,&info);
      int nKeys;
      MPI_Info_get_nkeys(info,&nKeys);
      for (int i=0; i<nKeys; ++i)
        {
        int flag;
        MPI_Info_get_nthkey(info,i,key);
        MPI_Info_get(info,key,MPI_MAX_INFO_KEY,val,&flag);
        cerr << "    " << key << "=" << val << endl;
        }
      }
    }

  return os;
}
