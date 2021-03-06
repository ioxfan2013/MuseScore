//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id$
//
//  Copyright (C) 2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "palette.h"
#include "musescore.h"
#include "libmscore/element.h"
#include "libmscore/style.h"
#include "globals.h"
#include "libmscore/sym.h"
#include "libmscore/symbol.h"
#include "libmscore/score.h"
#include "libmscore/image.h"
#include "libmscore/xml.h"
#include "scoreview.h"
#include "libmscore/note.h"
#include "libmscore/chord.h"
#include "libmscore/clef.h"
#include "libmscore/segment.h"
#include "preferences.h"
#include "seq.h"
#include "libmscore/part.h"
#include "libmscore/textline.h"
#include "painterqt.h"
#include "libmscore/measure.h"
#include "libmscore/icon.h"
#include "libmscore/mscore.h"

//---------------------------------------------------------
//   needsStaff
//    should a staff been drawn if e is used as icon in
//    a palette
//---------------------------------------------------------

static bool needsStaff(Element* e)
      {
      if (e == 0)
            return false;
      switch(e->type()) {
            case CHORD:
            case BAR_LINE:
            case CLEF:
            case KEYSIG:
            case TIMESIG:
            case REST:
                  return true;
            default:
                  return false;
            }
      }

//---------------------------------------------------------
//   Palette
//---------------------------------------------------------

Palette::Palette(QWidget* parent)
   : QWidget(parent)
      {
      extraMag      = 1.0;
      currentIdx    = -1;
      selectedIdx   = -1;
      _yOffset      = 0.0;
      hgrid         = 50;
      vgrid         = 60;
      _drawGrid     = false;
      _selectable   = false;
      setMouseTracking(true);
      QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
      policy.setHeightForWidth(true);
      setSizePolicy(policy);
      setSizeIncrement(QSize(hgrid, vgrid));
      setBaseSize(QSize(hgrid, vgrid));
      setReadOnly(true);
      }

Palette::~Palette()
      {
      foreach(PaletteCell* cell, cells)
            delete cell;
      }

//---------------------------------------------------------
//   setReadOnly
//---------------------------------------------------------

