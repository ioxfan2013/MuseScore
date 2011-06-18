//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2010 Werner Schweer and others
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

#ifndef __BEND_H__
#define __BEND_H__

#include "element.h"
#include "pitchvalue.h"

class Painter;

//---------------------------------------------------------
//   Bend
//---------------------------------------------------------

class Bend : public Element {
      QList<PitchValue> _points;
      double _lw;
      QPointF notePos;
      double noteWidth;

   public:
      Bend(Score* s);
      virtual Bend* clone() const { return new Bend(*this); }
      virtual ElementType type() const { return BEND; }
      virtual void layout();
      virtual void draw(Painter*) const;
      virtual void write(Xml&) const;
      virtual void read(QDomElement e);
      QList<PitchValue>& points()                { return _points; }
      const QList<PitchValue>& points() const    { return _points; }
      void setPoints(const QList<PitchValue>& p) { _points = p;    }
      };

#endif
