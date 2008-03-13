//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: pedal.h,v 1.2 2006/03/02 17:08:40 wschweer Exp $
//
//  Copyright (C) 2002-2008 Werner Schweer and others
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

#ifndef __PEDAL_H__
#define __PEDAL_H__

#include "line.h"

class Pedal;

//---------------------------------------------------------
//   PedalSegment
//---------------------------------------------------------

class PedalSegment : public LineSegment {
   protected:
   public:
      PedalSegment(Score* s) : LineSegment(s) {}
      Pedal* pedal() const                { return (Pedal*)parent(); }
      virtual ElementType type() const    { return PEDAL_SEGMENT; }
      virtual PedalSegment* clone() const { return new PedalSegment(*this); }
      virtual void draw(QPainter&) const;
      virtual QRectF bbox() const;
      };

//---------------------------------------------------------
//   Pedal
//---------------------------------------------------------

class Pedal : public SLine {
   protected:
      int symbol;
      qreal pedalLineWidth;
      qreal pedalTextDistance;

      friend class PedalSegment;

   public:
      Pedal(Score* s);
      virtual Pedal* clone() const     { return new Pedal(*this); }
      virtual ElementType type() const { return PEDAL; }

      virtual void layout(ScoreLayout*);
      virtual void write(Xml&) const;
      virtual void read(QDomElement);
      virtual LineSegment* createLineSegment();
      };

#endif

