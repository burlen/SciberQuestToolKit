/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#ifndef __XMLUtils_h
#define __XMLUtils_h

#include "vtkPVXMLElement.h"
#include "vtkXMLDataElement.h"
#include "SQMacros.h"
#include "postream.h"

#include <iostream>
using std::ws;
#include <sstream>
using std::istringstream;
#include <vector>
using std::vector;
#include <string>
using std::string;

/**
In the element elem return the value of attribute attName
in attValue. Return 0 if successful.
*/
int GetOptionalAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue);

int GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue);

int GetAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue,
      bool optional);

/**
In the element elem return the value of attribute attName
in attValue. Return 0 if successful.
*/
template<typename T, int N>
int GetAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      T *attValue,
      bool optional)
{
  const char *attValueStr=elem->GetAttribute(attName);
  if (attValueStr==NULL)
    {
    if (!optional)
      {
      sqErrorMacro(pCerr(),"No attribute named " << attName << ".");
      return -1;
      }
    else
      {
      return 0;
      }
    }

  T *pAttValue=attValue;
  istringstream is(attValueStr);
  for (int i=0; i<N; ++i)
    {
    if (!is.good())
      {
      sqErrorMacro(pCerr(),"Wrong number of values in " << attName <<".");
      return -1;
      }
    is >> *pAttValue;
    ++pAttValue;
    }
  return 0;
}

template<typename T, int N>
int GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      T *attValue)
{
    return GetAttribute<T,N>(elem,attName,attValue,false);
}

template<typename T, int N>
int GetOptionalAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      T *attValue)
{
    return GetAttribute<T,N>(elem,attName,attValue,true);
}


/**
Return the element named name in the hierarchy root. Return 0 if
unsuccessful.
*/
vtkPVXMLElement *GetRequiredElement(
      vtkPVXMLElement *root,
      const char *name);

vtkPVXMLElement *GetOptionalElement(
      vtkPVXMLElement *root,
      const char *name);

/// extract Delimiting character from stream
istream &
Delim(istream &s,char c);

/// append values Delimted by any combination of ',','\t','\n' or ' '
template<typename S, typename T>
void ExtractValues(
    S dataStr,
    vector<T> &data)
{
  istringstream ss(dataStr);

  while (ss && (ss >> ws) &&
      Delim(ss,',' ) && (ss >> std::ws) &&
      Delim(ss,'\n') && (ss >> std::ws) &&
      Delim(ss,'\t') && (ss >> std::ws))
    {
    T val;
    ss >> val;
    data.push_back(val);
    }
}

template<typename T>
int ExtractValues(
    vtkXMLDataElement *elem,
    const string &xml,
    vector<T> &data
    )
{
  // expecting no nested elements
  if (elem->GetNumberOfNestedElements()>0)
    {
    sqErrorMacro(
        pCerr(),
        << "Error, nested elements are not supported.");
    return -1;
    }

  const char *elemName = elem->GetName();
  string elemClose;
  elemClose += "<";
  elemClose += elemName;
  elemClose += "/>";

  // this gets us to opening tag
  size_t tagAt=elem->GetXMLByteIndex();

  // skip "<tag>" and locate the closing "</tag>"
  size_t elemNameLen=strlen(elemName);
  size_t startsAt=tagAt+elemNameLen+1;
  size_t endsAt=xml.find(elemClose, tagAt);
  if (endsAt==string::npos)
    {
    sqErrorMacro(
        pCerr(),
        << "Error, no closing tag " << elemClose << ".");
    return -1;
    }

  // read in the values
  string text=xml.substr(startsAt,endsAt-startsAt);

  cerr << endl << endl << "text=" << text << endl << endl;

  istringstream ss(text);

  while (ss && (ss >> ws) &&
      Delim(ss,',' ) && (ss >> ws) &&
      Delim(ss,'\n') && (ss >> ws) &&
      Delim(ss,'\t') && (ss >> ws))
    {
    T val;
    ss >> val;
    data.push_back(val);
    }

  cerr << endl << endl << data << endl << endl;

  return 0;
}

#endif

