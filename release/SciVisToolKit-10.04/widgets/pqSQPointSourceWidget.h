/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqSQPointSourceWidget.h,v $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2. 

   See License_v1.2.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqSQPointSourceWidget_h
#define _pqSQPointSourceWidget_h

#include "pqSMProxy.h"

#include "pqSQHandleWidget.h" 
#include "pqComponentsExport.h"

class pqPropertyManager;

/// Provides a complete Qt UI for working with a vtkPointSource filter
class PQCOMPONENTS_EXPORT pqSQPointSourceWidget : public pqSQHandleWidget
{
  typedef pqSQHandleWidget Superclass;
  
  Q_OBJECT
  
public:
  pqSQPointSourceWidget(vtkSMProxy* pxy, QWidget* p = 0);
  pqSQPointSourceWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p = 0);
  ~pqSQPointSourceWidget();

  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  /// This typically calls PlaceWidget on the underlying 3D Widget 
  /// with reference proxy bounds.
  /// This should be explicitly called after the panel is created
  /// and the widget is initialized i.e. the reference proxy, controlled proxy
  /// and hints have been set.
  virtual void resetBounds(double bounds[6]);
  virtual void resetBounds()
    { this->Superclass::resetBounds(); }

protected:
  /// Subclasses can override this method to map properties to
  /// GUI. Default implementation updates the internal datastructures
  /// so that default implementations can be provided for 
  /// accept/reset.
  virtual void setControlledProperty(const char* function,
    vtkSMProperty * controlled_property);

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
