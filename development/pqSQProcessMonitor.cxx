/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "pqSQProcessMonitor.h"

#define pqSQProcessMonitorDEBUG

#include "MemoryMonitor.h"

#include "pqComponentsExport.h"
#include "pqProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"

#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QMessageBox>
#include <QProgressBar>
#include <QPalette>

#include <unistd.h>

#if defined pqSQProcessMonitorDEBUG
  #include "PrintUtils.h"
#endif
#include "FsUtils.h"

#include <vtkstd/vector>
using vtkstd::vector;
#include <vtkstd/string>
using vtkstd::string;
#include <sstream>
using std::ostringstream;
using std::istringstream;
#include <iostream>
using std::cerr;
using std::endl;

enum {
  PROCESS_TYPE=Qt::UserRole,
  PROCESS_TYPE_INVALID,
  PROCESS_TYPE_LOCAL,
  PROCESS_TYPE_REMOTE,
  RANK_INVALID,
  RANK
};

//*****************************************************************************
template<typename T>
void ClearVectorOfPointers(vector<T *> data)
{
  size_t n=data.size();
  for (size_t i=0; i<n; ++i)
    {
    if (data[i])
      {
      delete data[i];
      }
    }
  data.clear();
}

/// data associated with the host
//=============================================================================
class HostData
{
public:
  HostData(string hostName,unsigned long long capacity);
  HostData(const HostData &other){ *this=other; }
  HostData &operator=(const HostData &other);
  ~HostData(){ this->ClearRankData(); }

  void SetHostName(string name){ this->Name=name; }
  string &GetHostName(){ return this->Name; }

  void SetCapacity(unsigned long long capacity){ this->Capacity=capacity; }
  unsigned long long GetCapacity(){ return this->Capacity; }

  void SetWidget(QProgressBar *widget){ this->Widget=widget; }
  QProgressBar *GetWidget(){ return this->Widget; }
  void InitializeWidget();
  void UpdateWidget();

  RankData *AddRankData(int rank, int pid);
  RankData *GetRankData(int i){ return this->RankData[i]; }
  void ClearRankData();

  unsigned long long GetLoad();
  float GetLoadFraction(){ return (float)this->GetLoad()/(float)this->Capacity; }

private:
  string HostName;
  unsigned long long Capacity;
  QProgressBar *Widget;
  vector<RankData *> Ranks;
};

//-----------------------------------------------------------------------------
HostData::HostData(
      string hostName,
      unsigned long long capacity)
        :
    HostName(hostName),
    Capacity(capacity)
{
  this->InitializeWidget();
}

//-----------------------------------------------------------------------------
const HostData &HostData::operator=(const HostData &other)
{
  if (this==&other) return *this;

  this->HostName=other.HostName;
  this->Capacity=other.Capacity;
  this->Widget=other.Widget;
  this->Ranks=other.Ranks;

  return *this;
}

//-----------------------------------------------------------------------------
void HostData::ClearRankData()
{
  ClearVectorOfPointers<RankData>(this->RankData);
}

//-----------------------------------------------------------------------------
unsigned long long HostData::GetLoad()
{
  unsigned long long load=0;
  size_t n=this->Ranks.size();
  for (size_t i=0; i<n; ++i)
    {
    load+=this->Ranks[i]->GetLoad();
    }
  return load;
}

//-----------------------------------------------------------------------------
void HostData::InitializeWidget()
{
  this->Widget=new QProgressBar;

  this->Widget->setMinimum(0);
  this->Widget->setMaximum(this->Capacity);

  this->UpdateWidget();
}

//-----------------------------------------------------------------------------
void HostData::UpdateWidget()
{
  this->Widget->setValue(this->GetLoad());

  QPalette palette(this->Widget->palette());
  if (this->GetLoadFraction()>0.75)
    {
    // danger -> red
    palette.setColor(QPalette::Highlight,QColor(232,80,80));
    }
  else
    {
    // ok -> green
    palette.setColor(QPalette::Highlight,QColor(66,232,20));
    }
  this->Widget->setPalette(palette);
}

