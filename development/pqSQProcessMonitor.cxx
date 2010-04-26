#include "pqSQProcessMonitor.h"


#include "pqProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"


#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"

#include <QTreeWidgetItem>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QMessageBox>

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
  PROCESS_TYPE_REMOTE
};

//-----------------------------------------------------------------------------
pqSQProcessMonitor::pqSQProcessMonitor(
        pqProxy* proxy,
        QWidget* widget)
  : pqNamedObjectPanel(proxy, widget)
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::pqSQProcessMonitor" << endl;
  #endif

  // Construct Qt form.
  this->Form=new pqSQProcessMonitorForm;
  this->Form->setupUi(this);
  // this->Form->addCommand->setIcon(QPixmap(":/pqWidgets/Icons/pqNewItem16.png"));
  // this->Form->execCommand->setIcon(QPixmap(":/pqWidgets/Icons/pqVcrPlay16.png"));
  this->Restore();


  vtkSMProxy* dpProxy=this->referenceProxy()->getProxy();

  // Connect to server side pipeline's UpdateInformation events.
  this->VTKConnect=vtkEventQtSlotConnect::New();
  this->VTKConnect->Connect(
      dpProxy,
      vtkCommand::UpdateInformationEvent,
      this, SLOT(UpdateInformationEvent()));

  // Set up configuration viewer
  this->PullServerConfig();

  // set up buttons
  QObject::connect(this->Form->execCommand,SIGNAL(clicked()),this,SLOT(ExecCommand()));
  // QObject::connect(this->Form->addCommand,SIGNAL(clicked()),this,SLOT(AddCommand()));
  QObject::connect(this->Form->delCommand,SIGNAL(clicked()),this,SLOT(DelCommand()));
  QObject::connect(this->Form->editCommand,SIGNAL(toggled(bool)),this,SLOT(EditCommand(bool)));

  // Let the super class do the undocumented stuff that needs to hapen.
  pqNamedObjectPanel::linkServerManagerProperties();
}

//-----------------------------------------------------------------------------
pqSQProcessMonitor::~pqSQProcessMonitor()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::~pqSQProcessMonitor" << endl;
  #endif

  this->Save();
  delete this->Form;

  this->VTKConnect->Delete();
  this->VTKConnect=0;
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::Restore()
{
  QStringList defaults;
  defaults
       << ""
       << "-geometry 200x40 -fg white -bg black"
       << "xterm @XTOPTS@ -e ssh -t @HOST@ gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @FEURL@ ssh -t @HOST@ gdb --pid=@PID@"
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

  QSettings settings("SciberQuest", "SciVisToolKit");

  settings.setValue("ProcessMonitor/defaults",defs);
}


//-----------------------------------------------------------------------------
void pqSQProcessMonitor::UpdateInformationEvent()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::UpdateInformationEvent" << endl;
  #endif
  // vtkSMProxy* dpProxy=this->referenceProxy()->getProxy();
}


//-----------------------------------------------------------------------------
void pqSQProcessMonitor::PullServerConfig()
{
  vtkSMProxy* dpProxy=this->referenceProxy()->getProxy();

  // client
  QTreeWidgetItem *clientGroup=new QTreeWidgetItem(this->Form->configView,QStringList("paraview"));
  clientGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroup->setExpanded(false);
  clientGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_LOCAL));
  // hostname
  char clientHostName[1024];
  gethostname(clientHostName,1024);
  clientGroup->setText(1,clientHostName);
  // pid
  pid_t clientPid=getpid();
  clientGroup->setText(2,QString("%1").arg(clientPid));

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

    for (int i=0; i<commSize; ++i)
      {
      QTreeWidgetItem *serverConfig=new QTreeWidgetItem(serverGroup);
      serverConfig->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverConfig->setExpanded(false);
      serverConfig->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_REMOTE));

      serverConfig->setText(0,QString("%1").arg(i));

      string serverHostName;
      is >> serverHostName;
      serverConfig->setText(1,serverHostName.c_str());

      pid_t serverPid;
      is >> serverPid;
      serverConfig->setText(2,QString("%1").arg(serverPid));

      cerr << serverHostName << " " << serverPid << endl;
      }
    }
  else
    {
    cerr << "Error: failed to get configuration stream. Aborting." << endl;
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
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::ForkExec" << endl;
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

        string hostNameStr((const char *)item->text(1).toAscii());
        SearchAndReplace(string("@HOST@"),hostNameStr,cmd);

        string pidStr((const char *)item->text(2).toAscii());
        SearchAndReplace(string("@PID@"),pidStr,cmd);

        string feURLStr(this->Form->feURL->text().toAscii());
        SearchAndReplace(string("@FEURL@"),feURLStr,cmd);

        string xtOptsStr(this->Form->xtOpts->text().toAscii());
        SearchAndReplace(string("@XTOPTS@"),xtOptsStr,cmd);

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
//   cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::Signal" << endl;
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
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::accept" << endl;
  #endif

  // Let our superclass do the undocumented stuff that needs to be done.
  pqNamedObjectPanel::accept();
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




  /// NOTE how to get something from the server.
  /// dbbProxy->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));
  /// int stamp = vtkSMPropertyHelper(dbbProxy, "SILUpdateStamp").GetAsInt();