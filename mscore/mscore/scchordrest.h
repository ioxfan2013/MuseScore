//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: scchord.h 1840 2009-05-20 11:57:51Z wschweer $
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

#ifndef __SCCHORDREST_H__
#define __SCCHORDREST_H__

class ChordRest;
typedef ChordRest* ChordRestPtr;

class Harmony;
typedef Harmony* HarmonyPtr;

//---------------------------------------------------------
//   ScChordRestPrototype
//---------------------------------------------------------

class ScChordRestPrototype : public QObject, public QScriptable
      {
      Q_OBJECT
      ChordRest* thisChordRest() const;
      Q_PROPERTY(int tickLen READ getTickLen WRITE setTickLen SCRIPTABLE true)

   public slots:
      void addHarmony(HarmonyPtr h);

   public:
      ScChordRestPrototype(QObject *parent = 0) : QObject(parent) {}
      ~ScChordRestPrototype() {}

      int getTickLen() const;
      void setTickLen(int v);
      };

Q_DECLARE_METATYPE(ChordRestPtr)
Q_DECLARE_METATYPE(ChordRestPtr*)

#endif