//-----------------------------------------------------------------------------
RankData *HostData::AddRankData(int rank, int pid)
{
  RankData *newRank=new RankData(rank,pid,0,this->Capacity);
  this->Ranks.push_back(newRank);
  return newRank;
}


/// data associated with a rank
//=============================================================================
class RankData
{
public:
  RankData(
      int rank,
      int pid,
      unsigned long long load,
      unsigned long long capacity);

  void SetRank(int rank){ this->Rank=rank; }
  int GetRank(){ return this->Rank; }

  void SetPid(int pid){ this->Pid=pid; }
  int GetPid(){ return this->Pid; }

  void SetLoad(unsigned long long load){ this->Load=load; }
  unsigned long long GetLoad(){ return this->Load; }
  float GetLoadFraction(){ return (float)this->Load/(float)this->Capacity; }

  void SetWidget(QTreeWidgetItem *widget){ this->Widget=widget; }
  QTreeWidgetItem *GetWidget(){ return this->Widget; }
  void UpdateWidget();
  void InitializeWidget();

private:
  int Rank;
  int Pid;
  unsigned long long Load;
  unsigned long long Capacity;
  QTreeWidgetItem *Widget;
  //HostData *Host;
};

//-----------------------------------------------------------------------------
RankData::RankData(
      int rank,
      int pid,
      unsigned long long load,
      unsigned long long capacity)
        :
    Rank(rank),
    Pid(pid),
    Load(load),
    Capacity(capacity)
{
  this->InitializeWidget();
}

//-----------------------------------------------------------------------------
void RankData::UpdateWidget()
{ 
  this->Widget->setValue(this->GetLoad());

  QPalette palette(this->Widget->palette());
  if (this->GetLoadFraction()>0.75)
    {
    // danger -> red
    palette.setColor(QPalette::Highlight,QColor(232,80,80));
    }
  else
    {
    // ok -> green
    palette.setColor(QPalette::Highlight,QColor(66,232,20));
    }
  this->Widget->setPalette(palette);
}

//-----------------------------------------------------------------------------
void RankData::InitializeWidget()
{
  this->Widget=new QProgressBar;

  this->Widget->setMaximumSize(1000,15);

  this->Widget->setMinimum(0);
  this->Widget->setMaximum(this->Capacity);

  this->UpdateWidget();
}

//-----------------------------------------------------------------------------
pqSQProcessMonitor::pqSQProcessMonitor(
        pqProxy* l_proxy,
        QWidget* l_parent)
  : pqNamedObjectPanel(l_proxy, l_parent)
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::pqSQProcessMonitor" << endl;
  #endif
  this->ClientHostData=0;
  this->ClientRankData=0;
  this->ClientMemMonitor=new MemoryMonitor;

  // Construct Qt form.
  this->Form=new pqSQProcessMonitorForm;
  this->Form->setupUi(this);
  // this->Form->addCommand->setIcon(QPixmap(":/pqWidgets/Icons/pqNewItem16.png"));
  // this->Form->execCommand->setIcon(QPixmap(":/pqWidgets/Icons/pqVcrPlay16.png"));
  this->Restore();

  vtkSMProxy* dpProxy=this->referenceProxy()->getProxy();

  // Set up configuration viewer
  this->PullServerConfig();

  // Connect to server side pipeline's UpdateInformation events.
  this->InformationMTime=-1;
  this->VTKConnect=vtkEventQtSlotConnect::New();
  this->VTKConnect->Connect(
      dpProxy,
      vtkCommand::UpdateInformationEvent,
      this, SLOT(UpdateInformationEvent()));
  this->UpdateInformationEvent();

  // These are bad because it gets updated more than when you call
  // modified, see the PV guide.
  //  iuiProp->SetImmediateUpdate(1); set this in constructor
  // vtkSMProperty *iuiProp=dbbProxy->GetProperty("InitializeUI");
  // iuiProp->Modified();

  // set command add,edit,del,exec buttons
  QObject::connect(
        this->Form->execCommand,
        SIGNAL(clicked()),
        this,
        SLOT(ExecCommand()));

  // QObject::connect(
  //       this->Form->addCommand,
  //       SIGNAL(clicked()),
  //       this,
  //       SLOT(AddCommand()));

  QObject::connect(
        this->Form->delCommand,
        SIGNAL(clicked()),
        this,
        SLOT(DelCommand()));

  QObject::connect(
        this->Form->editCommand,
        SIGNAL(toggled(bool)),
        this,
        SLOT(EditCommand(bool)));

  // connect execption handing checks to pv apply button
  QObject::connect(
        this->Form->btSignalHandler,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeDisableAll,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapOverflow,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapUnderflow,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapDivByZero,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapInvalid,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapInexact,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->updateMemUse,
        SIGNAL(released()),
        this,
        SLOT(setModified()));

  // Let the superclass do the undocumented stuff that needs to hapen.
  pqNamedObjectPanel::linkServerManagerProperties();
}

