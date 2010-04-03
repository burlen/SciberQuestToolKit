/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef vtkOOCBOVReader_h
#define vtkOOCBOVReader_h

#include "vtkOOCReader.h"

class vtkDataSet;
class BOVReader;
class BOVTimeStepImage;

/// Implementation class for Brick-Of-Values (BOV) Out-Of-Core (OOC) file access.
/**
Allow one to read in chuncks of data as needed. A specific
chunk of data is identified to be read by providing a point
in which the chunk should reside.
*/
class VTK_EXPORT vtkOOCBOVReader : public vtkOOCReader
{
public:
  static vtkOOCBOVReader *New();
  vtkTypeRevisionMacro(vtkOOCBOVReader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// \section IO \@{
  /**
  Open the dataset for reading. In the case ofg an error 0 is
  returned.
  */
  virtual int Open();

  /**
  Close the dataset.
  */
  virtual void Close();

  /**
  Read the data in a neighborhood bounded by the box.
  */
  virtual vtkDataSet *Read(double b[6]);

  /**
  Read the data in a neighborhood centered about a point, p.
  The number of cells read is implementation specific but is
  controlled by the dimension parameter.
  */
  virtual vtkDataSet *ReadNeighborhood(double p[3], int size);

  /**
  Turn on an array to be read.
  */
  virtual void ActivateArray(const char *arrayName);
  /**
  Turn off an array to be read.
  */
  virtual void DeActivateArray(const char *arrayName);
  virtual void DeActivateAllArrays();
  /// \@}

  /**
  Set the internal reader. The object is coppied and must
  be intialized and ready to read.
  */
  virtual void SetReader(BOVReader *reader);

protected:
  vtkOOCBOVReader();
  virtual ~vtkOOCBOVReader();
private:
  vtkOOCBOVReader(const vtkOOCBOVReader &o);
  const vtkOOCBOVReader &operator=(const vtkOOCBOVReader &o);

private:
  BOVReader *Reader;
  BOVTimeStepImage *Image;
};

#endif
