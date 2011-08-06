//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "inspector.h"
#include "musescore.h"
#include "libmscore/element.h"
#include "libmscore/score.h"
#include "libmscore/box.h"
#include "libmscore/undo.h"

//---------------------------------------------------------
//   showInspector
//---------------------------------------------------------

void MuseScore::showInspector(bool visible)
      {
      QAction* a = getAction("inspector");
      if (visible) {
            if (!inspector) {
                  inspector = new Inspector();
                  QAction* a = getAction("toggle-palette");
                  connect(inspector, SIGNAL(inspectorVisible(bool)), a, SLOT(setChecked(bool)));
                  addDockWidget(Qt::RightDockWidgetArea, inspector);
                  }
            }
      if (inspector)
            inspector->setVisible(visible);
      a->setChecked(visible);
      }

//---------------------------------------------------------
//   Inspector
//---------------------------------------------------------

Inspector::Inspector(QWidget* parent)
   : QDockWidget(tr("Inspector"), parent)
      {
      setObjectName("inspector");
      setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
      QWidget* mainWidget = new QWidget;
      mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); //??
      setWidget(mainWidget);
      layout = new QVBoxLayout;
      mainWidget->setLayout(layout);
      ie        = 0;
      _element  = 0;
      QHBoxLayout* hbox = new QHBoxLayout;
      apply = new QPushButton;
      apply->setText(tr("Apply"));
      apply->setEnabled(false);
      hbox->addWidget(apply);
      layout->addStretch(10);
      layout->addLayout(hbox);
      connect(apply, SIGNAL(clicked()), SLOT(applyClicked()));
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Inspector::reset()
      {
      if (ie)
            ie->setElement(_element);
      apply->setEnabled(false);
      }

//---------------------------------------------------------
//   applyClicked
//---------------------------------------------------------

void Inspector::applyClicked()
      {
      if (ie)
            ie->apply();
      apply->setEnabled(false);
      }

//---------------------------------------------------------
//   setElement
//---------------------------------------------------------

void Inspector::setElement(Element* e)
      {
      if (ie)
            delete ie;
      ie = 0;
      _element = e;
      if (_element == 0)
            return;
      switch(_element->type()) {
            case VBOX: ie = new InspectorVBox(this, this); break;
            default:   ie = new InspectorElement(this, this); break;
            }
      layout->insertWidget(0, ie);
      ie->setElement(_element);
      }

//---------------------------------------------------------
//   InspectorElement
//---------------------------------------------------------

InspectorElement::InspectorElement(Inspector* i, QWidget* parent)
   : InspectorElementBase(i, parent)
      {
      QWidget* w = new QWidget;
      layout = new QVBoxLayout;
      setLayout(layout);

      ie.setupUi(w);
      layout->addWidget(w);
      connect(ie.x, SIGNAL(valueChanged(double)), inspector, SLOT(enableApply()));
      connect(ie.y, SIGNAL(valueChanged(double)), inspector, SLOT(enableApply()));
      }

//---------------------------------------------------------
//   setElement
//---------------------------------------------------------

void InspectorElement::setElement(Element* e)
      {
      qreal _spatium = e->score()->spatium();
      ie.elementName->setText(e->name());
      ie.x->setValue(e->userOff().x() / _spatium);
      ie.y->setValue(e->userOff().y() / _spatium);
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void InspectorElement::apply()
      {
      Element* e      = inspector->element();
      Score* score    = e->score();
      qreal _spatium  = score->spatium();
      QPointF o(ie.x->value() * _spatium, ie.y->value() * _spatium);
      if (o != e->userOff()) {
            score->startCmd();
            score->undo()->push(new ChangeUserOffset(e, o));
            score->setLayoutAll(true);
            score->endCmd();
            mscore->endCmd();
            }
      }

//---------------------------------------------------------
//   InspectorVBox
//---------------------------------------------------------

InspectorVBox::InspectorVBox(Inspector* i, QWidget* parent)
   : InspectorElementBase(i, parent)
      {
      QWidget* w = new QWidget;
      layout = new QVBoxLayout;
      setLayout(layout);

      vb.setupUi(w);
      layout->addWidget(w);
      connect(vb.topGap,    SIGNAL(valueChanged(double)), inspector, SLOT(enableApply()));
      connect(vb.bottomGap, SIGNAL(valueChanged(double)), inspector, SLOT(enableApply()));
      connect(vb.height,    SIGNAL(valueChanged(double)), inspector, SLOT(enableApply()));
      }

//---------------------------------------------------------
//   setElement
//---------------------------------------------------------

void InspectorVBox::setElement(Element* e)
      {
      VBox* box = static_cast<VBox*>(e);
      qreal _spatium = e->score()->spatium();
      vb.elementName->setText(e->name());
      vb.topGap->blockSignals(true);
      vb.bottomGap->blockSignals(true);
      vb.height->blockSignals(true);
      vb.topGap->setValue(box->topGap() / _spatium);
      vb.bottomGap->setValue(box->bottomGap() / _spatium);
      vb.height->setValue(box->height() / _spatium);
      vb.topGap->blockSignals(false);
      vb.bottomGap->blockSignals(false);
      vb.height->blockSignals(false);
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void InspectorVBox::apply()
      {
      VBox* box       = static_cast<VBox*>(inspector->element());
      Score* score    = box->score();
      qreal _spatium  = score->spatium();
      qreal topGap    = vb.topGap->value() * _spatium;
      qreal bottomGap = vb.bottomGap->value() * _spatium;
      qreal height    = vb.height->value() * _spatium;
      if (topGap != box->topGap() || bottomGap != box->bottomGap()
         || height != box->height()) {
            score->startCmd();
            score->undo()->push(new ChangeBoxProperties(box,
               box->leftMargin(), box->topMargin(), box->rightMargin(), box->bottomMargin(),
               height, box->width(),
               topGap, bottomGap
               ));
            score->setLayoutAll(true);
            score->endCmd();
            mscore->endCmd();
            }
      }

