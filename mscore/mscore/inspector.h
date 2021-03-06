//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#ifndef __INSPECTOR_H__
#define __INSPECTOR_H__

#include "ui_inspector_element.h"
#include "ui_inspector_vbox.h"
#include "ui_inspector_hbox.h"
#include "ui_inspector_articulation.h"
#include "ui_inspector_spacer.h"

class Element;
class Inspector;

//---------------------------------------------------------
//   InspectorElementBase
//---------------------------------------------------------

class InspectorElementBase : public QWidget {
      Q_OBJECT

   protected:
      QVBoxLayout* layout;
      Inspector* inspector;

   public:
      InspectorElementBase(Inspector* i, QWidget* parent = 0) : QWidget(parent), inspector(i) {}
      virtual void setElement(Element*) = 0;
      virtual void apply() {}
      };

//---------------------------------------------------------
//   InspectorElement
//---------------------------------------------------------

class InspectorElement : public InspectorElementBase {
      Q_OBJECT

      Ui::InspectorElement ie;

   public:
      InspectorElement(Inspector*, QWidget* parent = 0);
      virtual void setElement(Element*);
      virtual void apply();
      };

//---------------------------------------------------------
//   InspectorVBox
//---------------------------------------------------------

class InspectorVBox : public InspectorElementBase {
      Q_OBJECT

      Ui::InspectorVBox vb;

   public:
      InspectorVBox(Inspector*, QWidget* parent = 0);
      virtual void setElement(Element*);
      virtual void apply();
      };

//---------------------------------------------------------
//   InspectorHBox
//---------------------------------------------------------

class InspectorHBox : public InspectorElementBase {
      Q_OBJECT

      Ui::InspectorHBox hb;

   public:
      InspectorHBox(Inspector*, QWidget* parent = 0);
      virtual void setElement(Element*);
      virtual void apply();
      };

//---------------------------------------------------------
//   InspectorArticulation
//---------------------------------------------------------

class InspectorArticulation : public InspectorElementBase {
      Q_OBJECT

      Ui::InspectorArticulation ar;

   public:
      InspectorArticulation(Inspector*, QWidget* parent = 0);
      virtual void setElement(Element*);
      virtual void apply();
      };

//---------------------------------------------------------
//   InspectorSpacer
//---------------------------------------------------------

class InspectorSpacer : public InspectorElementBase {
      Q_OBJECT

      Ui::InspectorSpacer sp;

   public:
      InspectorSpacer(Inspector*, QWidget* parent = 0);
      virtual void setElement(Element*);
      virtual void apply();
      };

//---------------------------------------------------------
//   Inspector
//---------------------------------------------------------

class Inspector : public QDockWidget {
      Q_OBJECT

      QVBoxLayout* layout;
      InspectorElementBase* ie;
      QPushButton* apply;
      Element* _element;

   private slots:
      void applyClicked();

   signals:
      void inspectorVisible(bool);

   public slots:
      void enableApply(bool val = true) { apply->setEnabled(val); }
      void reset();

   public:
      Inspector(QWidget* parent = 0);
      void setElement(Element*);
      Element* element() const { return _element; }
      };

#endif