//-----------------------------------------------------------------------------
pqSQProcessMonitor::~pqSQProcessMonitor()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::~pqSQProcessMonitor" << endl;
  #endif

  this->Save();

  delete this->Form;

  this->VTKConnect->Delete();

  delete this->ClientMemMonitor;

  this->ClearClientHostData();
  this->ClearServerHostData();
  this->ClearServerRankData();
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearClientHostData()
{
  if (this->ClientHostData)
    {
    delete this->ClientHostData;
    }
  this->ClientHostData=0;
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearServerHostData()
{
  ClearVectorOfPointers<HostData>(this->Hosts);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearServerRankData()
{
  ClearVectorOfPointers<RankData>(this->Ranks);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::Restore()
{
  QStringList defaults;
  defaults
       << ""
       << "-geometry 200x40 -fg white -bg black -T @HOST@:@PID@"
       << "xterm @XTOPTS@ -e gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @FEURL@ ssh -t @HOST@ gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @FEURL@ ssh -t @HOST@ top"
       << "xterm @XTOPTS@ -e ssh -t @HOST@ gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @HOST@ top"
       << "xterm -e ssh @HOST@ kill -TERM @PID@"
       << "xterm -e ssh @HOST@ kill -KILL @PID@";

  QSettings settings("SciberQuest", "SciVisToolKit");

  QStringList defs=settings.value("ProcessMonitor/defaults",defaults).toStringList();

  this->Form->feURL->setText(defs.at(0));
  this->Form->xtOpts->setText(defs.at(1));
  int n=defs.size();
  for (int i=2; i<n; ++i)
    {
    this->Form->commandCombo->addItem(defs.at(i));
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::Save()
{
  QStringList defs;

  defs
    << this->Form->feURL->text()
    << this->Form->xtOpts->text();

  int nCmds=this->Form->commandCombo->count();
  for (int i=0; i<nCmds; ++i)
    {
    defs << this->Form->commandCombo->itemText(i);
    }

  QSettings settings("SciberQuest","SciVisToolKit");

  settings.setValue("ProcessMonitor/defaults",defs);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::PullServerConfig()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::PullServerConfig" << endl;
  #endif
  vtkSMProxy* dpProxy=this->referenceProxy()->getProxy();

  // client
  QTreeWidgetItem *clientGroup=new QTreeWidgetItem(this->Form->configView,QStringList("paraview"));
  clientGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroup->setExpanded(false);
  clientGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_LOCAL));
  clientGroup->setData(1,RANK,QVariant(RANK_INVALID));

  // TODO modularize client side so that we can support windows.
  // {
  // hostname
  char clientHostName[1024];
  gethostname(clientHostName,1024);
  clientGroup->setText(1,clientHostName);
  // pid
  pid_t clientPid=getpid();
  clientGroup->setText(2,QString("%1").arg(clientPid));

  // local memory use
  this->ClearClientHostData();
  this->ClientHostData
    = new HostData(clientHostName,this->ClientMemMonitor->GetSystemTotal());
  this->ClientRankData=this->ClientHostData->AddRank(0,clientPid);
  this->Form->configView->setItemWidget(clientGroup,3,this->ClientRankData->GetWidget());
  // }

  // server
  QTreeWidgetItem *serverGroup=new QTreeWidgetItem(this->Form->configView,QStringList("pvserver"));
  serverGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  serverGroup->setExpanded(true);
  serverGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_INVALID));
  serverGroup->setData(1,RANK,QVariant(RANK_INVALID));

  // Pull run time configuration from server. The values are transfered
  // in the form of an ascii stream.
  vtkSMStringVectorProperty *csProp
    = dynamic_cast<vtkSMStringVectorProperty*>(dpProxy->GetProperty("ConfigStream"));
  dpProxy->UpdatePropertyInformation(csProp);
  string csBytes=csProp->GetElement(0);

  // cerr << csBytes << endl;

  istringstream is(csBytes);
  if (csBytes.size()>0 && is.good())
    {
    int commSize;
    is >> commSize;

    this->ClearHostData();

    this->HostData.resize(commSize,0);
    this->RankData.resize(commSize,0);

    for (int i=0; i<commSize; ++i)
      {
      string serverHostName;
      unsigned long long serverCapacity;
      int serverRank=i;
      pid_t serverPid;

      is >> serverHostName;
      is >> serverPid;
      is >> serverCapacity;

      HostData *host;

      pair<string,HostData*> ins(serverHostName,0);
      pair<bool,map<string,HostData*>::iterator> ret;
      ret=this->HostData.insert(ins);
      if (!ret.first)
        {
        // new host
        host=new HostData(serverHostName,serverCapacity);
        *(ret.second)=host;
        }
      else
        {
        host=*(ret.second);
        }
      RankData *serverRankData=host->AddRank(serverRank,serverPid);





      QTreeWidgetItem *serverConfig=new QTreeWidgetItem(serverGroup);
      serverConfig->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverConfig->setExpanded(false);
      serverConfig->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_REMOTE));
      serverConfig->setData(1,RANK,QVariant(i));

      serverConfig->setText(0,QString("%1").arg(i));
      serverConfig->setText(1,serverHostName.c_str());
      serverConfig->setText(2,QString("%1").arg(serverPid));

      memUsage=new QProgressBar;
      QPalette palette(memUsage->palette());
      palette.setColor(QPalette::Highlight,QColor(66,232,20));
      memUsage->setPalette(palette);
      memUsage->setMaximumSize(1000,15);
      memUsage->setMinimum(0);
      memUsage->setMaximum(1000);
      memUsage->setValue(0);
      this->Form->configView->setItemWidget(serverConfig,3,memUsage);

      cerr << serverHostName << " " << serverPid << endl;
      }
    }
  else
    {
    cerr << "Error: failed to get configuration stream. Aborting." << endl;
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::UpdateInformationEvent()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::UpdateInformationEvent" << endl;
  #endif

//   vtkSMProxy* reader = this->referenceProxy()->getProxy();
//   reader->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));
// 
//   int stamp = vtkSMPropertyHelper(reader, "SILUpdateStamp").GetAsInt();

  vtkSMProxy* pmProxy=this->referenceProxy()->getProxy();

  // see if there has been an update to the server side.
  vtkSMIntVectorProperty *infoMTimeProp
    =dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("GetInformationMTime"));
  pmProxy->UpdatePropertyInformation(infoMTimeProp);
  int infoMTime=infoMTimeProp->GetElement(0);

  cerr << "infoMTime=" << infoMTime << endl;

  if (infoMTime>this->InformationMTime)
    {
    this->InformationMTime=infoMTime;

    vtkSMStringVectorProperty *memProp
      =dynamic_cast<vtkSMStringVectorProperty *>(pmProxy->GetProperty("MemoryUseStream"));

    pmProxy->UpdatePropertyInformation(memProp);

    string stream=memProp->GetElement(0);
    istringstream is(stream);

    cerr << stream << endl;

    vector<float> memUse;

    if (stream.size()>0 && is.good())
      {
      int commSize;
      is >> commSize;

      memUse.resize(commSize);

      for (int i=0; i<commSize; ++i)
        {
        is >> memUse[i];
        }
      }

    QTreeWidgetItemIterator it(this->Form->configView);
    while (*it)
      {
      int nodeType=(*it)->data(0,PROCESS_TYPE).toInt(0);
      if (nodeType==PROCESS_TYPE_REMOTE)
        {
        QProgressBar *memUseWid
          = dynamic_cast<QProgressBar*>(this->Form->configView->itemWidget(*it,3));

        int rank=(*it)->data(1,RANK).toInt(0);
        memUseWid->setValue((int)(1000.0*memUse[rank]));
        }
      else
      if (nodeType==PROCESS_TYPE_LOCAL)
        {
        QProgressBar *memUseWid
          = dynamic_cast<QProgressBar*>(this->Form->configView->itemWidget(*it,3));

        float localMemUse=this->ClientMemMonitor->GetVmRSSPercent();
        memUseWid->setValue((int)(1000.0*localMemUse));
        }

      ++it;
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::AddCommand()
{
//   int idx=this->Form->commandCombo->count();
//   this->Form->commandCombo->addItem("NEW COMMAND");
//   this->Form->commandCombo->setCurrentIndex(idx);
//   if (!this->Form->editCommand->isChecked())
//     {
//     this->Form->editCommand->click();
//     }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::DelCommand()
{
  int idx=this->Form->commandCombo->currentIndex();
  this->Form->commandCombo->removeItem(idx);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::EditCommand(bool state)
{
  this->Form->commandCombo->setEditable(state);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ExecCommand()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::ExecCommand" << endl;
  #endif

  QTreeWidgetItem *item=this->Form->configView->currentItem();
  if (item)
    {
    bool ok;
    int type=item->data(0,PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case PROCESS_TYPE_LOCAL:
      case PROCESS_TYPE_REMOTE:
        {
        string cmd=(const char*)this->Form->commandCombo->currentText().toAscii();

        string feURLStr(this->Form->feURL->text().toAscii());
        SearchAndReplace(string("@FEURL@"),feURLStr,cmd);

        string xtOptsStr(this->Form->xtOpts->text().toAscii());
        SearchAndReplace(string("@XTOPTS@"),xtOptsStr,cmd);

        string hostNameStr((const char *)item->text(1).toAscii());
        SearchAndReplace(string("@HOST@"),hostNameStr,cmd);

        string pidStr((const char *)item->text(2).toAscii());
        SearchAndReplace(string("@PID@"),pidStr,cmd);

        vector<string> argStrs;
        istringstream is(cmd);
        while (is.good())
            {
            string argStr;
            is >> argStr;
            argStrs.push_back(argStr);
            }

        pid_t childPid=fork();
        if (childPid==0)
          {

          int nArgStrs=argStrs.size();
          char **args=(char **)malloc((nArgStrs+1)*sizeof(char *));
          for (int i=0; i<nArgStrs; ++i)
            {
            int argLen=argStrs[i].size();
            args[i]=(char *)malloc((argLen+1)*sizeof(char));
            strncpy(args[i],argStrs[i].c_str(),argLen+1);
            #if defined pqSQProcessMonitorDEBUG
            cerr << "[" << args[i] << "]" << endl;
            #endif
            }
          args[nArgStrs]=0;
          execvp(args[0],args);
          QMessageBox ebx;
          ebx.setText("Exec failed.");
          ebx.exec();
          /// exit(0);
          }
        else
          {
          // // client
          // QTreeWidgetItem *clientGroup=new QTreeWidgetItem(this->Form->configView);
          // clientGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
          // clientGroup->setExpanded(false);
          // clientGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_LOCAL));
          // // exec name
          // clientGroup->setText(0,argStrs[0].c_str());
          // // hostname
          // char clientHostName[1024];
          // gethostname(clientHostName,1024);
          // clientGroup->setText(1,clientHostName);
          // // pid
          // clientGroup->setText(2,QString("%1").arg(childPid));
          }
        }
        break;
      case PROCESS_TYPE_INVALID:
      default:
        QMessageBox ebx;
        ebx.setText("Could not exec. No process selected.");
        ebx.exec();
        return;
        break;
      }
    }
}

// //-----------------------------------------------------------------------------
// void pqSQProcessMonitor::Signal()
// {
//   #if defined pqSQProcessMonitorDEBUG
//   cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::Signal" << endl;
//   #endif
// 
//   QTreeWidgetItem *item=this->Form->configView->currentItem();
//   if (item)
//     {
//     bool ok;
//     int type=item->data(0,PROCESS_TYPE).toInt(&ok);
//     if (!ok) return;
//     switch (type)
//       {
//       case PROCESS_TYPE_LOCAL:
//       case PROCESS_TYPE_REMOTE:
//         {
//         pid_t childPid=fork();
//         if (childPid==0)
//           {
//           string pidStr((const char *)item->text(2).toAscii());
//           string sigName("-");
//           sigName+=(const char *)this->Form->signalCombo->currentText().toAscii();
//           const char *const args[]={"kill",sigName.c_str(),pidStr.c_str(),0};
//           execvp(args[0],( char* const*)args);
// 
// //           // use linux kill command
// //           const char exe[]="kill";
// //           // signame
// //           int sigNameLen=sigName.size()+1;
// //           args[0]=(char*)malloc(sigNameLen*sizeof(char));
// //           strncpy(args[0],sigName.c_str(),sigNameLen);
// //           // pid
// //           string pidStr((const char *)item->text(2).toAscii());
// //           int pidStrLen=pidStr.size()+1;
// //           args[1]=(char*)malloc(pidStrLen*sizeof(char));
// //           strncpy(args[0],sigName.c_str(),pidStrLen);
// //           // end
// //           args[2]=0;
// //           // exec
// //           execvp(exe,args);
//           // should never get here
//           QMessageBox ebx;
//           ebx.setText("Exec failed.");
//           ebx.exec();
//           /// exit(0);
//           }
//         }
//         break;
//       case PROCESS_TYPE_INVALID:
//       default:
//         QMessageBox ebx;
//         ebx.setText("Could not signal. No process selected.");
//         ebx.exec();
//         return;
//         break;
//       }
//     }
// }

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::accept()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::accept" << endl;
  #endif

  // Let our superclass do the undocumented stuff that needs to be done.
  pqNamedObjectPanel::accept();


  vtkSMProxy* l_proxy=this->referenceProxy()->getProxy();

  vtkSMIntVectorProperty *prop=0;

  prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableBacktraceHandler"));
  prop->SetElement(0,this->Form->btSignalHandler->isChecked());
  //l_proxy->UpdateProperty("EnableBacktraceHandler");


  bool disableFPE=this->Form->fpeDisableAll->isChecked();
  if (disableFPE)
    {
    prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableFE_ALL"));
    prop->SetElement(0,0);
    prop->Modified();
    }
  else
    {
    prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableFE_DIVBYZERO"));
    prop->SetElement(0,this->Form->fpeTrapDivByZero->isChecked());
    prop->Modified(); // force push

    prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableFE_INEXACT"));
    prop->SetElement(0,this->Form->fpeTrapInexact->isChecked());
    prop->Modified();

    prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableFE_INVALID"));
    prop->SetElement(0,this->Form->fpeTrapInvalid->isChecked());
    prop->Modified();

    prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableFE_OVERFLOW"));
    prop->SetElement(0,this->Form->fpeTrapOverflow->isChecked());
    prop->Modified();

    prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("EnableFE_UNDERFLOW"));
    prop->SetElement(0,this->Form->fpeTrapUnderflow->isChecked());
    prop->Modified();
    }

  // force a refresh
  ++this->InformationMTime;
  prop=dynamic_cast<vtkSMIntVectorProperty *>(l_proxy->GetProperty("SetInformationMTime"));
  prop->SetElement(0,this->InformationMTime);
  prop->Modified();

  l_proxy->UpdateVTKObjects();
}





/// VTK stuffs
//   // Connect to server side pipeline's UpdateInformation events.
//   this->VTKConnect=vtkEventQtSlotConnect::New();
//   this->VTKConnect->Connect(
//       dbbProxy,
//       vtkCommand::UpdateInformationEvent,
//       this, SLOT(UpdateInformationEvent()));
//   // Get our initial state from the server side. In server side RequestInformation
//   // the database view is encoded in vtkInformationObject. We are relying on the 
//   // fact that there is a pending event waiting for us.
//   this->UpdateInformationEvent();

// 
//   // These connection let PV know that we have changed, and makes the apply button
//   // is activated.
//   QObject::connect(
//       this->Form->DatabaseView,
//       SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//       this, SLOT(setModified()));

// vtkSMProxy* dbbProxy=this->referenceProxy()->getProxy();
/// Example of how to get stuff from the server side.
// vtkSMIntVectorProperty *serverDVMTimeProp
//   =dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("DatabaseViewMTime"));
// dbbProxy->UpdatePropertyInformation(serverDVMTimeProp);
// const int *serverDVMTime=dvmtProp->GetElement(0);
/// Example of how to get stuff from the XML configuration.
// vtkSMStringVectorProperty *ppProp
//   =dynamic_cast<vtkSMStringVectorProperty *>(dbbProxy->GetProperty("PluginPath"));
// const char *pp=ppProp->GetElement(0);
// this->Form->PluginPath->setText(pp);
/// Imediate update example.
// These are bad because it gets updated more than when you call
// modified, see the PV guide.
//  iuiProp->SetImmediateUpdate(1); set this in constructor
// vtkSMProperty *iuiProp=dbbProxy->GetProperty("InitializeUI");
// iuiProp->Modified();
/// Do some changes and force a push.
// vtkSMIntVectorProperty *meshProp
//   = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("PushMeshId"));
// meshProp->SetElement(meshIdx,meshId);
// ++meshIdx;
// dbbProxy->UpdateProperty("PushMeshId");
/// Catch all, update anything else that may need updating.
// dbbProxy->UpdateVTKObjects();
/// How a wiget in custom panel tells PV that things need to be applied
// QObject::connect(
//     this->Form->DatabaseView,
//     SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//     this, SLOT(setModified()));
  // Pull run time configuration from server. The values are transfered
  // in the form of an ascii stream.
//   vtkSMIntVectorProperty *pidProp
//     = dynamic_cast<vtkSMIntVectorProperty*>(dpProxy->GetProperty("Pid"));
//   dpProxy->UpdatePropertyInformation(pidProp);
//   pid_t p=pidProp->GetElement(0);
//   cerr << "PID=" << p << endl;
/*
  vtkSMProxy* reader = this->referenceProxy()->getProxy();
  reader->UpdatePropertyInformation(reader->GetProperty("Pid"));
  int stamp = vtkSMPropertyHelper(reader, "Pid").GetAsInt();
  cerr << stamp;*/
/// how to get something from the server.
// dbbProxy->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));
// int stamp = vtkSMPropertyHelper(dbbProxy, "SILUpdateStamp").GetAsInt();