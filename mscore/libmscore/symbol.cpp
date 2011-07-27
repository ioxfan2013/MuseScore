//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id$
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "symbol.h"
#include "sym.h"
#include "xml.h"
#include "system.h"
#include "staff.h"
#include "measure.h"
#include "page.h"
#include "score.h"
#include "image.h"
#include "segment.h"
#include "painter.h"
#include "mscore.h"

//---------------------------------------------------------
//   BSymbol
//---------------------------------------------------------

BSymbol::BSymbol(const BSymbol& s)
   : Element(s), ElementLayout(s)
      {
      _z = s._z;
      foreach(Element* e, s._leafs) {
            Element* ee = e->clone();
            ee->setParent(this);
            _leafs.append(ee);
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void BSymbol::add(Element* e)
      {
      if (e->type() == SYMBOL || e->type() == IMAGE) {
            e->setParent(this);
            _leafs.append(e);
            static_cast<BSymbol*>(e)->setZ(z() - 1);    // draw on top of parent
            }
      else
            printf("BSymbol::add: unsupported type %s\n", e->name());
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void BSymbol::remove(Element* e)
      {
      if (e->type() == SYMBOL || e->type() == IMAGE) {
            if (!_leafs.removeOne(e))
                  printf("BSymbol::remove: element <%s> not found\n", e->name());
            }
      else
            printf("BSymbol::remove: unsupported type %s\n", e->name());
      }

//---------------------------------------------------------
//   scanElements
//---------------------------------------------------------

void BSymbol::scanElements(void* data, void (*func)(void*, Element*), bool all)
      {
      func(data, this);
      foreach (Element* e, _leafs)
            e->scanElements(data, func, all);
      }

//---------------------------------------------------------
//   acceptDrop
//---------------------------------------------------------

bool BSymbol::acceptDrop(MuseScoreView*, const QPointF&, int type, int) const
      {
      return type == SYMBOL || type == IMAGE;
      }

//---------------------------------------------------------
//   drop
//---------------------------------------------------------

Element* BSymbol::drop(const DropData& data)
      {
      Element* el = data.element;
      if (el->type() == SYMBOL || el->type() == IMAGE) {
            el->setParent(this);
            QPointF p = data.pos - pagePos() - data.dragOffset;
            el->setUserOff(p);
            score()->undoAddElement(el);
            return el;
            }
      else
            delete el;
      return 0;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void BSymbol::layout()
      {
      foreach(Element* e, _leafs)
            e->layout();
      adjustReadPos();
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

QRectF BSymbol::drag(const EditData& data)
      {
      QRectF r(abbox());
      foreach(const Element* e, _leafs)
            r |= e->abbox();

      qreal x = data.pos.x();
      qreal y = data.pos.y();

      qreal _spatium = spatium();
      if (data.hRaster) {
            qreal hRaster = _spatium / MScore::hRaster();
            int n = lrint(x / hRaster);
            x = hRaster * n;
            }
      if (data.vRaster) {
            qreal vRaster = _spatium / MScore::vRaster();
            int n = lrint(y / vRaster);
            y = vRaster * n;
            }

      setUserOff(QPointF(x, y));

      r |= abbox();
      foreach(const Element* e, _leafs)
            r |= e->abbox();
      return r;
      }

//---------------------------------------------------------
//   Symbol
//---------------------------------------------------------

Symbol::Symbol(Score* s)
   : BSymbol(s)
      {
      _sym = 0;
      setZ(SYMBOL * 100);
      }

Symbol::Symbol(Score* s, int sy)
   : BSymbol(s)
      {
      _sym = sy;
      setZ(SYMBOL * 100);
      }

Symbol::Symbol(const Symbol& s)
   : BSymbol(s)
      {
      _sym   = s._sym;
      setZ(SYMBOL * 100);
      }

//---------------------------------------------------------
//   setAbove
//---------------------------------------------------------

void Symbol::setAbove(bool val)
      {
      setYoff(val ? -2.0 : 7.0);
      }

//---------------------------------------------------------
//   layout
//    height() and width() should return sensible
//    values when calling this method
//---------------------------------------------------------

void Symbol::layout()
      {
//      qreal m = parent() ? parent()->mag() : 1.0;
//      if (_small)
//            m *= score()->styleD(ST_smallNoteMag);
//      setMag(m);
      foreach(Element* e, leafs())
            e->layout();
      ElementLayout::layout(this);
      BSymbol::layout();
      setbbox(symbols[score()->symIdx()][_sym].bbox(magS()));
      }

//---------------------------------------------------------
//   Symbol::draw
//---------------------------------------------------------

void Symbol::draw(Painter* p) const
      {
      if (type() != NOTEDOT || !staff()->useTablature())
            symbols[score()->symIdx()][_sym].draw(p, magS());
      }

//---------------------------------------------------------
//   Symbol::write
//---------------------------------------------------------

void Symbol::write(Xml& xml) const
      {
      xml.stag(name());
      xml.tag("name", symbols[score()->symIdx()][_sym].name());
      Element::writeProperties(xml);
//      xml.tag("small", _small);
      foreach(const Element* e, leafs())
            e->write(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   Symbol::read
//---------------------------------------------------------

void Symbol::read(QDomElement e)
      {
      QPointF pos;
      int s = -1;

      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            QString val(e.text());
            if (tag == "name") {
                  for (int i = 0; i < symbols[0].size(); ++i) {
                        if (val == symbols[0][i].name()) {
                              s = i;
                              break;
                              }
                        }
                  if (s == -1) {
                        printf("unknown symbol <%s>, symbols %d\n",
                           qPrintable(val), symbols[0].size());
                        s = 0;
                        }
                  }
            else if (tag == "Symbol") {
                  Symbol* s = new Symbol(score());
                  s->read(e);
                  s->adjustReadPos();
                  add(s);
                  }
            else if (tag == "Image") {
                  // look ahead for image type
                  QString path;
                  QDomElement ee = e.firstChildElement("path");
                  if (!ee.isNull())
                        path = ee.text();
                  Image* image = 0;
                  QString s(path.toLower());
#ifdef SVG_IMAGES
                  if (s.endsWith(".svg"))
                        image = new SvgImage(score());
                  else
#endif
                        if (s.endsWith(".jpg")
                     || s.endsWith(".png")
                     || s.endsWith(".xpm")
                        ) {
                        image = new RasterImage(score());
                        }
                  else {
                        printf("unknown image format <%s>\n", path.toLatin1().data());
                        }
                  if (image) {
                        image->read(e);
                        add(image);
                        }
                  }
            else if (tag == "small") {
                  ; // _small = val.toInt();
                  }
            else if (!Element::readProperties(e))
                  domError(e);
            }
      if (s == -1) {
            printf("unknown symbol\n");
            s = 0;
            }
      setPos(pos);
      setSym(s);
      }

//---------------------------------------------------------
//   dragAnchor
//---------------------------------------------------------

QLineF Symbol::dragAnchor() const
      {
      if (parent()->type() == MEASURE) {
            Segment* seg     = segment();
            Measure* measure = seg->measure();
            System* s        = measure->system();
            qreal y         = measure->pagePos().y() + s->staff(staffIdx())->y();
            QPointF anchor(seg->abbox().x(), y);
            return QLineF(pagePos(), anchor);
            }
      else {
            return QLineF(pagePos(), parent()->pagePos());
            }
      }

//---------------------------------------------------------
//   pagePos
//---------------------------------------------------------

QPointF BSymbol::pagePos() const
      {
      if (parent() && (parent()->type() == SEGMENT)) {
            qreal yp = y();
            Segment* s = static_cast<Segment*>(parent());
            yp += s->measure()->system()->staffY(staffIdx());
            return QPointF(pageX(), yp);
            }
      else
            return Element::pagePos();
      }

//---------------------------------------------------------
//   FSymbol
//---------------------------------------------------------

FSymbol::FSymbol(Score* s)
  : Element(s)
      {
      _font.setStyleStrategy(QFont::NoFontMerging);
      }

FSymbol::FSymbol(const FSymbol& s)
  : Element(s)
      {
      _font = s._font;
      _code = s._code;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void FSymbol::draw(Painter* painter) const
      {
      QString s;
      painter->setFont(_font);
      if (_code & 0xffff0000) {
            s = QChar(QChar::highSurrogate(_code));
            s += QChar(QChar::lowSurrogate(_code));
            }
      else
            s = QChar(_code);
      painter->drawText(0, 0, s);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void FSymbol::write(Xml& xml) const
      {
      xml.stag(name());
      xml.tag("font", _font.family());
      xml.tag("fontsize", _font.pixelSize());
      xml.tag("code", _code);
      Element::writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void FSymbol::read(QDomElement e)
      {
      QPointF pos;

      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            QString val(e.text());
            if (tag == "font")
                  _font.setFamily(val);
            else if (tag == "fontsize")
                  _font.setPixelSize(val.toInt());
            else if (tag == "code")
                  _code = val.toInt();
            else if (!Element::readProperties(e))
                  domError(e);
            }
      setPos(pos);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void FSymbol::layout()
      {
      QString s;
      if (_code & 0xffff0000) {
            s = QChar(QChar::highSurrogate(_code));
            s += QChar(QChar::lowSurrogate(_code));
            }
      else
            s = QChar(_code);
      QFontMetricsF fm(_font);
      setbbox(fm.boundingRect(s));
      adjustReadPos();
      }

//---------------------------------------------------------
//   setFont
//---------------------------------------------------------

void FSymbol::setFont(const QFont& f)
      {
      _font = f;
      _font.setStyleStrategy(QFont::NoFontMerging);
      }

