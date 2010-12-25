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

#include <vtkstd/map>
using std::map;
using std::pair;
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
  PROCESS_TYPE_CLIENT,
  PROCESS_TYPE_SERVER_HOST,
  PROCESS_TYPE_SERVER_RANK,
  PROCESS_ID,
  HOST_NAME,
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

  void SetLoadWidget(QProgressBar *widget){ this->LoadWidget=widget; }
  QProgressBar *GetLoadWidget(){ return this->LoadWidget; }
  void UpdateLoadWidget();
  void InitializeLoadWidget();

private:
  int Rank;
  int Pid;
  unsigned long long Load;
  unsigned long long Capacity;
  QProgressBar *LoadWidget;
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
  this->InitializeLoadWidget();
}

//-----------------------------------------------------------------------------
void RankData::UpdateLoadWidget()
{
  this->LoadWidget->setValue(this->GetLoad());
  this->LoadWidget->setFormat(QString("%1%").arg(this->GetLoadFraction()*100.0, 0,'f',2));

  QPalette palette(this->LoadWidget->palette());
  if (this->GetLoadFraction()>0.75)
    {
    // danger -> red
    palette.setColor(QPalette::Highlight,QColor(232,40,40));
    }
  else
    {
    // ok -> green
    palette.setColor(QPalette::Highlight,QColor(66,232,20));
    }
  this->LoadWidget->setPalette(palette);
}

//-----------------------------------------------------------------------------
void RankData::InitializeLoadWidget()
{
  this->LoadWidget=new QProgressBar;

  this->LoadWidget->setMaximumSize(95,15);

  QFont font(this->LoadWidget->font());
  font.setPointSize(8);
  this->LoadWidget->setFont(font);

  this->LoadWidget->setMinimum(0);
  this->LoadWidget->setMaximum(this->Capacity);

  this->UpdateLoadWidget();
}

/// data associated with the host
//=============================================================================
class HostData
{
public:
  HostData(string hostName,unsigned long long capacity);
  HostData(const HostData &other){ *this=other; }
  const HostData &operator=(const HostData &other);
  ~HostData(){ this->ClearRankData(); }

  void SetHostName(string name){ this->HostName=name; }
  string &GetHostName(){ return this->HostName; }

  void SetCapacity(unsigned long long capacity){ this->Capacity=capacity; }
  unsigned long long GetCapacity(){ return this->Capacity; }

  void SetTreeItem(QTreeWidgetItem *item){ this->TreeItem=item; }
  QTreeWidgetItem *GetTreeItem(){ return this->TreeItem; }

  void SetLoadWidget(QProgressBar *widget){ this->LoadWidget=widget; }
  QProgressBar *GetLoadWidget(){ return this->LoadWidget; }
  void InitializeLoadWidget();
  void UpdateLoadWidget();

  RankData *AddRankData(int rank, int pid);
  RankData *GetRankData(int i){ return this->Ranks[i]; }
  void ClearRankData();

  unsigned long long GetLoad();
  float GetLoadFraction(){ return (float)this->GetLoad()/(float)this->Capacity; }

private:
  string HostName;
  unsigned long long Capacity;
  QProgressBar *LoadWidget;
  QTreeWidgetItem *TreeItem;
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
  this->InitializeLoadWidget();
}

//-----------------------------------------------------------------------------
const HostData &HostData::operator=(const HostData &other)
{
  if (this==&other) return *this;

  this->HostName=other.HostName;
  this->Capacity=other.Capacity;
  this->LoadWidget=other.LoadWidget;
  this->TreeItem=other.TreeItem;
  this->Ranks=other.Ranks;

  return *this;
}

