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

#include "note.h"
#include "chord.h"
#include "xml.h"
#include "slur.h"
#include "measure.h"
#include "utils.h"
#include "score.h"
#include "system.h"
#include "segment.h"
#include "staff.h"
#include "navigate.h"
#include "articulation.h"
#include "undo.h"
#include "stem.h"
#include "beam.h"
#include "painter.h"
#include "mscore.h"
#include "page.h"

//---------------------------------------------------------
//   SlurSegment
//---------------------------------------------------------

SlurSegment::SlurSegment(Score* score)
   : SpannerSegment(score)
      {
      }

SlurSegment::SlurSegment(const SlurSegment& b)
   : SpannerSegment(b)
      {
      for (int i = 0; i < SLUR_GRIPS; ++i)
            ups[i] = b.ups[i];
      path = b.path;
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void SlurSegment::move(const QPointF& s)
      {
      movePos(s);
      for (int k = 0; k < SLUR_GRIPS; ++k)
            ups[k].p += s;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void SlurSegment::draw(Painter* painter) const
      {
      if (slurTie()->lineType() == 0) {
            painter->setBrushColor(curColor());
            painter->setCapStyle(Qt::RoundCap);
            painter->setJoinStyle(Qt::RoundJoin);
            qreal lw = point(score()->styleS(ST_SlurEndWidth));
            painter->setLineWidth(lw);
            }
      else {
            painter->setNoBrush(true);
            qreal lw = point(score()->styleS(ST_SlurDottedWidth));
            painter->setLineWidth(lw);
            painter->setLineStyle(Qt::DotLine);
            }
      painter->drawPath(path);
      }

//---------------------------------------------------------
//   updateGrips
//    return grip rectangles in page coordinates
//---------------------------------------------------------

void SlurSegment::updateGrips(int* n, QRectF* r) const
      {
      *n = SLUR_GRIPS;
      QPointF p(pagePos());
      for (int i = 0; i < SLUR_GRIPS; ++i)
            r[i].translate(ups[i].p + ups[i].off * spatium() + p);
      }

//---------------------------------------------------------
//   edit
//    return true if event is accepted
//---------------------------------------------------------

bool SlurSegment::edit(MuseScoreView* viewer, int curGrip, int key, Qt::KeyboardModifiers modifiers, const QString&)
      {
      Slur* sl = static_cast<Slur*>(slurTie());

      if (key == Qt::Key_X) {
            sl->setSlurDirection(sl->up() ? DOWN : UP);
            sl->layout();
            return true;
            }
      if (slurTie()->type() != SLUR)
            return false;

      if (!((modifiers & Qt::ShiftModifier)
         && ((spannerSegmentType() == SEGMENT_SINGLE)
              || (spannerSegmentType() == SEGMENT_BEGIN && curGrip == 0)
              || (spannerSegmentType() == SEGMENT_END && curGrip == 3)
            )))
            return false;

      ChordRest* cr = 0;
      Element* e    = curGrip == 0 ? sl->startElement() : sl->endElement();
      Element* e1   = curGrip == 0 ? sl->endElement() : sl->startElement();

      if (key == Qt::Key_Left)
            cr = prevChordRest((ChordRest*)e);
      else if (key == Qt::Key_Right)
            cr = nextChordRest((ChordRest*)e);

      if (cr == 0 || cr == (ChordRest*)e1)
            return true;
      changeAnchor(viewer, curGrip, cr);
      return true;
      }

//---------------------------------------------------------
//   changeAnchor
//---------------------------------------------------------

void SlurSegment::changeAnchor(MuseScoreView* viewer, int curGrip, ChordRest* cr)
      {
      Slur* sl = static_cast<Slur*>(slurTie());
      if (curGrip == 0) {
            ((ChordRest*)sl->startElement())->removeSlurFor(sl);
            sl->setStartElement(cr);
            cr->addSlurFor(sl);
            }
      else {
            ((ChordRest*)sl->endElement())->removeSlurBack(sl);
            sl->setEndElement(cr);
            cr->addSlurBack(sl);
            }

      int segments  = sl->spannerSegments().size();
      ups[curGrip].off = QPointF();
      sl->layout();
      if (sl->spannerSegments().size() != segments) {
            SlurSegment* newSegment = curGrip == 3 ? sl->backSegment() : sl->frontSegment();
            score()->endCmd();
            score()->startCmd();
            viewer->startEdit(newSegment, curGrip);
            score()->setLayoutAll(true);
            }
      }

//---------------------------------------------------------
//   gripAnchor
//---------------------------------------------------------

QPointF SlurSegment::gripAnchor(int grip) const
      {
      SlurPos spos;
      slurTie()->slurPos(&spos);

      QPointF sp(system()->pagePos());
      QPointF p1(spos.p1 + spos.system1->pagePos());
      QPointF p2(spos.p2 + spos.system2->pagePos());
      switch(spannerSegmentType()) {
            case SEGMENT_SINGLE:
                  if (grip == GRIP_START)
                        return p1;
                  else if (grip == GRIP_END)
                        return p2;
                  break;

            case SEGMENT_BEGIN:
                  if (grip == GRIP_START)
                        return p1;
                  else if (grip == GRIP_END)
                        return system()->abbox().topRight();
                  break;

            case SEGMENT_MIDDLE:
                  if (grip == GRIP_START)
                        return sp;
                  else if (grip == GRIP_END)
                        return system()->abbox().topRight();
                  break;

            case SEGMENT_END:
                  if (grip == GRIP_START)
                        return sp;
                  else if (grip == GRIP_END)
                        return p2;
                  break;
            }
      return QPointF();
      }

//---------------------------------------------------------
//   getGrip
//---------------------------------------------------------

QPointF SlurSegment::getGrip(int n) const
      {
      switch(n) {
            case GRIP_START:
            case GRIP_END:
                  return (ups[n].p - gripAnchor(n)) / spatium() + ups[n].off;
            default:
                  return ups[n].off;
            }
      }

//---------------------------------------------------------
//   setGrip
//---------------------------------------------------------

void SlurSegment::setGrip(int n, const QPointF& pt)
      {
      switch(n) {
            case GRIP_START:
            case GRIP_END:
                  ups[n].off = ((pt * spatium()) - (ups[n].p - gripAnchor(n))) / spatium();
                  break;
            default:
                  ups[n].off = pt;
                  break;
            }
      slurTie()->layout();
      }

//---------------------------------------------------------
//   editDrag
//---------------------------------------------------------

void SlurSegment::editDrag(const EditData& ed)
      {
      ups[ed.curGrip].off += (ed.delta / spatium());
      computeBezier();
      if (ed.curGrip == GRIP_START || ed.curGrip == GRIP_END) {
            //
            // move anchor for slurs
            //
            Slur* slur = static_cast<Slur*>(slurTie());
            if ((slur->type() == SLUR)
               && (
                  (ed.curGrip == GRIP_START && (spannerSegmentType() == SEGMENT_SINGLE || spannerSegmentType() == SEGMENT_BEGIN))
                  || (ed.curGrip == GRIP_END && (spannerSegmentType() == SEGMENT_SINGLE || spannerSegmentType() == SEGMENT_END))
                  )
               ) {
                  Element* e = ed.view->elementNear(ed.pos);
                  if (e && e->type() == NOTE) {
                        Chord* chord = static_cast<Note*>(e)->chord();
                        if ((ed.curGrip == 3 && chord != slur->endElement())
                           || (ed.curGrip == 0 && chord != slur->startElement())) {
                              changeAnchor(ed.view, ed.curGrip, chord);
                              QPointF p1 = ed.pos - ups[ed.curGrip].p - pagePos();
                              ups[ed.curGrip].off = p1 / spatium();
                              return;
                              }
                        }
                  }
            }
      else if (ed.curGrip == GRIP_DRAG) {
            ups[GRIP_DRAG].off = QPointF();
            setUserOff(userOff() + ed.delta);
            }
      }

//---------------------------------------------------------
//    bbox
//---------------------------------------------------------

QRectF SlurSegment::bbox() const
      {
      return path.boundingRect();
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void SlurSegment::write(Xml& xml, int no) const
      {
      if (ups[GRIP_START].off.isNull()
         && ups[GRIP_END].off.isNull()
         && ups[GRIP_BEZIER1].off.isNull()
         && ups[GRIP_BEZIER2].off.isNull()
         && userOff().isNull()
         && visible())
            return;

      xml.stag(QString("SlurSegment no=\"%1\"").arg(no));
      if (!visible())
            xml.tag("visible", visible());
      if (!userOff().isNull())
            xml.tag("offset", userOff() / spatium());
      if (!(ups[GRIP_START].off.isNull()))
            xml.tag("o1", ups[0].off);
      if (!(ups[GRIP_BEZIER1].off.isNull()))
            xml.tag("o2", ups[1].off);
      if (!(ups[GRIP_BEZIER2].off.isNull()))
            xml.tag("o3", ups[2].off);
      if (!(ups[GRIP_END].off.isNull()))
            xml.tag("o4", ups[3].off);
      xml.etag();
      }

//---------------------------------------------------------
//   readSegment
//---------------------------------------------------------

void SlurSegment::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            if (tag == "o1")
                  ups[GRIP_START].off = readPoint(e);
            else if (tag == "o2")
                  ups[GRIP_BEZIER1].off = readPoint(e);
            else if (tag == "o3")
                  ups[GRIP_BEZIER2].off = readPoint(e);
            else if (tag == "o4")
                  ups[GRIP_END].off = readPoint(e);
            else if (!Element::readProperties(e))
                  domError(e);
            }
      }

//---------------------------------------------------------
//   computeBezier
//    compute help points of slur bezier segment
//---------------------------------------------------------

void SlurSegment::computeBezier()
      {
      qreal _spatium  = spatium();
      qreal shoulderW;              // height as fraction of slur-length
      qreal shoulderH;
      //
      // p1 and p2 are the end points of the slur
      //
      QPointF pp1 = ups[GRIP_START].p + ups[GRIP_START].off * _spatium;
      QPointF pp2 = ups[GRIP_END].p   + ups[GRIP_END].off   * _spatium;
      QPointF p6o = ups[GRIP_SHOULDER].off * _spatium;

      QPointF p2 = pp2 - pp1;
      if (p2.x() == 0.0) {
            printf("zero slur\n");
            if (slurTie()->type() == SLUR) {
                  Slur* s = static_cast<Slur*>(slurTie());
                  Measure* m1 = s->startChord()->segment()->measure();
                  Measure* m2 = s->endChord()->segment()->measure();
                  Page* page = m1->system()->page();
                  printf("   at tick %d in measure %d-%d page %d\n",
                     m1->tick(), m1->no(), m2->no(), page->no());
                  }
            return;
            }

      qreal sinb = atan(p2.y() / p2.x());
      QTransform t;
      t.rotateRadians(-sinb);
      p2  = t.map(p2);
      p6o = t.map(p6o);

      double smallH = 0.5;
      qreal d = p2.x() / _spatium;
      if (d <= 2.0) {
            shoulderH = d * 0.5 * smallH * _spatium;
            shoulderW = .6;
            }
      else {
            qreal dd = log10(1.0 + (d - 2.0) * .5) * 2.0;
            if (dd > 3.0)
                  dd = 3.0;
            shoulderH = (dd + smallH) * _spatium;
            if (d > 18.0)
                  shoulderW = 0.8;
            else if (d > 10)
                  shoulderW = 0.7;
            else
                  shoulderW = 0.6;
            }

      if (!slurTie()->up())
            shoulderH = -shoulderH;
      shoulderH -= p6o.y();

      qreal c    = p2.x();
      qreal c1   = (c - c * shoulderW) * .5 + p6o.x();
      qreal c2   = c1 + c * shoulderW       + p6o.x();

      QPointF p5 = QPointF(c * .5, 0.0);

      QPointF p3(c1, -shoulderH);
      QPointF p4(c2, -shoulderH);

      qreal w = (score()->styleS(ST_SlurMidWidth).val() - score()->styleS(ST_SlurEndWidth).val()) * _spatium;
      QPointF th(0.0, w);    // thickness of slur

      QPointF p3o = t.map(ups[GRIP_BEZIER1].off * _spatium);
      QPointF p4o = t.map(ups[GRIP_BEZIER2].off * _spatium);

      //-----------------------------------calculate p6
      QPointF pp3  = p3 + p3o;
      QPointF pp4  = p4 + p4o;
      QPointF ppp4 = pp4 - pp3;

      qreal r2 = atan(ppp4.y() / ppp4.x());
      t.reset();
      t.rotateRadians(-r2);
      QPointF p6  = QPointF(t.map(ppp4).x() * .5, 0.0);

      t.rotateRadians(2 * r2);
      p6 = t.map(p6) + pp3 - p6o;
      //-----------------------------------

      path = QPainterPath();
      path.moveTo(QPointF());
      path.cubicTo(p3 + p3o - th, p4 + p4o - th, p2);
      if (slurTie()->lineType() == 0)
            path.cubicTo(p4 +p4o + th, p3 + p3o + th, QPointF());

      th = QPointF(0.0, 3.0 * w);
      shapePath = QPainterPath();
      shapePath.moveTo(QPointF());
      shapePath.cubicTo(p3 + p3o - th, p4 + p4o - th, p2);
      shapePath.cubicTo(p4 +p4o + th, p3 + p3o + th, QPointF());

      // translate back
      t.reset();
      t.translate(pp1.x(), pp1.y());
      t.rotateRadians(sinb);
      path                 = t.map(path);
      shapePath            = t.map(shapePath);
      ups[GRIP_BEZIER1].p  = t.map(p3);
      ups[GRIP_BEZIER2].p  = t.map(p4);
      ups[GRIP_END].p      = t.map(p2) - ups[GRIP_END].off * _spatium;
      ups[GRIP_DRAG].p     = t.map(p5);
      ups[GRIP_SHOULDER].p = t.map(p6);
      }

//---------------------------------------------------------
//   layout
//    p1, p2  are in System coordinates
//---------------------------------------------------------

void SlurSegment::layout(const QPointF& p1, const QPointF& p2)
      {
      ups[GRIP_START].p = p1;
      ups[GRIP_END].p   = p2;
      computeBezier();
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void SlurSegment::dump() const
      {
      printf("SlurSegment %f/%f %f/%f %f/%f %f/%f\n",
            ups[GRIP_START].off.x(), ups[GRIP_START].off.y(),
            ups[GRIP_BEZIER1].off.x(), ups[GRIP_BEZIER1].off.y(),
            ups[GRIP_BEZIER2].off.x(), ups[GRIP_BEZIER2].off.y(),
            ups[GRIP_END].off.x(), ups[GRIP_END].off.y());
      }

//---------------------------------------------------------
//   SlurTie
//---------------------------------------------------------

SlurTie::SlurTie(Score* s)
   : Spanner(s)
      {
      _slurDirection = AUTO;
      _up            = true;
      _len           = 0;
      _lineType      = 0;     // default is solid
      }

SlurTie::SlurTie(const SlurTie& t)
   : Spanner(t)
      {
      _up            = t._up;
      _slurDirection = t._slurDirection;
      _len           = t._len;
      _lineType      = t._lineType;
      // delSegments    = t.delSegments;
      }

//---------------------------------------------------------
//   SlurTie
//---------------------------------------------------------

SlurTie::~SlurTie()
      {
      }

//---------------------------------------------------------
//   fixArticulations
//---------------------------------------------------------

static qreal fixArticulations(qreal yo, Chord* c, qreal _up)
      {
      //
      // handle special case of tenuto and staccato;
      //
      QList<Articulation*>* al = c->getArticulations();
      if (al->size() >= 2) {
            Articulation* a = al->at(1);
            if (a->subtype() == Articulation_Tenuto || a->subtype() == Articulation_Staccato)
                  return a->y() + (a->height() + c->score()->spatium() * .3) * _up;
            }
      else if (al->size() >= 1) {
            Articulation* a = al->at(0);
            if (a->subtype() == Articulation_Tenuto || a->subtype() == Articulation_Staccato)
                  return a->y() + (a->height() + c->score()->spatium() * .3) * _up;
            }
      return yo;
      }

//---------------------------------------------------------
//   slurPos
//    calculate position of start- and endpoint of slur
//    relative to System() position
//---------------------------------------------------------

void SlurTie::slurPos(SlurPos* sp)
      {
      qreal _spatium = spatium();
      Element* e1 = startElement();
      Element* e2 = endElement();
      bool isTie  = e1->type() == NOTE;

      Note* note1;
      Note* note2;
      Chord* sc;
      Chord* ec;

      if (isTie) {
            note1 = static_cast<Note*>(e1);
            note2 = static_cast<Note*>(e2);
            sc    = note1->chord();
            ec    = note2->chord();
            }
      else {
            if ((e1->type() != CHORD) || (e2->type() != CHORD)) {
                  sp->p1 = e1->pagePos();
                  sp->p2 = e2->pagePos();
                  sp->p1.rx() += e1->width();
                  sp->p2.rx() += e2->width();
                  sp->system1 = static_cast<ChordRest*>(e1)->measure()->system();
                  sp->system2 = static_cast<ChordRest*>(e2)->measure()->system();
                  return;
                  }
            sc    = static_cast<Chord*>(e1);
            ec    = static_cast<Chord*>(e2);
            note1 = _up ? sc->upNote() : sc->downNote();
            note2 = _up ? ec->upNote() : ec->downNote();
            }
      sp->p1      = sc->pagePos();
      sp->p2      = ec->pagePos();
      sp->system1 = sc->measure()->system();
      sp->system2 = ec->measure()->system();

      qreal xo, yo;

      Stem* stem1 = sc->stem();
      Stem* stem2 = ec->stem();

      //
      // default position:
      //    horizontal: middle of note head
      //    vertical:   _spatium * .4 above/below note head
      //
      qreal hw  = note1->headWidth();
      qreal hh  = note1->headHeight();
      qreal __up = _up ? -1.0 : 1.0;

      //------p1
      xo = hw * .5;
      yo = 0.0;

      bool stemPos = false;   // p1 starts at chord stem side

      if (isTie && sc->notes().size() > 1) {
            xo = hw * 1.12;
            yo = note1->pos().y() + hw * .3 * __up;
            }
      else {
            yo = note1->yPos() + (hh * .5 + _spatium * .4) * __up;
            if (stem1) {
                  // bool startIsGrace = sc->noteType() != NOTE_NORMAL;

                  Beam* beam1 = sc->beam();
                  if (beam1 && (beam1->elements().back() != sc) && (sc->up() == _up)) {
                        qreal sh = stem1->height() + _spatium;
                        if (_up)
                              yo = sc->downNote()->yPos() - sh;
                        else
                              yo = sc->upNote()->yPos() + sh;
                        xo = stem1->pos().x();
                        stemPos = true;
                        }
                  else {
                        if (sc->up() && _up)
                              xo = note1->headWidth() + _spatium * .3;

                        //
                        // handle case: stem up   - stem down
                        //              stem down - stem up
                        //
                        if ((sc->up() != ec->up()) && (sc->up() == _up)) {
                              Note* n1  = sc->up() ? sc->downNote() : sc->upNote();
                              Note* n2  = ec->up() ? ec->downNote() : ec->upNote();
                              qreal yd = n2->yPos() - n1->yPos();

                              qreal sh = stem1->height();    // limit y move
                              if (yd > 0.0) {
                                    if (yd > sh)
                                          yd = sh;
                                    }
                              else {
                                    if (yd < - sh)
                                          yd = -sh;
                                    }
                              stemPos = true;
                              if ((_up && (yd < -_spatium)) || (!_up && (yd > _spatium)))
                                    yo += yd;
                              }
                        else if (sc->up() != _up)
                              yo = fixArticulations(yo, sc, __up);
                        }
                  }
            }
      sp->p1 += QPointF(xo, yo);

      //------p2
      xo = hw * .5;
      yo = 0.0;
      if (isTie && ec->notes().size() > 1) {
            xo = - hw * 0.12;
            yo = note2->pos().y() + hw * .3 * __up;
            }
      else {
            yo = note2->yPos() + (hh * .5 + _spatium * .4) * __up;
            if (stem2) {
                  Beam* beam2 = ec->beam();
                  if ((stemPos && (sc->up() == ec->up()))
                     || (beam2
                       && (!beam2->elements().isEmpty())
                       && (beam2->elements().front() != ec)
                       && (ec->up() == _up)
                       && (sc->noteType() == NOTE_NORMAL)
                       )
                        ) {
                        qreal sh = stem2->height() + _spatium;
                        if (_up)
                              yo = ec->downNote()->yPos() - sh;
                        else
                              yo = ec->upNote()->yPos() + sh;
                        xo = stem2->pos().x();
                        }
                  else if (!ec->up() && !_up)
                        xo = -_spatium * .3;
                  //
                  // handle case: stem up   - stem down
                  //              stem down - stem up
                  //
                  if ((sc->up() != ec->up()) && (ec->up() == _up)) {
                        Note* n1 = sc->up() ? sc->downNote() : sc->upNote();
                        Note* n2 = ec->up() ? ec->downNote() : ec->upNote();
                        qreal yd = n2->yPos() - n1->yPos();

                        qreal mh = stem2->height();    // limit y move
                        if (yd > 0.0) {
                              if (yd > mh)
                                    yd = mh;
                              }
                        else {
                              if (yd < - mh)
                                    yd = -mh;
                              }

                        if ((_up && (yd > _spatium)) || (!_up && (yd < -_spatium)))
                              yo -= yd;
                        }
                  else if (ec->up() != _up)
                        yo = fixArticulations(yo, ec, __up);
                  }
            }
      sp->p2 += QPointF(xo, yo);

      sp->p1 -= sp->system1->pagePos();
      sp->p2 -= sp->system2->pagePos();
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void SlurTie::writeProperties(Xml& xml) const
      {
      Element::writeProperties(xml);
      int idx = 0;
      foreach(const SpannerSegment* ss, spannerSegments())
            ((SlurSegment*)ss)->write(xml, idx++);
      if (_slurDirection)
            xml.tag("up", _slurDirection);
      if (_lineType)
            xml.tag("lineType", _lineType);
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool SlurTie::readProperties(QDomElement e)
      {
      QString tag(e.tagName());
      QString val(e.text());

      if (tag == "SlurSegment") {
            int idx = e.attribute("no", 0).toInt();
            int n = spannerSegments().size();
            for (int i = n; i < idx; ++i)
                  add(new SlurSegment(score()));
            SlurSegment* segment = new SlurSegment(score());
            segment->read(e);
            add(segment);
            }
      else if (tag == "up")
            _slurDirection = Direction(val.toInt());
      else if (tag == "lineType")
            _lineType = val.toInt();
      else if (!Element::readProperties(e))
            return false;
      return true;
      }

//---------------------------------------------------------
//   toDefault
//---------------------------------------------------------

void SlurTie::toDefault()
      {
      score()->undoChangeUserOffset(this, QPointF());
      }

void SlurSegment::toDefault()
      {
      score()->undoChangeUserOffset(this, QPointF());
      score()->undo()->push(new ChangeSlurOffsets(this, QPointF(), QPointF(), QPointF(), QPointF()));
      for (int i = 0; i < SLUR_GRIPS; ++i)
            ups[i].off = QPointF();
      parent()->toDefault();
      parent()->layout();
      }

//---------------------------------------------------------
//   Slur
//---------------------------------------------------------

Slur::Slur(Score* s)
   : SlurTie(s)
      {
      setId(-1);
      _track2 = 0;
      }

//---------------------------------------------------------
//   Slur
//---------------------------------------------------------

Slur::~Slur()
      {
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Slur::write(Xml& xml) const
      {
      xml.stag(QString("Slur id=\"%1\"").arg(id() + 1));
      SlurTie::writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Slur::read(QDomElement e)
      {
      setTrack(0);      // set staff
      setId(e.attribute("id").toInt());
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            QString val(e.text());
            int i = val.toInt();
//            if (tag == "tick2")
//                  _tick2 = score()->fileDivision(i);
            if (tag == "track2")
                  _track2 = i;
//            else if (tag == "startTick")        // obsolete
//                  ; //                  setTick(i);
//            else if (tag == "endTick")          // obsolete
//                  setTick2(i);
            else if (tag == "startTrack")       // obsolete
                  setTrack(i);
            else if (tag == "endTrack")         // obsolete
                  setTrack2(i);
            else if (!SlurTie::readProperties(e))
                  domError(e);
            }
      }

//---------------------------------------------------------
//   chordsHaveTie
//---------------------------------------------------------

static bool chordsHaveTie(Chord* c1, Chord* c2)
      {
      foreach(Note* n1, c1->notes()) {
            foreach(Note* n2, c2->notes()) {
                  if (n1->tieFor() && n1->tieFor() == n2->tieBack())
                        return true;
                  }
            }
      return false;
      }

//---------------------------------------------------------
//   directionMixture
//---------------------------------------------------------

static bool isDirectionMixture(Chord* c1, Chord* c2)
      {
      bool up = c1->up();
      for (Segment* seg = c1->segment(); seg; seg = seg->next(SegChordRest)) {
            Chord* c = static_cast<Chord*>(seg->element(c1->track()));
            if (!c || c->type() != CHORD)
                  continue;
            if (c->up() != up)
                  return true;
            if (seg == c2->segment())
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Slur::layout()
      {
      qreal _spatium = spatium();

      if (score() == gscore || !startElement()) {
            //
            // when used in a palette, slur has no parent and
            // tick and tick2 has no meaning so no layout is
            // possible and needed
            //
            setLen(_spatium * 7);
            SlurSegment* s;
            if (spannerSegments().isEmpty()) {
                  s = new SlurSegment(score());
                  s->setTrack(track());
                  add(s);
                  }
            else {
                  s = frontSegment();
                  }
            s->setSpannerSegmentType(SEGMENT_SINGLE);
            s->layout(QPointF(0, 0), QPointF(_len, 0));
            return;
            }
      switch (_slurDirection) {
            case UP:
                  _up = true;
                  break;
            case DOWN:
                  _up = false;
                  break;
            case AUTO:
                  {
                  //
                  // assumption:
                  // slurs have only chords or rests as start/end elements
                  //
                  ChordRest* cr1 = static_cast<ChordRest*>(startElement());
                  ChordRest* cr2 = static_cast<ChordRest*>(endElement());
                  Measure* m1    = cr1->measure();

                  Chord* c1 = (cr1->type() == CHORD) ? static_cast<Chord*>(cr1) : 0;
                  Chord* c2 = (cr2->type() == CHORD) ? static_cast<Chord*>(cr2) : 0;

                  _up = !(cr1->up());

                  if ((cr2->tick() - cr1->tick()) > m1->ticks()) {
                        // long slurs are always above
                        _up = true;
                        }
                  else
                        _up = !(cr1->up());

                  if (c1 && c2 && isDirectionMixture(c1, c2) && (c1->noteType() == NOTE_NORMAL)) {
                        // slurs go above if start and end note have different stem directions,
                        // but grace notes are exceptions
                        _up = true;
                        }
                  else if (m1->mstaff(cr1->staffIdx())->hasVoices && c1 && c1->noteType() == NOTE_NORMAL) {
                        // in polyphonic passage, slurs go on the stem side
                        _up = cr1->up();
                        }
                  else if (c1 && c2 && chordsHaveTie(c1, c2)) {
                        // could confuse slur with tie, put slur on stem side
                        _up = cr1->up();
                        }
                  }
                  break;
            }

      SlurPos sPos;
      slurPos(&sPos);

      QList<System*>* sl = score()->systems();
      iSystem is = sl->begin();
      while (is != sl->end()) {
            if (*is == sPos.system1)
                  break;
            ++is;
            }
      if (is == sl->end())
            printf("Slur::layout  first system not found\n");
      setPos(0, 0);

      //---------------------------------------------------------
      //   count number of segments, if no change, all
      //    user offsets (drags) are retained
      //---------------------------------------------------------

      unsigned nsegs = 1;
      for (iSystem iis = is; iis != sl->end(); ++iis) {
            if ((*iis)->isVbox())
                  continue;
            if (*iis == sPos.system2)
                  break;
            ++nsegs;
            }

      unsigned onsegs = spannerSegments().size();
      if (nsegs > onsegs) {
            for (unsigned i = onsegs; i < nsegs; ++i) {
                  SlurSegment* s;
                  if (!delSegments.isEmpty()) {
                        s = delSegments.dequeue();
                        }
                  else {
                        s = new SlurSegment(score());
                        }
                  s->setTrack(track());
                  add(s);
                  }
            }
      else if (nsegs < onsegs) {
            for (unsigned i = nsegs; i < onsegs; ++i) {
                  SlurSegment* s = takeLastSegment();
                  if (s->system())
                        s->system()->remove(s);
                  delSegments.enqueue(s);  // cannot delete: used in SlurSegment->edit()
                  }
            }

      for (int i = 0; is != sl->end(); ++i, ++is) {
            System* system  = *is;
            if (system->isVbox()) {
                  --i;
                  continue;
                  }
            SlurSegment* segment = segmentAt(i);
            segment->setSystem(system);

            // case 1: one segment
            if (sPos.system1 == sPos.system2) {
                  segment->setSubtype(SEGMENT_SINGLE);
                  segment->layout(sPos.p1, sPos.p2);
                  }
            // case 2: start segment
            else if (i == 0) {
                  segment->setSubtype(SEGMENT_BEGIN);
                  qreal x = system->bbox().width();
                  segment->layout(sPos.p1, QPointF(x, sPos.p1.y()));
                  }
            // case 3: middle segment
            else if (i != 0 && system != sPos.system2) {
                  segment->setSubtype(SEGMENT_MIDDLE);
                  qreal x1 = firstNoteRestSegmentX(system) - _spatium;
                  qreal x2 = system->bbox().width();
                  qreal y  = system->staff(startElement()->staffIdx())->y();
                  segment->layout(QPointF(x1, y), QPointF(x2, y));
                  }
            // case 4: end segment
            else {
                  segment->setSubtype(SEGMENT_END);
                  qreal x = firstNoteRestSegmentX(system) - _spatium;
                  segment->layout(QPointF(x, sPos.p2.y()), sPos.p2);
                  }
            if (system == sPos.system2)
                  break;
            }
      }

//---------------------------------------------------------
//   firstNoteRestSegmentX
//    in System() coordinates
//---------------------------------------------------------

qreal SlurTie::firstNoteRestSegmentX(System* system)
      {
      foreach(const MeasureBase* mb, system->measures()) {
            if (mb->type() == MEASURE) {
                  const Measure* measure = static_cast<const Measure*>(mb);
                  for (const Segment* seg = measure->first(); seg; seg = seg->next()) {
                        if (seg->subtype() == SegChordRest) {
                              return seg->pos().x() + seg->measure()->pos().x();
                              }
                        }
                  }
            }
      printf("firstNoteRestSegmentX: did not find segment\n");
      return 0.0;
      }

//---------------------------------------------------------
//   bbox
//    used in palette
//---------------------------------------------------------

QRectF Slur::bbox() const
      {
      if (spannerSegments().isEmpty())
            return QRectF();
      else
            return frontSegment()->bbox();
      }

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void Slur::setTrack(int n)
      {
      Element::setTrack(n);
      foreach(SpannerSegment* ss, spannerSegments())
            ss->setTrack(n);
      }

//---------------------------------------------------------
//   Tie
//---------------------------------------------------------

Tie::Tie(Score* s)
   : SlurTie(s)
      {
      }

//---------------------------------------------------------
//   setStartNote
//---------------------------------------------------------

void Tie::setStartNote(Note* note)
      {
      setStartElement(note);
      setParent(note);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Tie::write(Xml& xml) const
      {
      xml.stag("Tie");
      SlurTie::writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Tie::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            QString val(e.text());
            if (Element::readProperties(e))
                  ;
            else if (SlurTie::readProperties(e))
                  ;
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Tie::layout()
      {
      //
      // TODO: if there is a startNote but no endNote
      //    show short bow
      if (startElement() == 0 || endElement() == 0) {
            printf("Tie::layout(): no start or end\n");
            return;
            }

      qreal _spatium = spatium();

      Chord* c1   = startNote()->chord();
      Measure* m1 = c1->measure();
      // Chord* c2   = endNote()->chord();

      if (_slurDirection == AUTO)
            if (m1->mstaff(c1->staffIdx())->hasVoices) {
                  // in polyphonic passage, ties go on the stem side
                  _up = c1->up();
                  }
            else
                  _up = !(c1->up());
      else
            _up = _slurDirection == UP ? true : false;
      qreal w   = startNote()->headWidth();
      qreal xo1 = w * 1.12;
      qreal h   = w * 0.3;
      qreal yo  = _up ? -h : h;

      QPointF off1(xo1, yo);
      QPointF off2(0.0, yo);

      QPointF ppos(pagePos());

      // TODO: cleanup

      SlurPos sPos;
      slurPos(&sPos);

      // p1, p2, s1, s2

      QList<System*>* systems = score()->systems();
      setPos(0, 0);

      //---------------------------------------------------------
      //   count number of segments, if no change, all
      //    user offsets (drags) are retained
      //---------------------------------------------------------

      int sysIdx1 = systems->indexOf(sPos.system1);
      if (sysIdx1 == -1) {
            printf("system not found\n");
            foreach(System* s, *systems)
                  printf("   search %p in %p\n", sPos.system1, s);
            return;
            }

      int sysIdx2     = systems->indexOf(sPos.system2, sysIdx1);
      unsigned nsegs  = sysIdx2 - sysIdx1 + 1;
      unsigned onsegs = spannerSegments().size();

      if (nsegs != onsegs) {
            if (nsegs > onsegs) {
                  int n = nsegs - onsegs;
                  for (int i = 0; i < n; ++i) {
                        SlurSegment* s = new SlurSegment(score());
                        add(s);
                        }
                  }
            else {
                  int n = onsegs - nsegs;
                  for (int i = 0; i < n; ++i) {
                        /* LineSegment* seg = */ takeLastSegment();
                        // delete seg;   // DEBUG: will be used later
                        }
                  }
            }

      int i = 0;
      for (uint ii = 0; ii < nsegs; ++ii) {
            System* system = (*systems)[sysIdx1++];
            if (system->isVbox())
                  continue;
            SlurSegment* segment = segmentAt(i);
            segment->setSystem(system);

            // case 1: one segment
            if (sPos.system1 == sPos.system2) {
                  segment->layout(sPos.p1, sPos.p2);
                  segment->setSpannerSegmentType(SEGMENT_SINGLE);
                  }
            // case 2: start segment
            else if (i == 0) {
                  qreal x = system->bbox().width();
                  segment->layout(sPos.p1, QPointF(x, sPos.p1.y()));
                  segment->setSpannerSegmentType(SEGMENT_BEGIN);
                  }
            // case 4: end segment
            else {
                  qreal x = firstNoteRestSegmentX(system) - 2 * _spatium;

                  segment->layout(QPointF(x, sPos.p2.y()), sPos.p2);
                  segment->setSpannerSegmentType(SEGMENT_END);
                  }
            ++i;
            }
      }

