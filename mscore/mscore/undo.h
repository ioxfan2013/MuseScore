//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: undo.h,v 1.17 2006/03/02 17:08:43 wschweer Exp $
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

#ifndef __UNDO_H__
#define __UNDO_H__

/**
 \file
 Definition of undo-releated classes and structs.
*/

#include "select.h"
#include "measure.h"
#include "input.h"

class ElementList;
class Element;
class Instrument;
class System;
class Measure;
class Segment;
class Staff;
class Part;

//---------------------------------------------------------
//   UndoOp
//---------------------------------------------------------

/**
 Specifies a single low level undo operation.
*/

struct UndoOp {
      enum UndoType {
            RemoveElement, AddElement,
            InsertPart, RemovePart,
            InsertStaff, RemoveStaff,
            InsertSegStaff, RemoveSegStaff,
            InsertMStaff, RemoveMStaff,
            InsertMeasure, RemoveMeasure,
            InsertStaves, RemoveStaves,
            SortStaves,
            ToggleInvisible,
            ChangeColor,
            ChangePitch,
            ChangeSubtype,
            ChangeAccidental,
            FlipStemDirection,
            FlipSlurDirection,
            ChangeTimeSig,
            ChangeKeySig,
            ChangeClef,
            };
      UndoType type;
      Element* obj;
      Part* part;
      Staff* staff;
      MStaff mstaff;
      System* system;
      Measure* measure;
      Segment* segment;
      QList<int> si;
      QList<int> di;
      int val1, val2, val3;
      QColor color;

      const char* name() const;
      };

//---------------------------------------------------------
//   Undo
//---------------------------------------------------------

/**
 A single user visible undo.
*/

struct Undo : public QList<UndoOp> {
      InputState inputState;
      Selection selection;

   public:
      Undo(const InputState&, const Selection*);
      };

//---------------------------------------------------------
//   UndoList
//---------------------------------------------------------

/**
 An undo list: a list user visible undo actions, each of which is
 a list of low level undo operations.
*/

class UndoList : public QList<Undo*> {
   public:
      };

typedef UndoList::iterator iUndo;

#endif