void Palette::setReadOnly(bool val)
      {
      _readOnly = val;
      setAcceptDrops(!val);
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void Palette::contextMenuEvent(QContextMenuEvent* event)
      {
//      if (_readOnly)
//            return;
      int i = idx(event->pos());
      if (i == -1)
            return;
      QMenu menu;
      QAction* deleteAction = menu.addAction(tr("Delete Contents"));
      QAction* contextAction = menu.addAction(tr("Properties..."));
      if (cells[i] && cells[i]->readOnly)
            deleteAction->setEnabled(false);

      QAction* action = menu.exec(mapToGlobal(event->pos()));

      if (action == deleteAction) {
            PaletteCell* cell = cells[i];
            if (cell)
                  delete cell->element;
            delete cell;
            cells[i] = 0;
            update();
            emit changed();
            }
      else if (action == contextAction) {
            PaletteCell* c = cells[i];
            if (c == 0)
                  return;
            PaletteCell cell(*c);
            PaletteCellProperties props(&cell);
            if (props.exec()) {
                  *cells[i] = cell;
                  update();
                  emit changed();
                  }
            }
      bool sizeChanged = false;
      while (cells.size() > 1) {
            if (cells.back() == 0) {
                  cells.removeLast();
                  sizeChanged = true;
                  }
            else
                  break;
            }
      if (sizeChanged) {
            updateGeometry();
            static_cast<QWidget*>(parent())->updateGeometry();
            }
      }

//---------------------------------------------------------
//   setGrid
//---------------------------------------------------------

void Palette::setGrid(int hh, int vv)
      {
      hgrid = hh;
      vgrid = vv;
      setSizeIncrement(QSize(hgrid, vgrid));
      setBaseSize(QSize(hgrid, vgrid));
      }

//---------------------------------------------------------
//   contentsMousePressEvent
//---------------------------------------------------------

void Palette::mousePressEvent(QMouseEvent* ev)
      {
      dragStartPosition = ev->pos();
      if (_selectable) {
            int i = idx(dragStartPosition);
            if (i == -1)
                  return;
            if (i != selectedIdx) {
                  update(idxRect(i) | idxRect(selectedIdx));
                  selectedIdx = i;
                  }
            emit boxClicked(i);
            }
      }

//---------------------------------------------------------
//   applyDrop
//---------------------------------------------------------

static void applyDrop(Score* score, ScoreView* viewer, Element* target, Element* e, QPointF pt = QPointF())
      {
      if (target->acceptDrop(viewer, pt, e->type(), e->subtype())) {
            Element* ne = e->clone();
            ne->setScore(score);
            DropData dropData;
            dropData.view = viewer;
            dropData.pos  = pt;
            dropData.dragOffset = pt;
            dropData.modifiers = 0;
            dropData.element = e->clone();
            ne = target->drop(dropData);
            if (ne)
                  score->select(ne, SELECT_SINGLE, 0);
            viewer->setDropTarget(0);     // acceptDrop sets dropTarget
            }
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void Palette::mouseDoubleClickEvent(QMouseEvent* ev)
      {
      int i = idx(ev->pos());
      if (i == -1)
            return;
      Score* score   = mscore->currentScore();
      if (score == 0)
            return;
      const Selection& sel = score->selection();

      if (sel.state() == SEL_NONE)
            return;

//      QMimeData* mimeData = new QMimeData;
      Element* element    = cells[i]->element;
      if (element == 0)
            return;
      ScoreView* viewer = mscore->currentScoreView();
//      mimeData->setData(mimeSymbolFormat, element->mimeData(QPointF()));

      score->startCmd();
      if (sel.state() == SEL_LIST) {
            foreach(Element* e, sel.elements())
                  applyDrop(score, viewer, e, element);
            }
      else if (sel.state() == SEL_RANGE) {
            // TODO: check for other element types:
            if (element->type() == BAR_LINE) {
                  // TODO: apply to multiple measures
                  Measure* m = sel.startSegment()->measure();
                  QRectF r = m->staffabbox(sel.staffStart());
                  QPointF pt(r.x() + r.width() * .5, r.y() + r.height() * .5);
                  applyDrop(score, viewer, m, element, pt);
                  }
            else {
                  int track1 = sel.staffStart() * VOICES;
                  int track2 = sel.staffEnd() * VOICES;
                  for (Segment* s = sel.startSegment(); s && s != sel.endSegment(); s = s->next1()) {
                        for (int track = track1; track < track2; ++track) {
                              Element* e = s->element(track);
                              if (e == 0)
                                    continue;
                              if (e->type() == CHORD) {
                                    Chord* chord = static_cast<Chord*>(e);
                                    foreach(Note* n, chord->notes())
                                          applyDrop(score, viewer, n, element);
                                    }
                              else
                                    applyDrop(score, viewer, e, element);
                              }
                        }
                  }
            }
      else
            printf("unknown selection state\n");
      score->endCmd();
      mscore->endCmd();
      }

//---------------------------------------------------------
//   idx
//---------------------------------------------------------

int Palette::idx(const QPoint& p) const
      {
      int x = p.x();
      int y = p.y();

      int row = y / vgrid;
      int col = x / hgrid;

      int nc = columns();
      if (col > nc)
            return -1;

      int idx = row * nc + col;
      if (idx >= cells.size())
            return -1;
      return idx;
      }

//---------------------------------------------------------
//   idxRect
//---------------------------------------------------------

QRect Palette::idxRect(int i)
      {
      if (i == -1)
            return QRect();
      if (columns() == 0)
            return QRect();
      int cc = i % columns();
      int cr = i / columns();
      return QRect(cc * hgrid, cr * vgrid, hgrid, vgrid);
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void Palette::mouseMoveEvent(QMouseEvent* ev)
      {
      if ((currentIdx != -1) && (ev->buttons() & Qt::LeftButton)
         && (ev->pos() - dragStartPosition).manhattanLength() > QApplication::startDragDistance()) {
            if (cells[currentIdx]) {
                  QDrag* drag = new QDrag(this);
                  QMimeData* mimeData = new QMimeData;
                  Element* el  = cells[currentIdx]->element;
                  // QRect ir     = idxRect(currentIdx);
                  qreal mag    = PALETTE_SPATIUM * extraMag / gscore->spatium();
                  QPointF spos = QPointF(dragStartPosition) / mag;
                  spos        -= QPointF(cells[currentIdx]->x, cells[currentIdx]->y);

                  // DEBUG:
                  spos.setX(0.0);
                  mimeData->setData(mimeSymbolFormat, el->mimeData(spos));
                  drag->setMimeData(mimeData);

                  dragSrcIdx = currentIdx;
                  emit startDragElement(el);
                  if (_readOnly) {
                        drag->start(Qt::CopyAction);
                        }
                  else {
                        /*Qt::DropAction action = */drag->start(Qt::CopyAction | Qt::MoveAction);
                        }
                  }
            }
      else {
            QRect r;
            if (currentIdx != -1)
                  r = idxRect(currentIdx);
            currentIdx = idx(ev->pos());
            if (currentIdx != -1) {
                  if (cells[currentIdx] == 0)
                        currentIdx = -1;
                  else
                        r |= idxRect(currentIdx);
                  }
            update(r);
            }
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void Palette::leaveEvent(QEvent*)
      {
      if (currentIdx != -1) {
            QRect r = idxRect(currentIdx);
            currentIdx = -1;
            update(r);
            }
      }

//---------------------------------------------------------
//   append
//    append element to palette
//---------------------------------------------------------

void Palette::append(Element* s, const QString& name, QString tag, qreal mag)
      {
      PaletteCell* cell = new PaletteCell;

      cells.append(cell);
      cell->element   = s;
      cell->name      = name;
      cell->tag       = tag;
      cell->drawStaff = needsStaff(s);
      cell->xoffset   = 0;
      cell->yoffset   = 0;
      cell->mag       = mag;
      cell->readOnly  = false;
      update();
      if (s && s->type() == ICON) {
            Icon* icon = static_cast<Icon*>(s);
            connect(getAction(icon->action()), SIGNAL(toggled(bool)), SLOT(actionToggled(bool)));
            }
      }

void Palette::append(int symIdx)
      {
      if (!symbols[0][symIdx].isValid())
            return;
      Symbol* s = new Symbol(gscore);
      s->setSym(symIdx);
      append(s, qApp->translate("symbol", ::symbols[0][symIdx].name()));
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Palette::add(int idx, Element* s, const QString& name, QString tag)
      {
      PaletteCell* cell = new PaletteCell;

      if (idx < cells.size()) {
            delete cells[idx];
            }
      else {
            for (int i = cells.size(); i <= idx; ++i)
                  cells.append(0);
            }
      cells[idx]      = cell;
      cell->element   = s;
      cell->name      = name;
      cell->tag       = tag;
      cell->drawStaff = needsStaff(s);
      cell->xoffset   = 0;
      cell->yoffset   = 0;
      cell->mag       = 1.0;
      cell->readOnly  = false;
      update();
      if (s && s->type() == ICON) {
            Icon* icon = static_cast<Icon*>(s);
            connect(getAction(icon->action()), SIGNAL(toggled(bool)), SLOT(actionToggled(bool)));
            }
      }

//---------------------------------------------------------
//   paintPaletteElement
//---------------------------------------------------------

static void paintPaletteElement(void* data, Element* e)
      {
      QPainter* p = static_cast<QPainter*>(data);
      p->save();
      p->translate(e->pos());
      PainterQt painter(p, 0);
      e->draw(&painter);
      p->restore();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Palette::paintEvent(QPaintEvent* event)
      {
      qreal _spatium = gscore->spatium();
      qreal mag = PALETTE_SPATIUM * extraMag / _spatium;
      gscore->setSpatium(SPATIUM20  * DPI);

      QPainter p(this);
      p.setRenderHint(QPainter::Antialiasing, true);

      //
      // draw grid
      //

      int c = columns();
      if (_drawGrid) {
            p.setPen(Qt::gray);
            for (int row = 1; row < rows(); ++row)
                  p.drawLine(0, row*vgrid, c * hgrid, row*vgrid);
            for (int column = 1; column < c; ++column)
                  p.drawLine(hgrid*column, 0, hgrid*column, rows()*vgrid);
            }

      qreal dy = lrint(2 * PALETTE_SPATIUM * extraMag);

      //
      // draw symbols
      //

      QPen pen(palette().color(QPalette::Normal, QPalette::Text));
      pen.setWidthF(MScore::defaultStyle()->valueS(ST_staffLineWidth).val() * PALETTE_SPATIUM * extraMag);

      p.fillRect(event->rect(), p.background().color());
      for (int idx = 0; idx < cells.size(); ++idx) {
            QRect r = idxRect(idx);
            p.setPen(pen);
            if (idx == selectedIdx)
                  p.fillRect(r, palette().color(QPalette::Normal, QPalette::Highlight));
            else if (idx == currentIdx)
                  p.fillRect(r, p.background().color().light(118));
            if (cells.isEmpty() || cells[idx] == 0)
                  continue;

            if (!cells[idx]->tag.isEmpty()) {
                  p.setPen(Qt::darkGray);
                  QFont f(p.font());
                  f.setPointSize(12);
                  p.setFont(f);
                  p.drawText(r, Qt::AlignLeft | Qt::AlignTop, cells[idx]->tag);
                  }

            p.setPen(pen);

            Element* el = cells[idx]->element;
            if (el == 0)
                  continue;
            bool drawStaff = cells[idx]->drawStaff;
            if (el->type() != ICON) {
                  int row    = idx / c;
                  int column = idx % c;

                  el->layout();
                  el->setPos(0.0, 0.0);   // HACK

                  if (drawStaff) {
                        qreal y = r.y() + vgrid * .5 - dy + _yOffset;
                        qreal x = r.x() + 3;
                        qreal w = hgrid - 6;
                        for (int i = 0; i < 5; ++i) {
                              qreal yy = y + PALETTE_SPATIUM * i * extraMag;
                              p.drawLine(QLineF(x, yy, x + w, yy));
                              }
                        }
                  p.save();
                  qreal cellMag = cells[idx]->mag * mag;
                  p.scale(cellMag, cellMag);

                  double gw = hgrid / cellMag;
                  double gh = vgrid / cellMag;
                  double gx = column * gw + cells[idx]->xoffset / cellMag;
                  double gy = row    * gh + cells[idx]->yoffset / cellMag;

                  double sw = el->width();
                  double sh = el->height();
                  double sy;

                  if (drawStaff)
                        sy = gy + gh * .5 - 2.0 * gscore->spatium();
                  else
                        sy  = gy + (gh - sh) * .5 - el->bbox().y();
                  double sx  = gx + (gw - sw) * .5 - el->bbox().x();

                  sy += _yOffset / cellMag;

                  p.translate(QPointF(sx, sy));
                  // el->setPos(sx, sy);
                  cells[idx]->x = sx;
                  cells[idx]->y = sy;

                  QColor color;
                  if (idx != selectedIdx) {
                        // show voice colors for notes
                        if (el->type() == CHORD) {
                              el->setSelected(true);
                              color = el->curColor();
                              }
                        else
                              color = palette().color(QPalette::Normal, QPalette::Text);
                        }
                  else
                        color = palette().color(QPalette::Normal, QPalette::HighlightedText);

                  p.setPen(QPen(color));
                  el->scanElements(&p, paintPaletteElement);
                  p.restore();
                  }
            else {
                  int x      = r.x();
                  int y      = r.y();
                  Icon* _icon = static_cast<Icon*>(el);
                  QIcon icon = _icon->icon();
                  static const int border = 2;
                  int size   = (hgrid < vgrid ? hgrid : vgrid) - 2 * border;
                  QPixmap pm(icon.pixmap(size, QIcon::Normal, QIcon::On));
                  p.drawPixmap(x + (hgrid - size) / 2, y + (vgrid - size) / 2, pm);
                  }
            }
      }

//---------------------------------------------------------
//   event
//---------------------------------------------------------

bool Palette::event(QEvent* ev)
      {
      if (ev->type() == QEvent::ToolTip) {
            QHelpEvent* he = (QHelpEvent*)ev;
            int x = he->pos().x();
            int y = he->pos().y();

            int row = y / vgrid;
            int col = x / hgrid;

            if (row < 0 || row >= rows())
                  return false;
            if (col < 0 || col >= columns())
                  return false;
            int idx = row * columns() + col;
            if (idx >= cells.size())
                  return false;
            if (cells[idx] == 0)
                  return false;
            QToolTip::showText(he->globalPos(), cells[idx]->name, this);
            return false;
            }
      return QWidget::event(ev);
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void Palette::dragEnterEvent(QDragEnterEvent* event)
      {
      const QMimeData* data = event->mimeData();
      if (data->hasUrls()) {
            QList<QUrl>ul = event->mimeData()->urls();
            QUrl u = ul.front();
            if (debugMode) {
                  printf("drag Url: %s\n", u.toString().toLatin1().data());
                  printf("scheme <%s> path <%s>\n", u.scheme().toLatin1().data(),
                     u.path().toLatin1().data());
                  }
            if (u.scheme() == "file") {
                  QFileInfo fi(u.path());
                  QString suffix(fi.suffix().toLower());
                  if (suffix == "svg"
                     || suffix == "jpg"
                     || suffix == "png"
                     || suffix == "xpm"
                     ) {
                        event->acceptProposedAction();
                        }
                  }
            }
      else if (data->hasFormat(mimeSymbolFormat))
            event->acceptProposedAction();
      else {
            if (debugMode) {
                  printf("dragEnterEvent: formats:\n");
                  foreach(const QString& s, event->mimeData()->formats())
                        printf("   %s\n", s.toLatin1().data());
                  }
            }
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void Palette::dragMoveEvent(QDragMoveEvent* ev)
      {
      int i = idx(ev->pos());
      if (i == -1)
            return;

      int n = cells.size();
      int ii = i;
      for (; ii < n; ++ii) {
            if (cells[ii] == 0)
                  break;
            }
      if (ii == n)
            return;

      QRect r;
      if (currentIdx != -1)
            r = idxRect(currentIdx);
      update(r | idxRect(ii));
      currentIdx = ii;
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void Palette::dropEvent(QDropEvent* event)
      {
      Element* e = 0;
      QString name;

      const QMimeData* data = event->mimeData();
      if (data->hasUrls()) {
            QList<QUrl>ul = event->mimeData()->urls();
            QUrl u = ul.front();
            if (u.scheme() == "file") {
                  QFileInfo fi(u.path());
                  Image* s = 0;
                  QString suffix(fi.suffix().toLower());
#ifdef SVG_IMAGES
                  if (suffix == "svg")
                        s = new SvgImage(gscore);
                  else
#endif
                        if (suffix == "jpg"
                     || suffix == "png"
                     || suffix == "xpm"
                        )
                        s = new RasterImage(gscore);
                  else
                        return;
                  qreal mag = PALETTE_SPATIUM * extraMag / gscore->spatium();
                  s->setPath(u.toLocalFile());
                  s->setSize(QSizeF(hgrid / mag, hgrid / mag));
                  e = s;
                  name = s->path();
                  }
            }
      else if (data->hasFormat(mimeSymbolFormat)) {
            QByteArray data(event->mimeData()->data(mimeSymbolFormat));
            QDomDocument doc;
            int line, column;
            QString err;
            if (!doc.setContent(data, &err, &line, &column)) {
                  printf("error reading drag data\n");
                  return;
                  }
            docName = "--";
            QDomElement el = doc.documentElement();
            QPointF dragOffset;
            ElementType type = Element::readType(el, &dragOffset);

            if (type == IMAGE) {
                  // look ahead for image type
                  QString path;
                  for (QDomElement ee = el.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        QString tag(ee.tagName());
                        if (tag == "path") {
                              path = ee.text();
                              break;
                              }
                        }
                  Image* image = 0;
                  QString s(path.toLower());
#ifdef SVG_IMAGES
                  if (s.endsWith(".svg"))
                        image = new SvgImage(gscore);
                  else
#endif
                        if (s.endsWith(".jpg")
                     || s.endsWith(".png")
                     || s.endsWith(".xpm")
                        )
                        image = new RasterImage(gscore);
                  else {
                        printf("unknown image format <%s>\n", path.toLatin1().data());
                        }
                  if (image) {
                        image->read(el);
                        e = image;
                        }
                  }
            else if (type == SYMBOL) {
                  Symbol* s = new Symbol(gscore);
                  s->read(el);
                  e = s;
                  }
            else {
                  e = Element::create(type, gscore);
                  if (e)
                        e->read(el);
                  if (e->type() == TEXTLINE) {
                        TextLine* tl = static_cast<TextLine*>(e);
                        tl->setLen(gscore->spatium() * 7);
                        tl->setTrack(0);
                        }
                  }
            }
      if (e == 0)
            return;
      e->setSelected(false);
      bool ok = false;
      if (event->source() == this) {
            int i = idx(event->pos());
            if (i == -1) {
                  cells.append(cells[dragSrcIdx]);
                  cells[dragSrcIdx] = 0;
                  ok = true;
                  }
            else if (dragSrcIdx != i) {
                  PaletteCell* c = cells[dragSrcIdx];
                  cells[dragSrcIdx] = cells[i];
                  cells[i] = c;
                  delete e;
                  ok = true;
                  }
            event->setDropAction(Qt::MoveAction);
            }
      else {
            append(e, name);
            ok = true;
            }
      if (ok) {
            event->acceptProposedAction();
            updateGeometry();
//            static_cast<QWidget*>(parent())->updateGeometry();
            update();
            emit changed();
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Palette::write(Xml& xml, const QString& name) const
      {
      xml.stag(QString("Palette name=\"%1\"").arg(Xml::xmlString(name)));
      xml.tag("gridWidth", hgrid);
      xml.tag("gridHeight", vgrid);
      if (extraMag != 1.0)
            xml.tag("mag", extraMag);
      if (_drawGrid)
            xml.tag("grid", _drawGrid);
      if (_yOffset != 0.0)
            xml.tag("yoffset", _yOffset);

      int n = cells.size();
      for (int i = 0; i < n; ++i) {
            if (cells[i] && cells[i]->readOnly)
                  continue;
            if (cells[i] == 0 || cells[i]->element == 0) {
                  xml.tagE("Cell");
                  continue;
                  }
            if (!cells[i]->name.isEmpty())
                  xml.stag(QString("Cell name=\"%1\"").arg(Xml::xmlString(cells[i]->name)));
            else
                  xml.stag("Cell");
            if (cells[i]->drawStaff)
                  xml.tag("staff", cells[i]->drawStaff);
            if (cells[i]->xoffset)
                  xml.tag("xoffset", cells[i]->xoffset);
            if (cells[i]->yoffset)
                  xml.tag("yoffset", cells[i]->yoffset);
            if (!cells[i]->tag.isEmpty())
                  xml.tag("tag", cells[i]->tag);
            if (cells[i]->mag != 1.0)
                  xml.tag("mag", cells[i]->mag);
            cells[i]->element->write(xml);
            xml.etag();
            }
      xml.etag();
      }

//---------------------------------------------------------
//   read
//    return false on error
//---------------------------------------------------------

bool Palette::read(QFile* qf)
      {
      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(qf, false, &err, &line, &column)) {
            QString error;
            error.sprintf("error reading palette  %s at line %d column %d: %s\n",
               qPrintable(qf->fileName()), line, column, qPrintable(err));
            QMessageBox::warning(0,
               QWidget::tr("MuseScore: Load Palette failed:"),
               error,
               QString::null, QWidget::tr("Quit"), QString::null, 0, 1);
            return false;
            }
      docName = qf->fileName();
      for (QDomElement e = doc.documentElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "museScore") {
                  QString version = e.attribute(QString("version"));
                  QStringList sl = version.split('.');
                  int versionId = sl[0].toInt() * 100 + sl[1].toInt();
                  gscore->setMscVersion(versionId);

                  for (QDomElement ee = e.firstChildElement(); !ee.isNull();  ee = ee.nextSiblingElement()) {
                        QString tag(ee.tagName());
                        if (tag == "Palette") {
                              QString name = ee.attribute("name", "");
                              setName(name);
                              read(ee);
                              }
                        else
                              domError(ee);
                        }
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Palette::write(const QString& path)
      {
      QFile f(path);
      if (!f.open(QIODevice::WriteOnly)) {
            printf("cannot write modified palettes\n");
            return;
            }
      Xml xml(&f);
      xml.header();
      xml.stag("museScore version=\"" MSC_VERSION "\"");
      write(xml, name());
      xml.etag();
      f.close();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Palette::read(QDomElement e)
      {
      QString name = e.attribute("name", "");
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            QString text(e.text());
            if (tag == "gridWidth")
                  hgrid = text.toDouble();
            else if (tag == "gridHeight")
                  vgrid = text.toDouble();
            else if (tag == "mag")
                  extraMag = text.toDouble();
            else if (tag == "grid")
                  _drawGrid = text.toInt();
            else if (tag == "yoffset")
                  _yOffset = text.toDouble();
            else if (tag == "drumPalette")      // obsolete
                  ;
            else if (tag == "Cell") {
                  if (e.firstChildElement().isNull())
                        append(0, "");
                  else {
                        QString name = e.attribute("name", "");
                        bool drawStaff = false;
                        int xoffset = 0;
                        int yoffset = 0;
                        qreal mag   = 1.0;
                        for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                              QString tag(ee.tagName());
                              if (tag == "staff")
                                    drawStaff = ee.text().toInt();
                              else if (tag == "xoffset")
                                    xoffset = ee.text().toInt();
                              else if (tag == "yoffset")
                                    yoffset = ee.text().toInt();
                              else if (tag == "mag")
                                    mag = ee.text().toDouble();
                              else if (tag == "tag")
                                    tag = ee.text();
                              else if (tag == "Image") {
                                    // look ahead for image type
                                    QString path;
                                    for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                          QString tag(eee.tagName());
                                          if (tag == "path") {
                                                path = eee.text();
                                                break;
                                                }
                                          }
                                    Image* image = 0;
                                    QString s(path.toLower());
#ifdef SVG_IMAGES
                                    if (s.endsWith(".svg"))
                                          image = new SvgImage(gscore);
                                    else
#endif
                                          if (s.endsWith(".jpg")
                                       || s.endsWith(".png")
                                       || s.endsWith(".xpm")
                                          )
                                          image = new RasterImage(gscore);
                                    else {
                                          printf("unknown image format <%s>\n", path.toLatin1().data());
                                          }
                                    if (image) {
                                          image->read(ee);
                                          append(image, name);
                                          }
                                    }
                              else {
                                    Element* element = Element::name2Element(tag, gscore);
                                    if (element == 0) {
                                          domError(ee);
                                          return;
                                          }
                                    else {
                                          element->read(ee);
                                          if (element->type() == ICON) {
                                                Icon* icon = static_cast<Icon*>(element);
                                                // Shortcut* s = getShortcut(icon->action());
                                                QIcon qicon(getAction(icon->action())->icon());
                                                icon->setAction(icon->action(), qicon);
                                                }
                                          append(element, name);
                                          }
                                    }
                              }
                        cells.back()->drawStaff = drawStaff;
                        cells.back()->xoffset   = xoffset;
                        cells.back()->yoffset   = yoffset;
                        cells.back()->mag       = mag;
                        }
                  }
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Palette::clear()
      {
      foreach(PaletteCell* cell, cells)
            delete cell;
      cells.clear();
      }

//---------------------------------------------------------
//   rows
//---------------------------------------------------------

int Palette::rows() const
      {
      int c = columns();
      if (c == 0)
            return 0;
      return (cells.size() + c - 1) / c;
      }

//---------------------------------------------------------
//   heightForWidth
//---------------------------------------------------------

int Palette::heightForWidth(int w) const
      {
      int c = w / hgrid;
      if (c <= 0)
            c = 1;
      int r = (cells.size() + c - 1) / c;
      if (r <= 0)
            r = 1;
      return r * vgrid;
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize Palette::sizeHint() const
      {
      int w = width();
      int h = heightForWidth(w);
      return QSize(hgrid, h);
      }

//---------------------------------------------------------
//   actionToggled
//---------------------------------------------------------

void Palette::actionToggled(bool /*val*/)
      {
      selectedIdx = -1;
      int nn = cells.size();
      for (int n = 0; n < nn; ++n) {
            Element* e = cells[n]->element;
            if (e && e->type() == ICON) {
                  QAction* a = getAction(static_cast<Icon*>(e)->action());
                  if (a->isChecked()) {
                        selectedIdx = n;
                        break;
                        }
                  }
            }
      update();
      }

//---------------------------------------------------------
//   PaletteBoxButton
//---------------------------------------------------------

PaletteBoxButton::PaletteBoxButton(PaletteScrollArea* sa, Palette* p, QWidget* parent)
   : QToolButton(parent)
      {
      palette = p;
      scrollArea = sa;
      setCheckable(true);
      setFocusPolicy(Qt::NoFocus);
      connect(this, SIGNAL(clicked(bool)), this, SLOT(showPalette(bool)));
//      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      QMenu* menu = new QMenu;
      connect(menu, SIGNAL(aboutToShow()), SLOT(beforePulldown()));
      setArrowType(Qt::RightArrow);
      setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

      QAction* action;

      action = menu->addAction(tr("Palette Properties"));
      connect(action, SIGNAL(triggered()), SLOT(propertiesTriggered()));

      action = menu->addAction(tr("Insert new Palette"));
      connect(action, SIGNAL(triggered()), SLOT(newTriggered()));

      action = menu->addAction(tr("Move Palette Up"));
      connect(action, SIGNAL(triggered()), SLOT(upTriggered()));

      action = menu->addAction(tr("Move Palette Down"));
      connect(action, SIGNAL(triggered()), SLOT(downTriggered()));

      editAction = menu->addAction(tr("Enable Editing"));
      editAction->setCheckable(true);
      connect(editAction, SIGNAL(triggered(bool)), SLOT(enableEditing(bool)));

      menu->addSeparator();
      action = menu->addAction(tr("Delete Palette"));
      connect(action, SIGNAL(triggered()), SLOT(deleteTriggered()));
      setMenu(menu);
      }

//---------------------------------------------------------
//   beforePulldown
//---------------------------------------------------------

void PaletteBoxButton::beforePulldown()
      {
      editAction->setChecked(!palette->readOnly());
      }

//---------------------------------------------------------
//   enableEditing
//---------------------------------------------------------

void PaletteBoxButton::enableEditing(bool val)
      {
      palette->setReadOnly(!val);
      }

//---------------------------------------------------------
//   changeEvent
//---------------------------------------------------------

void PaletteBoxButton::changeEvent(QEvent* ev)
      {
      if (ev->type() == QEvent::FontChange)
            setFixedHeight(QFontMetrics(font()).height() + 2);
      }

//---------------------------------------------------------
//   showPalette
//---------------------------------------------------------

void PaletteBoxButton::showPalette(bool visible)
      {
      if (visible && preferences.singlePalette) {
            // close all palettes
            emit closeAll();
            }
      scrollArea->setVisible(visible);
      // this->setChecked(visible);
      setChecked(visible);
//      palette->updateGeometry();
      setArrowType(visible ? Qt::DownArrow : Qt::RightArrow );
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void PaletteBoxButton::paintEvent(QPaintEvent*)
      {
      //remove automatic menu arrow
      QStylePainter p(this);
      QStyleOptionToolButton opt;
      initStyleOption(&opt);
      opt.features &= (~QStyleOptionToolButton::HasMenu);
      p.drawComplexControl(QStyle::CC_ToolButton, opt);
      }

//---------------------------------------------------------
//   PaletteScrollArea
//---------------------------------------------------------

PaletteScrollArea::PaletteScrollArea(QWidget* w, QWidget* parent)
   : QScrollArea(parent)
      {
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
      setWidget(w);
      setWidgetResizable(true);
      _restrictHeight = true;
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      //setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void PaletteScrollArea::resizeEvent(QResizeEvent* re)
      {
      Palette* palette = static_cast<Palette*>(widget());
      int h = palette->heightForWidth(width());
      if (_restrictHeight)
            // setMaximumHeight(h+8);
            setMaximumHeight(h+6);

      QScrollArea::resizeEvent(re);
      }

//---------------------------------------------------------
//   PaletteBox
//---------------------------------------------------------

PaletteBox::PaletteBox(QWidget* parent)
   : QDockWidget(tr("Palettes"), parent)
      {
      setObjectName("palette-box");
      setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
      QWidget* mainWidget = new QWidget;
      mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); //??
      vbox = new QVBoxLayout;
      vbox->setSizeConstraint(QLayout::SetNoConstraint);
      vbox->setMargin(0);
      vbox->setSpacing(1);    // 2
      vbox->addStretch();
      mainWidget->setLayout(vbox);
      setWidget(mainWidget);
      connect(mainWidget, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(contextMenu(const QPoint&)));
      mainWidget->setContextMenuPolicy(Qt::CustomContextMenu);
      _dirty = false;
      }

//---------------------------------------------------------
//   contextMenu
//---------------------------------------------------------

void PaletteBox::contextMenu(const QPoint& pt)
      {
      QMenu menu(this);
      menu.setSeparatorsCollapsible(false);
      QAction* titel = menu.addSeparator();
      titel->setText(tr("Palette Operations"));

      QAction* b = menu.addAction(tr("Single Palette Mode"));
      b->setCheckable(true);
      b->setChecked(preferences.singlePalette);

      QAction* a = menu.addAction(tr("Reset to factory defaults"));
// obsoleted by profile:
//      QString s(dataPath + "/" + "mscore-palette.xml");
//      QFile f(s);
//      if (!f.exists() && !_dirty)
//            a->setEnabled(false);

      QAction* ra = menu.exec(mapToGlobal(pt));
      if (a == ra) {
//            if (f.exists())
//                  QFile::remove(s);
            clear();
            mscore->populatePalette();      // hack
            _dirty = true;                  // save profile
            }
      else if (b == ra) {
            preferences.singlePalette = b->isChecked();
            preferences.dirty = true;
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void PaletteBox::clear()
      {
      int n = vbox->count() - 1;    // do not delete last spacer item
      while (n--) {
            QLayoutItem* item = vbox->takeAt(0);
            if (item->widget())
                  item->widget()->hide();
            delete item;
            }
      vbox->invalidate();
      }

//---------------------------------------------------------
//   addPalette
//---------------------------------------------------------

void PaletteBox::addPalette(Palette* w)
      {
      PaletteScrollArea* sa = new PaletteScrollArea(w);
      PaletteBoxButton* b   = new PaletteBoxButton(sa, w);

      sa->setVisible(false);
      b->setText(w->name());
      int slotIdx = vbox->count() - 1;
      vbox->insertWidget(slotIdx, b);
      vbox->insertWidget(slotIdx+1, sa, 1000);
      b->setId(slotIdx);
      connect(b, SIGNAL(paletteCmd(int,int)), SLOT(paletteCmd(int,int)));
      connect(b, SIGNAL(closeAll()), SLOT(closeAll()));
      connect(w, SIGNAL(changed()), SLOT(setDirty()));
      }

//---------------------------------------------------------
//   paletteCmd
//---------------------------------------------------------

void PaletteBox::paletteCmd(int cmd, int slot)
      {
      QLayoutItem* item = vbox->itemAt(slot);
      PaletteBoxButton* b = static_cast<PaletteBoxButton*>(item->widget());

      switch(cmd) {
            case PALETTE_DELETE:
                  {
                  vbox->removeItem(item);
                  b->deleteLater();      // this is the button widget
                  delete item;
                  item = vbox->itemAt(slot);
                  vbox->removeItem(item);
                  delete item->widget();
                  delete item;
                  for (int i = 0; i < (vbox->count() - 1) / 2; ++i)
                        static_cast<PaletteBoxButton*>(vbox->itemAt(i * 2)->widget())->setId(i*2);
                  }
                  break;
            case PALETTE_NEW:
                  {
                  Palette* p = new Palette;
                  p->setReadOnly(false);
                  PaletteScrollArea* sa = new PaletteScrollArea(p);
                  PaletteBoxButton* b   = new PaletteBoxButton(sa, p);
                  sa->setVisible(false);
                  p->setName("new Palette");
                  b->setText(p->name());
                  vbox->insertWidget(slot, b);
                  vbox->insertWidget(slot+1, sa, 1000);
                  connect(b, SIGNAL(paletteCmd(int,int)), SLOT(paletteCmd(int,int)));
                  connect(p, SIGNAL(changed()), SLOT(setDirty()));
                  for (int i = 0; i < (vbox->count() - 1) / 2; ++i)
                        static_cast<PaletteBoxButton*>(vbox->itemAt(i * 2)->widget())->setId(i*2);
                  }
                  // fall through

            case PALETTE_EDIT:
                  {
                  PaletteScrollArea* sa = static_cast<PaletteScrollArea*>(vbox->itemAt(slot+1)->widget());
                  Palette* palette = static_cast<Palette*>(sa->widget());
                  QLayoutItem* item = vbox->itemAt(slot);
                  b = static_cast<PaletteBoxButton*>(item->widget());

                  PaletteProperties pp(palette, 0);
                  int rv = pp.exec();
                  if (rv == 1) {
                        _dirty = true;
                        b->setText(palette->name());
                        palette->update();
                        }
                  }
                  break;
            case PALETTE_UP:
                  if (slot) {
                        QLayoutItem* i1 = vbox->itemAt(slot);
                        QLayoutItem* i2 = vbox->itemAt(slot+1);
                        vbox->removeItem(i1);
                        vbox->removeItem(i2);
                        vbox->insertWidget(slot-2, i2->widget());
                        vbox->insertWidget(slot-2, i1->widget());
                        delete i1;
                        delete i2;
                        for (int i = 0; i < (vbox->count() - 1) / 2; ++i)
                              static_cast<PaletteBoxButton*>(vbox->itemAt(i * 2)->widget())->setId(i*2);
                        }
                  break;

            case PALETTE_DOWN:
                  if (slot < (vbox->count() - 3)) {
                        QLayoutItem* i1 = vbox->itemAt(slot);
                        QLayoutItem* i2 = vbox->itemAt(slot+1);
                        vbox->removeItem(i1);
                        vbox->removeItem(i2);
                        vbox->insertWidget(slot+2, i2->widget());
                        vbox->insertWidget(slot+2, i1->widget());
                        delete i1;
                        delete i2;
                        for (int i = 0; i < (vbox->count() - 1) / 2; ++i)
                              static_cast<PaletteBoxButton*>(vbox->itemAt(i * 2)->widget())->setId(i*2);
                        }
                  break;

            }

      _dirty = true;
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void PaletteBox::closeEvent(QCloseEvent* ev)
      {
      emit paletteVisible(false);
      QWidget::closeEvent(ev);
      }

//---------------------------------------------------------
//   closeAll
//---------------------------------------------------------

void PaletteBox::closeAll()
      {
      for (int i = 0; i < (vbox->count() - 1); i += 2) {
            PaletteBoxButton* b = static_cast<PaletteBoxButton*> (vbox->itemAt(i)->widget() );
            b->showPalette(false);
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void PaletteBox::write(Xml& xml)
      {
      xml.stag("PaletteBox");
      for (int i = 0; i < (vbox->count() - 1); i += 2) {
            PaletteBoxButton* b = static_cast<PaletteBoxButton*>(vbox->itemAt(i)->widget());
            PaletteScrollArea* sa = static_cast<PaletteScrollArea*>(vbox->itemAt(i + 1)->widget());
            Palette* p = static_cast<Palette*>(sa->widget());
            p->write(xml, b->text());
            }
      xml.etag();
      }

//---------------------------------------------------------
//   read
//    return false on error
//---------------------------------------------------------

bool PaletteBox::read(QDomElement e)
      {
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            if (tag == "Palette") {
                  Palette* p = new Palette();
                  QString name = e.attribute("name", "");
                  p->setName(name);
                  p->read(e);
                  addPalette(p);
                  }
            else
                  domError(e);
            }
      return true;
      }

//---------------------------------------------------------
//   PaletteProperties
//---------------------------------------------------------

PaletteProperties::PaletteProperties(Palette* p, QWidget* parent)
   : QDialog(parent)
      {
      palette = p;
      setupUi(this);
      name->setText(palette->name());
      cellWidth->setValue(palette->gridWidth());
      cellHeight->setValue(palette->gridHeight());
      showGrid->setChecked(palette->drawGrid());
      elementOffset->setValue(palette->yOffset());
      mag->setValue(palette->mag());
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PaletteProperties::accept()
      {
      palette->setName(name->text());
      palette->setGrid(cellWidth->value(), cellHeight->value());
      palette->setDrawGrid(showGrid->isChecked());
      palette->setYOffset(elementOffset->value());
      palette->setMag(mag->value());
      QDialog::accept();
      }

//---------------------------------------------------------
//   PaletteCellProperties
//---------------------------------------------------------

PaletteCellProperties::PaletteCellProperties(PaletteCell* p, QWidget* parent)
   : QDialog(parent)
      {
      cell = p;
      setupUi(this);
      xoffset->setValue(cell->xoffset);
      yoffset->setValue(cell->yoffset);
      scale->setValue(cell->mag);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PaletteCellProperties::accept()
      {
      cell->xoffset = xoffset->value();
      cell->yoffset = yoffset->value();
      cell->mag     = scale->value();
      QDialog::accept();
      }
