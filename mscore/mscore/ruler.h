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

#ifndef __RULER_H__
#define __RULER_H__

#include "al/pos.h"

class Score;

static const int rulerHeight = 28;

//---------------------------------------------------------
//   Ruler
//---------------------------------------------------------

class Ruler : public QWidget {
      Q_OBJECT

      Score* _score;
      AL::Pos _cursor;
      AL::Pos* _locator;

      int magStep;
      double _xmag;
      int _xpos;
      AL::TType _timeType;
      QFont _font1, _font2;

      static QPixmap* markIcon[3];

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*);

      AL::Pos pix2pos(int x) const;
      int pos2pix(const AL::Pos& p) const;

   signals:
      void posChanged(const AL::Pos&);
      void locatorMoved(int);

   public slots:
      void setXpos(int);
      void setMag(double xmag, double ymag);
      void setPos(const AL::Pos&);

   public:
      Ruler(Score*, AL::Pos* locator, QWidget* parent = 0);
      };


#endif
