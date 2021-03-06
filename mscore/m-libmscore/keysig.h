//=============================================================================
//  MuseScore
//  Music Score Editor/Player
//  $Id:$
//
//  Copyright (C) 2002-2011 Werner Schweer
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

#ifndef __KEYSIG_H__
#define __KEYSIG_H__

#include "key.h"
#include "element.h"

class Sym;
class Segment;
class Painter;

//---------------------------------------------------------
//   KeySig
///   The KeySig class represents a Key Signature on a staff
//---------------------------------------------------------

struct KeySym {
      int sym;
      QPointF spos;
      QPointF pos;
      };

class KeySig : public Element {
	bool	_showCourtesySig;
	bool	_showNaturals;
      QList<KeySym*> keySymbols;
      void addLayout(int sym, qreal x, int y);
      KeySigEvent _sig;

   public:
      KeySig(Score*);
      KeySig(const KeySig&);
      virtual KeySig* clone() const { return new KeySig(*this); }
      virtual QPointF canvasPos() const;      ///< position in canvas coordinates
      virtual void draw(Painter*) const;
      virtual ElementType type() const { return KEYSIG; }
      virtual void layout();
      void setSig(int oldSig, int newSig);
      void setOldSig(int oldSig);
      Segment* segment() const            { return (Segment*)parent(); }
      Measure* measure() const            { return (Measure*)parent()->parent(); }
      Space space() const;
      void setCustom(const QList<KeySym*>& symbols);
      virtual void read(XmlReader*);
      int keySignature() const            { return _sig.accidentalType(); }    // -7 - +7
      int customType() const              { return _sig.customType(); }
      bool isCustom() const               { return _sig.custom(); }
      KeySigEvent keySigEvent() const     { return _sig; }
      bool operator==(const KeySig&) const;
      void changeKeySigEvent(const KeySigEvent&);
      void setKeySigEvent(const KeySigEvent& e)      { _sig = e; }
      int tick() const;

      bool showCourtesySig() const        { return _showCourtesySig; };
      bool showNaturals() const           { return _showNaturals;    };
      void setShowCourtesySig(bool v)     { _showCourtesySig = v;    };
	void setShowNaturals(bool v)        { _showNaturals = v;       };
	};

extern const char* keyNames[15];

#endif

