//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2009 Werner Schweer and others
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

#include "pianoscene.h"
#include "staff.h"
#include "piano.h"
#include "measure.h"
#include "chord.h"
#include "score.h"
#include "note.h"

static const int MAP_OFFSET = 480;

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

static int pitch2y(int pitch)
      {
      static int tt[] = {
            12, 19, 25, 32, 38, 51, 58, 64, 71, 77, 84, 90
            };
      int y = (75 * keyHeight) - (tt[pitch % 12] + (7 * keyHeight) * (pitch / 12));
      if (y < 0)
            y = 0;
      return y;
      }

//---------------------------------------------------------
//   PianoItem
//---------------------------------------------------------

PianoItem::PianoItem(Note* n)
   : QGraphicsRectItem()
      {
      setFlags(flags() | QGraphicsItem::ItemIsSelectable);
      int pitch    = n->pitch();
      Chord* chord = n->chord();
      int x        = chord->tick() + 480;
      int len      = chord->tickLen();
      int y        = pitch2y(pitch) + keyHeight/4;
      setRect(x, y, len, keyHeight/2);
      setBrush(QColor(Qt::blue));
      setPen(QPen());
      setSelected(n->selected());
      setData(0, QVariant::fromValue<void*>(n));
      }

//---------------------------------------------------------
//   PianoScene
//---------------------------------------------------------