//-----------------------------------------------------------------------------
void HostData::ClearRankData()
{
  ClearVectorOfPointers<RankData>(this->Ranks);
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
void HostData::InitializeLoadWidget()
{
  this->LoadWidget=new QProgressBar;

  this->LoadWidget->setMaximumSize(95,15);

  QFont font(this->LoadWidget->font());
  font.setPointSize(8);
  this->LoadWidget->setFont(font);

  this->LoadWidget->setMinimum(0);
  this->LoadWidget->setMaximum(this->Capacity);

  this->UpdateLoadWidget();
}

//-----------------------------------------------------------------------------
void HostData::UpdateLoadWidget()
{
  this->LoadWidget->setValue(this->GetLoad());
  this->LoadWidget->setFormat(QString("%1%").arg(this->GetLoadFraction()*100.0, 0,'f',2));

  QPalette palette(this->LoadWidget->palette());
  if (this->GetLoadFraction()>0.75)
    {
    // danger -> red
    palette.setColor(QPalette::Highlight,QColor(232,40,40));
    }
  else
    {
    // ok -> green
    palette.setColor(QPalette::Highlight,QColor(66,232,20));
    }
  this->LoadWidget->setPalette(palette);
}

//-----------------------------------------------------------------------------
RankData *HostData::AddRankData(int rank, int pid)
{
  RankData *newRank=new RankData(rank,pid,0,this->Capacity);
  this->Ranks.push_back(newRank);
  return newRank;
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
  this->ClientHost=0;
  this->ClientRank=0;
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

  this->ClearClientHost();
  this->ClearServerHosts();
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearClientHost()
{
  if (this->ClientHost)
    {
    delete this->ClientHost;
    }
  this->ClientHost=0;
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearServerHosts()
{
  map<string,HostData*>::iterator it=this->ServerHosts.begin();
  map<string,HostData*>::iterator end=this->ServerHosts.end();
  while (it!=end)
    {
    if ((*it).second)
      {
      delete (*it).second;
      }
    ++it;
    }
  this->ServerHosts.clear();
  this->ServerRanks.clear();
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
  QTreeWidgetItem *clientGroupItem=new QTreeWidgetItem(this->Form->configView);
  clientGroupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroupItem->setExpanded(true);
  clientGroupItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_INVALID));
  clientGroupItem->setData(1,RANK,QVariant(RANK_INVALID));
  clientGroupItem->setText(0,QString("paraview"));

  // TODO modularize client side so that we can support windows.
  // {
  char clientHostName[1024];
  gethostname(clientHostName,1024);
  pid_t clientPid=getpid();
  this->ClearClientHost();
  this->ClientHost
    = new HostData(clientHostName,this->ClientMemMonitor->GetSystemTotal());
  this->ClientRank=this->ClientHost->AddRankData(0,clientPid);
  this->ClientRank->SetLoad(this->ClientMemMonitor->GetVmRSS());
  this->ClientRank->UpdateLoadWidget();
  // }

  QTreeWidgetItem *clientHostItem=new QTreeWidgetItem(clientGroupItem);
  clientHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientHostItem->setExpanded(true);
  clientHostItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_INVALID));
  clientHostItem->setText(0,clientHostName);

  QTreeWidgetItem *clientRankItem=new QTreeWidgetItem(clientHostItem);
  clientRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientRankItem->setExpanded(true);
  clientRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT));
  clientRankItem->setData(1,HOST_NAME,QVariant(clientHostName));
  clientRankItem->setData(2,PROCESS_ID,QVariant(clientPid));
  clientRankItem->setText(0,QString("0:%1").arg(clientPid));
  this->Form->configView->setItemWidget(clientRankItem,1,this->ClientRank->GetLoadWidget());

  // server
  QTreeWidgetItem *serverGroup=new QTreeWidgetItem(this->Form->configView,QStringList("pvserver"));
  serverGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  serverGroup->setExpanded(true);
  serverGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_INVALID));

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

    this->ClearServerHosts();
    this->ServerRanks.resize(commSize,0);

    for (int i=0; i<commSize; ++i)
      {
      string serverHostName;
      unsigned long long serverCapacity;
      pid_t serverPid;

      is >> serverHostName;
      is >> serverPid;
      is >> serverCapacity;

      HostData *serverHost;

      pair<string,HostData*> ins(serverHostName,0);
      pair<map<string,HostData*>::iterator,bool> ret;
      ret=this->ServerHosts.insert(ins);
      if (ret.second)
        {
        // new host
        serverHost=new HostData(serverHostName,serverCapacity);
        ret.first->second=serverHost;

        QTreeWidgetItem *serverHostItem=new QTreeWidgetItem(serverGroup);
        serverHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        serverHostItem->setExpanded(true);
        serverHostItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_INVALID));
        serverHostItem->setText(0,serverHostName.c_str());
        this->Form->configView->setItemWidget(serverHostItem,1,serverHost->GetLoadWidget());

        serverHost->SetTreeItem(serverHostItem);
        }
      else
        {
        serverHost=ret.first->second;
        }
      RankData *serverRank=serverHost->AddRankData(i,serverPid);
      this->ServerRanks[i]=serverRank;

      QTreeWidgetItem *serverRankItem=new QTreeWidgetItem(serverHost->GetTreeItem());
      serverRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverRankItem->setExpanded(false);
      serverRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_RANK));
      serverRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_HOST));
      serverRankItem->setData(1,HOST_NAME,QVariant(serverHostName.c_str()));
      serverRankItem->setData(2,PROCESS_ID,QVariant(serverPid));
      serverRankItem->setText(0,QString("%1:%2").arg(i).arg(serverPid));
      this->Form->configView->setItemWidget(serverRankItem,1,serverRank->GetLoadWidget());

      cerr << serverHostName << " " << serverPid << " " << serverCapacity << endl;
      }
    }
  else
    {
    cerr << "Error: failed to get configuration stream. Aborting." << endl;
    }

  // update host laod to reflect all of its ranks.
  map<string,HostData*>::iterator it=this->ServerHosts.begin();
  map<string,HostData*>::iterator end=this->ServerHosts.end();
  while (it!=end)
    {
    it->second->UpdateLoadWidget();
    ++it;
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

    this->ClientRank->SetLoad(this->ClientMemMonitor->GetVmRSS());
    this->ClientRank->UpdateLoadWidget();

    vtkSMStringVectorProperty *memProp
      =dynamic_cast<vtkSMStringVectorProperty *>(pmProxy->GetProperty("MemoryUseStream"));

    pmProxy->UpdatePropertyInformation(memProp);

    string stream=memProp->GetElement(0);
    istringstream is(stream);

    cerr << stream << endl;

    if (stream.size()>0 && is.good())
      {
      int commSize;
      is >> commSize;

      for (int i=0; i<commSize; ++i)
        {
        unsigned long long memUse;
        is >> memUse;
        this->ServerRanks[i]->SetLoad(memUse);
        this->ServerRanks[i]->UpdateLoadWidget();
        }
      // update host laod to reflect all of its ranks.
      map<string,HostData*>::iterator it=this->ServerHosts.begin();
      map<string,HostData*>::iterator end=this->ServerHosts.end();
      while (it!=end)
        {
        (*it).second->UpdateLoadWidget();
        ++it;
        }
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
      case PROCESS_TYPE_CLIENT:
      case PROCESS_TYPE_SERVER_RANK:
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
          // clientGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT));
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
//       case PROCESS_TYPE_CLIENT:
//       case PROCESS_TYPE_SERVER_RANK:
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