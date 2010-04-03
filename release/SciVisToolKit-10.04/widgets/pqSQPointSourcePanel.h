/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef pqSQPointSourcePanel_h
#define pqSQPointSourcePanel_h

#include "pqNamedObjectPanel.h"
#include "pqComponentsExport.h"



// Define the following to enable debug io
#define pqSQPointSourcePanelDEBUG

#include "ui_pqSQPointSourcePanelForm.h"
using Ui::pqSQPointSourcePanelForm;

class pqProxy;
class QWidget;
class pqSQHandleWidget;
//class pqSQPointSourcePanelForm;

class pqSQPointSourcePanel : public pqNamedObjectPanel
{
  Q_OBJECT
public:
  pqSQPointSourcePanel(pqProxy* proxy, QWidget* p = NULL);
  virtual ~pqSQPointSourcePanel();

protected slots:
  // Description:
  // This is where we have to communicate our state to the server.
  void accept();

private:

private:
  pqSQHandleWidget *HandleUI;
  pqSQPointSourcePanelForm *Form;
};

#endif
