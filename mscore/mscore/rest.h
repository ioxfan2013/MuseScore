//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: rest.h,v 1.3 2006/03/02 17:08:41 wschweer Exp $
//
//  Copyright (C) 2002-2006 Werner Schweer (ws@seh.de)
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

#ifndef __REST_H__
#define __REST_H__

#include "chordrest.h"

//---------------------------------------------------------
//   Rest
//---------------------------------------------------------

class Rest : public ChordRest {
      int _sym;
      int _move;              // -1, 0, +1

      virtual bool startDrag(const QPointF&);
      virtual QRectF drag(const QPointF& s);
      virtual void endDrag();
      virtual qreal upPos()   const;
      virtual qreal downPos() const;
      virtual qreal centerX() const;

   public:
      Rest(Score*);
      Rest(Score*, int tick, int len);
      virtual ElementType type() const { return REST; }

      virtual void setTickLen(int l);
      virtual void dump() const;
      virtual void draw1(Painter&) const;
      virtual void write(Xml& xml) const;
      virtual void read(QDomNode);
      virtual void add(Element*);
      virtual void remove(Element*);

      virtual int move() const      { return _move; }
      void setMove(int val)         { _move = val; }

      void setSym(int);
      virtual void space(double& min, double& extra) const;
      virtual bool acceptDrop(int, int) const;
      virtual void drop(const QPointF&, int, int);
      virtual Element* findSelectableElement(QPointF p) const;
      virtual void layout();
      virtual void bboxUpdate();
      };

#endif

