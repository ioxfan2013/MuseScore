//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2009 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "textline.h"
#include "style.h"
#include "system.h"
#include "measure.h"
#include "xml.h"
#include "utils.h"
#include "layout.h"
#include "score.h"
#include "preferences.h"
#include "sym.h"

//---------------------------------------------------------
//   TextLineSegment
//---------------------------------------------------------

TextLineSegment::TextLineSegment(Score* s)
   : LineSegment(s)
      {
      _text = 0;
      }

TextLineSegment::TextLineSegment(const TextLineSegment& seg)
   : LineSegment(seg)
      {
      _text = 0;
      }

void TextLineSegment::setSelected(bool f)
      {
      Element::setSelected(f);
      if (_text && textLine()->hasBeginText())
            _text->setSelected(f);
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void TextLineSegment::add(Element*)
      {
#if 0  // TODO
      if (e->type() != TEXT) {
            printf("TextLineSegment: add illegal element\n");
            return;
            }
      _text = (TextC*)e;
      _text->setParent(this);
      TextLine* tl = (TextLine*)line();

      TextBase* tb = 0;
      if (_text->otb()) {
            tb = _text->otb();
            _text->setOtb(0);
            }
      else {
            tb = new TextBase(*tl->textBase());
            }
      tl->setTextBase(tb);
      _text->baseChanged();
#endif
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void TextLineSegment::remove(Element*)
      {
#if 0 // TODO
      if (e != _text) {
            printf("TextLineSegment: cannot remove %s %p %p\n", e->name(), e, _text);
            return;
            }
      _text->setOtb(_text->textBase());
      _text = 0;
#endif
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TextLineSegment::draw(QPainter& p) const
      {
      TextLine* tl = textLine();
      qreal textlineLineWidth    = tl->lineWidth().point();
      qreal textlineTextDistance = _spatium * .5;

      QPointF pp2(pos2());

      qreal l = 0.0;
      if (_segmentType == SEGMENT_SINGLE || _segmentType == SEGMENT_BEGIN) {
            if (_text && tl->hasBeginText()) {
                  QRectF bb(_text->bbox());
                  l = _text->pos().x() + bb.width() + textlineTextDistance;
                  p.save();
                  p.translate(_text->pos());
                  p.setPen(QPen(_text->curColor()));
                  _text->draw(p);
                  p.restore();
                  }
            else if (tl->beginSymbol() != -1) {
                  int sym = tl->beginSymbol();
                  const QRectF& bb = symbols[sym].bbox();
                  qreal h = bb.height() * .5;
                  QPointF o = tl->beginSymbolOffset() * _spatium;
                  symbols[sym].draw(p, o.x(), h + o.y());
                  l = bb.width() + textlineTextDistance;
                  }
            }
      if (_segmentType == SEGMENT_SINGLE || _segmentType == SEGMENT_END) {
            if (tl->endSymbol() != -1) {
                  int sym = tl->endSymbol();
                  const QRectF& bb = symbols[sym].bbox();
                  qreal h = bb.height() * .5;
                  QPointF o = tl->endSymbolOffset() * _spatium;
                  pp2.setX(pp2.x() - bb.width() + textlineTextDistance);
                  symbols[sym].draw(p, pp2.x() + textlineTextDistance + o.x(), h + o.y());
                  }
            }

      QPointF pp1(l, 0.0);

      QPen pen(p.pen());
      pen.setWidthF(textlineLineWidth);
      pen.setStyle(tl->lineStyle());

      if (selected() && !(score() && score()->printing()))
            pen.setColor(preferences.selectColor[0]);
      else
            pen.setColor(tl->lineColor());

      p.setPen(pen);
      p.drawLine(QLineF(pp1, pp2));

      if (tl->endHook()) {
            double hh = textLine()->endHookHeight().point();
            if (_segmentType == SEGMENT_SINGLE || _segmentType == SEGMENT_END) {
                  p.drawLine(QLineF(pp2, QPointF(pp2.x(), pp2.y() + hh)));
                  }
            }
      if (tl->beginHook()) {
            double hh = textLine()->beginHookHeight().point();
            if (_segmentType == SEGMENT_SINGLE || _segmentType == SEGMENT_BEGIN) {
                  p.drawLine(QLineF(pp1, QPointF(pp1.x(), pp1.y() + hh)));
                  }
            }
      }

//---------------------------------------------------------
//   bbox
//---------------------------------------------------------

QRectF TextLineSegment::bbox() const
      {
      QPointF pp1;
      QPointF pp2(pos2());

      if (!textLine()->hasBeginText() && pp2.y() != 0)
            return QRectF(pp1, pp2).normalized();
      qreal h1;
      int sym = textLine()->beginSymbol();
      if (_text)
            h1 = _text->height() * .5;
      else if (sym != -1)
            h1 = symbols[sym].height() * .5;
      if (textLine()->endHook()) {
            double hh = textLine()->endHookHeight().point();
            if (hh > h1)
                  h1 = hh;
            }
      if (textLine()->beginHook()) {
            double hh = textLine()->beginHookHeight().point();
            if (hh > h1)
                  h1 = hh;
            }
      return QRectF(.0, -h1, pp2.x(), h1 * 2);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void TextLineSegment::layout(ScoreLayout* l)
      {
      TextLine* tl = (TextLine*)line();
      _userOff2.setY(0);
      switch (_segmentType) {
            case SEGMENT_SINGLE:
            case SEGMENT_BEGIN:
                  if (tl->hasBeginText()) {
                        if (_text == 0) {
                              _text = new TextC(tl->beginTextBasePtr(), score());
                              _text->setSubtype(TEXT_TEXTLINE);
                              _text->setMovable(false);
                              _text->setParent(this);
                              }
                        _text->layout(l);
                        }
                  else if (_text) {
                        delete _text;
                        _text = 0;
                        }
                  break;
            case SEGMENT_MIDDLE:
            case SEGMENT_END:
                  if (tl->hasContinueText()) {
                        if (_text == 0) {
                              _text = new TextC(tl->continueTextBasePtr(), score());
                              _text->setSubtype(TEXT_TEXTLINE);
                              _text->setMovable(false);
                              _text->setParent(this);
                              }
                        _text->layout(l);
                        }
                  else if (_text) {
                        delete _text;
                        _text = 0;
                        }
                  break;
            }
      }

//---------------------------------------------------------
//   TextLine
//---------------------------------------------------------

TextLine::TextLine(Score* s)
   : SLine(s)
      {
      _beginText       = new TextBase;
      _continueText    = new TextBase;
      _hasBeginText    = false;
      _hasContinueText = false;

      _beginHookHeight = Spatium(1.5);
      _endHookHeight   = Spatium(1.5);
      _beginHook       = false;
      _endHook         = false;

      _lineWidth  = Spatium(0.15);
      _lineStyle  = Qt::SolidLine;
      _lineColor  = Qt::black;
      _mxmlOff2   = 0;

      _beginSymbol    = -1;
      _continueSymbol = -1;
      _endSymbol      = -1;

      _beginText->setDefaultFont(score()->textStyle(TEXT_STYLE_TEXTLINE)->font());
      _continueText->setDefaultFont(score()->textStyle(TEXT_STYLE_TEXTLINE)->font());

      setLen(_spatium * 7);   // for use in palettes
      }

TextLine::TextLine(const TextLine& e)
   : SLine(e)
      {
      _lineWidth            = e._lineWidth;
      _lineColor            = e._lineColor;
      _lineStyle            = e._lineStyle;
      _beginHook            = e._beginHook;
      _endHook              = e._endHook;
      _beginHookHeight      = e._beginHookHeight;
      _endHookHeight        = e._endHookHeight;
      _hasBeginText         = e._hasBeginText;
      _hasContinueText      = e._hasContinueText;
      _beginSymbol          = e._beginSymbol;
      _continueSymbol       = e._continueSymbol;
      _endSymbol            = e._endSymbol;
      _beginSymbolOffset    = e._beginSymbolOffset;
      _continueSymbolOffset = e._continueSymbolOffset;
      _endSymbolOffset      = e._endSymbolOffset;

      _mxmlOff2   = e._mxmlOff2;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void TextLine::write(Xml& xml) const
      {
      xml.stag(name());
      if (_beginHook)
            xml.tag("beginHookHeight", _beginHookHeight.val());
      if (_endHook)
            xml.tag("endHookHeight", _endHookHeight.val());

      xml.tag("lineWidth", _lineWidth.val());
      xml.tag("lineStyle", _lineStyle);
      xml.tag("lineColor", _lineColor);

      SLine::writeProperties(xml);
      if (_hasBeginText) {
            xml.stag("beginText");
            _beginText->writeProperties(xml);
            xml.etag();
            }
      if (_hasContinueText) {
            xml.stag("continueText");
            _continueText->writeProperties(xml);
            xml.etag();
            }
      if (_beginSymbol != -1) {
            xml.tag("beginSymbol", _beginSymbol);   // symbols[_symbol].name();
            xml.tag("beginSymbolOffset", _beginSymbolOffset);
            }
      if (_continueSymbol != -1) {
            xml.tag("continueSymbol", _continueSymbol);
            xml.tag("continueSymbolOffset", _continueSymbolOffset);
            }
      if (_endSymbol != -1) {
            xml.tag("endSymbol", _endSymbol);
            xml.tag("endSymbolOffset", _endSymbolOffset);
            }
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void TextLine::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            const QString& text = e.text();
            if (tag == "beginHookHeight") {
                  _beginHookHeight = Spatium(text.toDouble());
                  _beginHook = true;
                  }
            else if (tag == "endHookHeight" || tag == "hookHeight") { // hookHeight is obsolete
                  _endHookHeight = Spatium(text.toDouble());
                  _endHook = true;
                  }
            else if (tag == "hookUp")           // obsolete
                  _endHookHeight *= -1.0;
            else if (tag == "beginSymbol" || tag == "symbol")     // "symbol" is obsolete
                  _beginSymbol = text.toInt();
            else if (tag == "continueSymbol")
                  _continueSymbol = text.toInt();
            else if (tag == "endSymbol")
                  _endSymbol = text.toInt();
            else if (tag == "beginSymbolOffset")
                  _beginSymbolOffset = readPoint(e);
            else if (tag == "continueSymbolOffset")
                  _continueSymbolOffset = readPoint(e);
            else if (tag == "endSymbolOffset")
                  _endSymbolOffset = readPoint(e);
            else if (tag == "lineWidth")
                  _lineWidth = Spatium(text.toDouble());
            else if (tag == "lineStyle")
                  _lineStyle = Qt::PenStyle(text.toInt());
            else if (tag == "lineColor")
                  _lineColor = readColor(e);
            else if (tag == "beginText") {
                  // else if (_text->readProperties(e))
                  _hasBeginText = true;
                  }
            else if (tag == "continueText") {
                  _hasContinueText = true;
                  }
            else if (!SLine::readProperties(e))
                  domError(e);
            }
      }

//---------------------------------------------------------
//   createLineSegment
//---------------------------------------------------------

LineSegment* TextLine::createLineSegment()
      {
      LineSegment* seg = new TextLineSegment(score());
      return seg;
      }

//---------------------------------------------------------
//   genPropertyMenu
//---------------------------------------------------------

bool TextLineSegment::genPropertyMenu(QMenu* popup) const
      {
      QAction* a = popup->addAction(tr("Line Properties..."));
      a->setData("props");
      return true;
      }

//---------------------------------------------------------
//   propertyAction
//---------------------------------------------------------

void TextLineSegment::propertyAction(const QString& s)
      {
      if (s == "props") {
            LineProperties lp(textLine(), 0);
            lp.exec();
            }
      else
            Element::propertyAction(s);
      }

//---------------------------------------------------------
//   populateLineSymbolComboBox
//---------------------------------------------------------

static void populateLineSymbolComboBox(QComboBox* cb)
      {
      cb->clear();
      cb->addItem(cb->tr("Ped (Pedal)"), pedalPedSym);
      cb->addItem(cb->tr("* (Pedal)"), pedalasteriskSym);
      cb->addItem(cb->tr(". (Pedal)"), pedaldotSym);
      cb->addItem(cb->tr("dash (Pedal)"), pedaldashSym);
      cb->addItem(cb->tr("tr (Trill)"), trillSym);
      }

static void setLineSymbolComboBox(QComboBox* cb, int sym)
      {
      if (sym == -1)
            return;
      for (int i = 0; i < cb->count(); ++i) {
            if (cb->itemData(i).toInt() == sym) {
                  cb->setCurrentIndex(i);
                  return;
                  }
            }
      printf("setLineSymbol: not found %d\n", sym);
      }

//---------------------------------------------------------
//   LineProperties
//---------------------------------------------------------

LineProperties::LineProperties(TextLine* l, QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      tl = l;

      populateLineSymbolComboBox(beginSymbol);
      populateLineSymbolComboBox(continueSymbol);
      populateLineSymbolComboBox(endSymbol);

      lineWidth->setValue(tl->lineWidth().val());
      lineStyle->setCurrentIndex(int(tl->lineStyle() - 1));
      linecolor->setColor(tl->lineColor());

      beginTextRb->setChecked(tl->hasBeginText());
      continueTextRb->setChecked(tl->hasContinueText());
      beginSymbolRb->setChecked(tl->beginSymbol() != -1);
      continueSymbolRb->setChecked(tl->continueSymbol() != -1);
      endSymbolRb->setChecked(tl->endSymbol() != -1);

      beginText->setText(tl->beginTextBase()->getText());
      continueText->setText(tl->continueTextBase()->getText());

      setLineSymbolComboBox(beginSymbol, tl->beginSymbol());
      setLineSymbolComboBox(continueSymbol, tl->continueSymbol());
      setLineSymbolComboBox(endSymbol, tl->endSymbol());

      beginSymbolX->setValue(tl->beginSymbolOffset().x());
      beginSymbolY->setValue(tl->beginSymbolOffset().y());
      continueSymbolX->setValue(tl->continueSymbolOffset().x());
      continueSymbolY->setValue(tl->continueSymbolOffset().y());
      endSymbolX->setValue(tl->endSymbolOffset().x());
      endSymbolY->setValue(tl->endSymbolOffset().y());

      beginHook->setChecked(tl->beginHook());
      endHook->setChecked(tl->endHook());
      beginHookHeight->setValue(tl->beginHookHeight().val());
      endHookHeight->setValue(tl->endHookHeight().val());
#if 0
      TextBase* tb = tl->textBase();
      if (tb->frameWidth()) {
            frame->setChecked(true);
            frameWidth->setValue(tb->frameWidth());
            frameMargin->setValue(tb->paddingWidth());
            frameColor->setColor(tb->frameColor());
            frameCircled->setChecked(tb->circle());
            }
      else
            frame->setChecked(false);
#endif
      connect(beginTextTb, SIGNAL(clicked()), SLOT(beginTextClicked()));
      connect(continueTextTb, SIGNAL(clicked()), SLOT(continueTextClicked()));
      connect(beginTextRb, SIGNAL(toggled(bool)), SLOT(beginTextToggled(bool)));
      connect(beginSymbolRb, SIGNAL(toggled(bool)), SLOT(beginSymbolToggled(bool)));
      connect(continueTextRb, SIGNAL(toggled(bool)), SLOT(continueTextToggled(bool)));
      connect(continueSymbolRb, SIGNAL(toggled(bool)), SLOT(continueSymbolToggled(bool)));
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void LineProperties::accept()
      {
      tl->setLineWidth(Spatium(lineWidth->value()));
      tl->setLineStyle(Qt::PenStyle(lineStyle->currentIndex() + 1));
      tl->setLineColor(linecolor->color());

      tl->setBeginHookHeight(Spatium(beginHookHeight->value()));
      tl->setBeginHook(beginHook->isChecked());
      tl->setEndHookHeight(Spatium(endHookHeight->value()));
      tl->setEndHook(endHook->isChecked());
      tl->setHasBeginText(beginTextRb->isChecked());
      tl->setHasContinueText(continueTextRb->isChecked());

      int sym = beginSymbol->itemData(beginSymbol->currentIndex()).toInt();
      tl->setBeginSymbol(beginSymbolRb->isChecked() ? sym : -1);

      sym = continueSymbol->itemData(continueSymbol->currentIndex()).toInt();
      tl->setContinueSymbol(continueSymbolRb->isChecked() ? sym : -1);

      sym = endSymbol->itemData(endSymbol->currentIndex()).toInt();
      tl->setEndSymbol(endSymbolRb->isChecked() ? sym : -1);

      tl->beginTextBase()->setText(beginText->text(), 0);
      tl->continueTextBase()->setText(continueText->text(), 0);

      tl->setBeginSymbolOffset(QPointF(beginSymbolX->value(), beginSymbolY->value()));
      tl->setContinueSymbolOffset(QPointF(continueSymbolX->value(), continueSymbolY->value()));
      tl->setEndSymbolOffset(QPointF(endSymbolX->value(), endSymbolY->value()));

#if 0
      TextBase* tb = tl->textBase();
      if (frame->isChecked()) {
            tb->setFrameWidth(frameWidth->value());
            tb->setPaddingWidth(frameMargin->value());
            tb->setFrameColor(frameColor->color());
            tb->setCircle(frameCircled->isChecked());
            }
      else
            tb->setFrameWidth(0.0);
#endif

      QDialog::accept();
      }

//---------------------------------------------------------
//   beginTextClicked
//---------------------------------------------------------

void LineProperties::beginTextClicked()
      {
      printf("beginText...\n");
      }

//---------------------------------------------------------
//   continueTextClicked
//---------------------------------------------------------

void LineProperties::continueTextClicked()
      {
      printf("continueText...\n");
      }

void LineProperties::beginTextToggled(bool val)
      {
      if (val)
            beginSymbolRb->setChecked(false);
      }

void LineProperties::beginSymbolToggled(bool val)
      {
      if (val)
            beginTextRb->setChecked(false);
      }

void LineProperties::continueTextToggled(bool val)
      {
      if (val)
            continueSymbolRb->setChecked(false);
      }

void LineProperties::continueSymbolToggled(bool val)
      {
      if (val)
            continueTextRb->setChecked(false);
      }