PianoScene::PianoScene(QWidget* parent)
   : QGraphicsScene(parent)
      {
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoScene::setStaff(Staff* staff)
      {
      clear();
      Measure* m = staff->score()->firstMeasure();
      int staffIdx = staff->idx();
      int startTrack = staffIdx * VOICES;
      int endTrack   = startTrack + VOICES;
      for (Segment* s = m->first(); s; s = s->next1()) {
            for (int track = startTrack; track < endTrack; ++track) {
                  Element* e = s->element(track);
                  if (e == 0 || e->type() != CHORD)
                        continue;
                  Chord* chord = static_cast<Chord*>(e);
                  NoteList* nl = chord->noteList();
                  for (iNote in = nl->begin(); in != nl->end(); ++in)
                        addItem(new PianoItem(in->second));
                  }
            }
      }

//---------------------------------------------------------
//   pix2pos
//---------------------------------------------------------

AL::Pos PianoView::pix2pos(int x) const
      {
      x -= MAP_OFFSET;
      if (x < 0)
            x = 0;
      return AL::Pos(staff->score()->tempomap(), staff->score()->sigmap(), x, _timeType);
      }

//---------------------------------------------------------
//   pos2pix
//---------------------------------------------------------

int PianoView::pos2pix(const AL::Pos& p) const
      {
      return p.time(_timeType) + MAP_OFFSET;
      }

//---------------------------------------------------------
//   drawBackground
//---------------------------------------------------------

void PianoView::drawBackground(QPainter* p, const QRectF& r)
      {
      Score* _score = staff->score();

      QRectF r1;
      r1.setCoords(-1000000.0, 0.0, 480.0, 1000000.0);
      QRectF r2;
      r2.setCoords(ticks + 480, 0.0, 1000000.0, 1000000.0);
      QColor bg(0x71, 0x8d, 0xbe);

      p->fillRect(r, bg);
      if (r.intersects(r1))
            p->fillRect(r.intersected(r1), bg.darker(150));
      if (r.intersects(r2))
            p->fillRect(r.intersected(r2), bg.darker(150));

      //
      // draw horizontal grid lines
      //
      qreal y1 = r.y();
      qreal y2 = y1 + r.height();
      qreal kh = 13.0;
      qreal x1 = r.x();
      qreal x2 = x1 + r.width();

      int key = floor(y1 / 75);
      qreal y = key * kh;

      for (; key < 75; ++key, y += kh) {
            if (y < y1)
                  continue;
            if (y > y2)
                  break;
            p->setPen(QPen((key % 7) == 5 ? Qt::lightGray : Qt::gray));
            p->drawLine(QLineF(x1, y, x2, y));
            }

      //
      // draw vertical grid lines
      //
      static const int mag[7] = {
            1, 1, 2, 5, 10, 20, 50
            };

      AL::Pos pos1 = pix2pos(x1);
      AL::Pos pos2 = pix2pos(x2);

      //---------------------------------------------------
      //    draw raster
      //---------------------------------------------------

      int bar1, bar2, beat, tick;
      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      int n = mag[magStep];

      bar1 = (bar1 / n) * n;           // round down
      if (bar1 && n >= 2)
            bar1 -= 1;
      bar2 = ((bar2 + n - 1) / n) * n; // round up

      for (int bar = bar1; bar <= bar2;) {
            AL::Pos stick(_score->tempomap(), _score->sigmap(), bar, 0, 0);
            if (magStep) {
                  double x = double(pos2pix(stick));
                  if (x > 0) {
                        p->setPen(Qt::lightGray);
                        p->drawLine(x, y1, x, y2);
                        }
                  else {
                        p->setPen(Qt::black);
                        p->drawLine(x, y1, x, y1);
                        }
                  }
            else {
                  int z = stick.timesig().nominator;
                  for (int beat = 0; beat < z; beat++) {
                        AL::Pos xx(_score->tempomap(), _score->sigmap(), bar, beat, 0);
                        int xp = pos2pix(xx);
                        if (xp < 0)
                              continue;
                        if (xp > 0) {
                              p->setPen(beat == 0 ? Qt::lightGray : Qt::gray);
                              p->drawLine(xp, y1, xp, y2);
                              }
                        else {
                              p->setPen(Qt::black);
                              p->drawLine(xp, y1, xp, y2);
                              }
                        }
                  }
            if (bar == 0 && n >= 2)
                  bar += (n-1);
            else
                  bar += n;
            }
      }

//---------------------------------------------------------
//   drawForeground
//---------------------------------------------------------

void PianoView::drawForeground(QPainter* painter, const QRectF& rect)
      {
      static const QColor lcColors[3] = { Qt::red, Qt::blue, Qt::blue };

      qreal x1 = rect.x();
      qreal x2 = x1 + rect.width();
      for (int i = 0; i < 3; ++i) {
            if (!_locator[i].valid())
                  continue;
            painter->setPen(lcColors[i]);
            qreal xp = qreal(pos2pix(_locator[i]));
            if (xp >= x1 && xp < x2)
                  painter->drawLine(QLineF(xp, rect.top(), xp, rect.bottom()));
            }
      }

//---------------------------------------------------------
//   PianoView
//---------------------------------------------------------

PianoView::PianoView()
   : QGraphicsView()
      {
      setScene(new PianoScene);
      setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
      setResizeAnchor(QGraphicsView::AnchorUnderMouse);
      setMouseTracking(true);
      setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);
      setDragMode(QGraphicsView::RubberBandDrag);
      _timeType = AL::TICKS;
      magStep   = 0;
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoView::setStaff(Staff* s, AL::Pos* l)
      {
      staff    = s;
      _locator = l;
      pos.setContext(s->score()->tempomap(), s->score()->sigmap());

      scene()->blockSignals(true);
      static_cast<PianoScene*>(scene())->setStaff(s);
      scene()->blockSignals(false);

      Measure* lm = s->score()->lastMeasure();
      ticks       = lm->tick() + lm->tickLen();
      scene()->setSceneRect(0.0, 0.0, double(ticks + 960), keyHeight * 75);

      //
      // move to something interesting
      //
      QList<QGraphicsItem*> items = scene()->selectedItems();
      QRectF boundingRect;
      foreach(QGraphicsItem* item, items)
            boundingRect |= item->boundingRect();
      centerOn(boundingRect.center());
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void PianoView::wheelEvent(QWheelEvent* event)
      {
      int step = event->delta() / 120;
      double xmag = transform().m11();
      double ymag = transform().m22();

      if (event->modifiers() == Qt::ControlModifier) {
            if (step > 0) {
                  for (int i = 0; i < step; ++i) {
                        if (xmag > 10.0)
                              break;
                        scale(1.1, 1.0);
                        xmag *= 1.1;
                        }
                  }
            else {
                  for (int i = 0; i < -step; ++i) {
                        if (xmag < 0.001)
                              break;
                        scale(.9, 1.0);
                        xmag *= .9;
                        }
                  }
            emit magChanged(xmag, ymag);

            int tpix  = (480 * 4) * xmag;
            magStep = 0;
            if (tpix < 64)
                  magStep = 1;
            if (tpix < 32)
                  magStep = 2;
            if (tpix <= 16)
                  magStep = 3;
            if (tpix < 8)
                  magStep = 4;
            if (tpix <= 4)
                  magStep = 5;
            if (tpix <= 2)
                  magStep = 6;


            //
            // if xpos <= 0, then the scene is centered
            // there is no scroll bar anymore sending
            // change signals, so we have to do it here:
            //
            double xpos = -(mapFromScene(QPointF()).x());
            if (xpos <= 0)
                  emit xposChanged(xpos);
            }
      else if (event->modifiers() == Qt::ShiftModifier) {
            QWheelEvent we(event->pos(), event->delta(), event->buttons(), 0, Qt::Horizontal);
            QGraphicsView::wheelEvent(&we);
            }
      else if (event->modifiers() == 0) {
            QGraphicsView::wheelEvent(event);
            }
      else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
            if (step > 0) {
                  for (int i = 0; i < step; ++i) {
                        if (ymag > 3.0)
                              break;
                        scale(1.0, 1.1);
                        ymag *= 1.1;
                        }
                  }
            else {
                  for (int i = 0; i < -step; ++i) {
                        if (ymag < 0.4)
                              break;
                        scale(1.0, .9);
                        ymag *= .9;
                        }
                  }
            emit magChanged(xmag, ymag);
            }
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int PianoView::y2pitch(int y) const
      {
      int pitch;
      const int total = (10 * 7 + 5) * keyHeight;       // 75 Ganztonschritte
      y = total - y;
      int oct = (y / (7 * keyHeight)) * 12;
      static const char kt[] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4, 4, 4, 4,
            5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
            6, 6, 6, 6, 6, 6, 6,
            7, 7, 7, 7, 7, 7,
            8, 8, 8, 8, 8, 8, 8,
            9, 9, 9, 9, 9, 9,
            10, 10, 10, 10, 10, 10, 10,
            11, 11, 11, 11, 11, 11, 11, 11, 11, 11
            };
      pitch = kt[y % 91] + oct;
      if (pitch < 0 || pitch > 127)
            pitch = -1;
      return pitch;
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoView::mouseMoveEvent(QMouseEvent* event)
      {
      QPointF p(mapToScene(event->pos()));
      int pitch = y2pitch(int(p.y()));
      emit pitchChanged(pitch);
      int tick = int(p.x()) -480;
      if (tick < 0) {
            tick = 0;
            pos.setTick(tick);
            pos.setInvalid();
            }
      else
            pos.setTick(tick);
      emit posChanged(pos);
      QGraphicsView::mouseMoveEvent(event);
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoView::leaveEvent(QEvent* event)
      {
      emit pitchChanged(-1);
      pos.setInvalid();
      emit posChanged(pos);
      QGraphicsView::leaveEvent(event);
      }

void PianoView::ensureVisible(int tick)
      {
      tick += MAP_OFFSET;
      QPointF pt = mapToScene(0, height() / 2);
      QGraphicsView::ensureVisible(qreal(tick), pt.y(), 240.0, 1.0);
      }


