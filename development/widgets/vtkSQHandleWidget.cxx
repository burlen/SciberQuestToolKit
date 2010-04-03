/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQHandleWidget.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"


vtkCxxRevisionMacro(vtkSQHandleWidget, "$Revision: 1.9 $");
vtkStandardNewMacro(vtkSQHandleWidget);

//----------------------------------------------------------------------------------
vtkSQHandleWidget::vtkSQHandleWidget()
{
  // Set the initial state
  this->WidgetState = vtkSQHandleWidget::Start;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkSQHandleWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSQHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkSQHandleWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, vtkSQHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, vtkSQHandleWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, vtkSQHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSQHandleWidget::MoveAction);
  this->EnableAxisConstraint = 1;
  this->AllowHandleResize    = 1;
}

//----------------------------------------------------------------------------------
vtkSQHandleWidget::~vtkSQHandleWidget()
{
}

//----------------------------------------------------------------------
void vtkSQHandleWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkPointHandleRepresentation3D::New();
    }
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::SetCursor(int cState)
{
  if ( this->ManagesCursor )
    {
    switch (cState)
      {
      case vtkHandleRepresentation::Outside:
        this->RequestCursorShape(VTK_CURSOR_DEFAULT);
        break;
      default:
        this->RequestCursorShape(VTK_CURSOR_HAND);
      }
    }
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkSQHandleWidget *self = reinterpret_cast<vtkSQHandleWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->WidgetRep->ComputeInteractionState(X, Y);
  if ( self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  if ( ! self->Parent )
    {
    self->GrabFocus(self->EventCallbackCommand);
    }
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->WidgetState = vtkSQHandleWidget::Active;
  reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkHandleRepresentation::Selecting);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::TranslateAction(vtkAbstractWidget *w)
{
  vtkSQHandleWidget *self = reinterpret_cast<vtkSQHandleWidget*>(w);

  double eventPos[2];
  eventPos[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
  eventPos[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);

  self->WidgetRep->StartWidgetInteraction(eventPos);
  if ( self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  self->WidgetState = vtkSQHandleWidget::Active;
  reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkHandleRepresentation::Translating);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::ScaleAction(vtkAbstractWidget *w)
{
  vtkSQHandleWidget *self = reinterpret_cast<vtkSQHandleWidget*>(w);

  if (self->AllowHandleResize)
    {

    double eventPos[2];
    eventPos[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
    eventPos[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);

    self->WidgetRep->StartWidgetInteraction(eventPos);
    if ( self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside )
      {
      return;
      }

    // We are definitely selected
    self->WidgetState = vtkSQHandleWidget::Active;
    reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->
      SetInteractionState(vtkHandleRepresentation::Scaling);

    self->GenericAction(self);
    }
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::GenericAction(vtkSQHandleWidget *self)
{
  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor.
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Check to see whether motion is constrained
  if ( self->Interactor->GetShiftKey() && self->EnableAxisConstraint )
    {
    reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->ConstrainedOn();
    }
  else
    {
    reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->ConstrainedOff();
    }

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSQHandleWidget *self = reinterpret_cast<vtkSQHandleWidget*>(w);

  if ( self->WidgetState != vtkSQHandleWidget::Active )
    {
    return;
    }

  // Return state to not selected
  self->WidgetState = vtkSQHandleWidget::Start;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // stop adjusting
  if ( ! self->Parent )
    {
    self->ReleaseFocus();
    }
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->WidgetState = vtkSQHandleWidget::Start;
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSQHandleWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSQHandleWidget *self = reinterpret_cast<vtkSQHandleWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if ( self->WidgetState == vtkSQHandleWidget::Start )
    {
    int state = self->WidgetRep->GetInteractionState();
    self->WidgetRep->ComputeInteractionState(X, Y);
    self->SetCursor(self->WidgetRep->GetInteractionState());
    // Must rerender if we change appearance
    if ( reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->GetActiveRepresentation() &&
         state != self->WidgetRep->GetInteractionState() )
      {
      self->Render();
      }
    return;
    }

  // Okay, adjust the representation
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPosition);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------------------
void vtkSQHandleWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Allow Handle Resize: " 
     << (this->AllowHandleResize ? "On\n" : "Off\n");

  os << indent << "Enable Axis Constraint: " 
     << (this->EnableAxisConstraint ? "On\n" : "Off\n");

  os << indent << "WidgetState: " << this->WidgetState << endl;  
}
