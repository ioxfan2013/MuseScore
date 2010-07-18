//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id$
//
//  Copyright (C) 2002-2009 Werner Schweer and others
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

#include "spatium.h"
#include "globals.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "input.h"
#include "style.h"
#include "key.h"
#include "select.h"
#include "instrument.h"
#include "msynth/synti.h"

class ElementList;
class Element;
class Instrument;
class System;
class Measure;
class Segment;
class Staff;
class Part;
class Volta;
class Score;
class Note;
class Chord;
class ChordRest;
class Harmony;
class SlurTie;
class Beam;
class MStaff;
class MeasureBase;
class Dynamic;
class Selection;
class TextB;
class Channel;
struct PageFormat;
class TextStyle;
class Tuplet;
class UndoGroup;
class KeySig;
class StaffType;
class TimeSig;
class Clef;
class Image;
class Hairpin;

//---------------------------------------------------------
//   UndoCommand
//---------------------------------------------------------

class UndoCommand {
      QList<UndoCommand*> childList;

   public:
      virtual ~UndoCommand();
      virtual void undo();
      virtual void redo();
      void appendChild(UndoCommand* cmd) { childList.append(cmd);       }
      UndoCommand* removeChild()         { return childList.takeLast(); }
      int childCount() const             { return childList.size();     }
      void unwind();
      };

//---------------------------------------------------------
//   UndoStack
//---------------------------------------------------------

class UndoStack : public QObject {
      Q_OBJECT
      UndoGroup* group;
      UndoCommand* curCmd;
      QList<UndoCommand*> list;
      int curIdx;
      int cleanIdx;

   signals:
      void canUndoChanged(bool);
      void canRedoChanged(bool);
      void cleanChanged(bool);

   public:
      UndoStack();
      ~UndoStack();

      bool active() const           { return curCmd != 0; }
      void beginMacro();
      void endMacro(bool rollback);
      void push(UndoCommand*);
      void pop();
      void setClean();
      bool canUndo() const          { return curIdx > 0;           }
      bool canRedo() const          { return curIdx < list.size(); }
      bool isClean() const          { return cleanIdx == curIdx;   }
      void setGroup(UndoGroup* g)   { group = g;                   }
      UndoCommand* current() const  { return curCmd;               }
      void undo();
      void redo();
      };

//---------------------------------------------------------
//   UndoGroup
//---------------------------------------------------------

class UndoGroup : public QObject {
      Q_OBJECT
      QList<UndoStack*> group;
      UndoStack* _activeStack;

   signals:
      void canUndoChanged(bool);
      void canRedoChanged(bool);
      void cleanChanged(bool);

   public slots:
      void undo();
      void redo();

   public:
      UndoGroup();
      void addStack(UndoStack*);
      void removeStack(UndoStack*);

      void setActiveStack(UndoStack*);
      bool canUndo() const;
      bool canRedo() const;
      bool isClean() const;
      };

//---------------------------------------------------------
//   SaveState
//---------------------------------------------------------

class SaveState : public UndoCommand {
      InputState undoInputState;
      InputState redoInputState;
      Selection  undoSelection;
      Selection  redoSelection;
      Score* score;

   public:
      SaveState(Score*);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   InsertPart
//---------------------------------------------------------

class InsertPart : public UndoCommand {
      Part* part;
      int idx;

   public:
      InsertPart(Part* p, int i);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   RemovePart
//---------------------------------------------------------

class RemovePart : public UndoCommand {
      Part* part;
      int idx;

   public:
      RemovePart(Part*, int idx);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   InsertStaff
//---------------------------------------------------------

class InsertStaff : public UndoCommand {
      Staff* staff;
      int idx;

   public:
      InsertStaff(Staff*, int idx);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   RemoveStaff
//---------------------------------------------------------

class RemoveStaff : public UndoCommand {
      Staff* staff;
      int idx;

   public:
      RemoveStaff(Staff*, int idx);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   InsertMStaff
//---------------------------------------------------------

class InsertMStaff : public UndoCommand {
      Measure* measure;
      MStaff* mstaff;
      int idx;

   public:
      InsertMStaff(Measure*, MStaff*, int);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   RemoveMStaff
//---------------------------------------------------------

class RemoveMStaff : public UndoCommand {
      Measure* measure;
      MStaff* mstaff;
      int idx;

   public:
      RemoveMStaff(Measure*, MStaff*, int);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   InsertMeasure
//---------------------------------------------------------

class InsertMeasure : public UndoCommand {
      MeasureBase* measure;

   public:
      InsertMeasure(MeasureBase*);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   InsertStaves
//---------------------------------------------------------

class InsertStaves : public UndoCommand {
      Measure* measure;
      int a;
      int b;

   public:
      InsertStaves(Measure*, int, int);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   RemoveStaves
//---------------------------------------------------------

class RemoveStaves : public UndoCommand {
      Measure* measure;
      int a;
      int b;

   public:
      RemoveStaves(Measure*, int, int);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   SortStaves
//---------------------------------------------------------

class SortStaves : public UndoCommand {
      Score* score;
      QList<int> list;

      void flip();

   public:
      SortStaves(Score*, QList<int>);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ToggleInvisible
//---------------------------------------------------------

class ToggleInvisible : public UndoCommand {
      Element* element;
      void flip();

   public:
      ToggleInvisible(Element*);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeColor
//---------------------------------------------------------

class ChangeColor : public UndoCommand {
      Element* element;
      QColor color;

      void flip();
   public:
      ChangeColor(Element*, QColor);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangePitch
//---------------------------------------------------------

class ChangePitch : public UndoCommand {
      Note* note;
      int pitch;
      int tpc;
      int userAccidental;
      int line;
      int fret;
      void flip();

   public:
      ChangePitch(Note* note, int pitch, int tpc, int userAccidental, int l, int f);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeTpc
//---------------------------------------------------------

class ChangeTpc : public UndoCommand {
      Note* note;
      int tpc;

      void flip();
   public:
      ChangeTpc(Note* note, int tpc);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeSubtype
//---------------------------------------------------------

class ChangeSubtype : public UndoCommand {
      Element* element;
      int subtype;
      void flip();

   public:
      ChangeSubtype(Element*, int subtype);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   SetStemDirection
//---------------------------------------------------------

class SetStemDirection : public UndoCommand {
      Chord* chord;
      Direction direction;
      void flip();

   public:
      SetStemDirection(Chord*, Direction);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   FlipSlurDirection
//---------------------------------------------------------

class FlipSlurDirection : public UndoCommand {
      SlurTie* slur;
      void flip();

   public:
      FlipSlurDirection(SlurTie*);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   FlipBeamDirection
//---------------------------------------------------------

class FlipBeamDirection : public UndoCommand {
      Beam* beam;
      void flip();

   public:
      FlipBeamDirection(Beam*);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeKeySig
//---------------------------------------------------------

class ChangeKeySig : public UndoCommand {
      KeySig* keysig;
      KeySigEvent ks;
      bool showCourtesy;
      bool showNaturals;

      void flip();

   public:
      ChangeKeySig(KeySig*, KeySigEvent newKeySig, bool sc, bool sn);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   FlipTupletDirection
//---------------------------------------------------------

class FlipTupletDirection : public UndoCommand {
      Tuplet* tuplet;
      void flip();

   public:
      FlipTupletDirection(Tuplet*);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeMeasureLen
//---------------------------------------------------------

class ChangeMeasureLen : public UndoCommand {
      Measure* measure;
      int oldTicks;
      int newTicks;
      void flip();

   public:
      ChangeMeasureLen(Measure*, int oldTicks, int newTicks);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeElement
//---------------------------------------------------------

class ChangeElement : public UndoCommand {
      Element* oldElement;
      Element* newElement;
      void flip();

   public:
      ChangeElement(Element* oldElement, Element* newElement);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   InsertTime
//---------------------------------------------------------

class InsertTime : public UndoCommand {
      Score* score;
      int tick;
      int len;
      void flip();

   public:
      InsertTime(Score*, int tick, int len);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeRepeatFlags
//---------------------------------------------------------

class ChangeRepeatFlags : public UndoCommand {
      Measure* measure;
      int flags;
      void flip();

   public:
      ChangeRepeatFlags(Measure*, int flags);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeVoltaEnding
//---------------------------------------------------------

class ChangeVoltaEnding : public UndoCommand {
      Volta* volta;
      QList<int> list;
      void flip();

   public:
      ChangeVoltaEnding(Volta*, const QList<int>&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeVoltaText
//---------------------------------------------------------

class ChangeVoltaText : public UndoCommand {
      Volta* volta;
      QString text;
      void flip();

   public:
      ChangeVoltaText(Volta*, const QString&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeChordRestSize
//---------------------------------------------------------

class ChangeChordRestSize : public UndoCommand {
      ChordRest* cr;
      bool small;
      void flip();

   public:
      ChangeChordRestSize(ChordRest*, bool small);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeChordNoStem
//---------------------------------------------------------

class ChangeChordNoStem : public UndoCommand {
      Chord* chord;
      bool noStem;
      void flip();

   public:
      ChangeChordNoStem(Chord*, bool noStem);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeChordRestSpace
//---------------------------------------------------------

class ChangeChordRestSpace : public UndoCommand {
      ChordRest* cr;
      Spatium l;
      Spatium t;
      void flip();

   public:
      ChangeChordRestSpace(ChordRest*, Spatium l, Spatium t);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeEndBarLineType
//---------------------------------------------------------

class ChangeEndBarLineType : public UndoCommand {
      Measure* measure;
      int subtype;
      void flip();

   public:
      ChangeEndBarLineType(Measure*, int subtype);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeBarLineSpan
//---------------------------------------------------------

class ChangeBarLineSpan : public UndoCommand {
      Staff* staff;
      int span;
      void flip();

   public:
      ChangeBarLineSpan(Staff*, int span);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeUserOffset
//---------------------------------------------------------

class ChangeUserOffset : public UndoCommand {
      Element* element;
      QPointF offset;
      void flip();

   public:
      ChangeUserOffset(Element*, const QPointF& offset);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeDynamic
//---------------------------------------------------------

class ChangeDynamic : public UndoCommand {
      Dynamic* dynamic;
      int velocity;
      DynamicType dynType;
      void flip();

   public:
      ChangeDynamic(Dynamic*, int velocity, DynamicType dt);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   SigInsertTime
//---------------------------------------------------------

class SigInsertTime : public UndoCommand {
      Score* score;
      int tick;
      int len;
      void flip();

   public:
      SigInsertTime(Score*, int tick, int len);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeBeamMode
//---------------------------------------------------------

class ChangeBeamMode : public UndoCommand {
      ChordRest* cr;
      BeamMode mode;
      void flip();

   public:
      ChangeBeamMode(ChordRest*, BeamMode mode);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeCopyright
//---------------------------------------------------------

class ChangeCopyright : public UndoCommand {
      Score* score;
      QString text;
      void flip();

   public:
      ChangeCopyright(Score*, const QString&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   TransposeHarmony
//---------------------------------------------------------

class TransposeHarmony : public UndoCommand {
      Harmony* harmony;
      int rootTpc, baseTpc;
      void flip();

   public:
      TransposeHarmony(Harmony*, int rootTpc, int baseTpc);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ExchangeVoice
//---------------------------------------------------------

class ExchangeVoice : public UndoCommand {
      Measure* measure;
      int val1, val2;
      int staff1, staff2;

   public:
      ExchangeVoice(Measure*, int val1, int val2, int staff1, int staff2);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   ChangeInstrumentShort
//---------------------------------------------------------

class ChangeInstrumentShort : public UndoCommand {
      Part* part;
      QString text;
      void flip();

   public:
      ChangeInstrumentShort(Part*, const QString&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeInstrumentLong
//---------------------------------------------------------

class ChangeInstrumentLong : public UndoCommand {
      Part* part;
      QString text;
      void flip();

   public:
      ChangeInstrumentLong(Part*, const QString&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeChordRestLen
//---------------------------------------------------------

class ChangeChordRestLen : public UndoCommand {
      ChordRest* cr;
      Duration d;
      void flip();

   public:
      ChangeChordRestLen(ChordRest*, const Duration& d);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   MoveElement
//---------------------------------------------------------

class MoveElement : public UndoCommand {
      Element* element;
      QPointF offset;
      void flip();

   public:
      MoveElement(Element*, const QPointF&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeBracketSpan
//---------------------------------------------------------

class ChangeBracketSpan : public UndoCommand {
      Staff* staff;
      int column;
      int span;
      void flip();

   public:
      ChangeBracketSpan(Staff*, int column, int span);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   AddElement
//---------------------------------------------------------

class AddElement : public UndoCommand {
      Element* element;

   public:
      AddElement(Element*);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   RemoveElement
//---------------------------------------------------------

class RemoveElement : public UndoCommand {
      Element* element;

   public:
      RemoveElement(Element*);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   ChangeNoteHead
//---------------------------------------------------------

class ChangeNoteHead : public UndoCommand {
      Note* note;
      int group;
      NoteHeadType type;
      void flip();

   public:
      ChangeNoteHead(Note* note, int group, NoteHeadType type);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeConcertPitch
//---------------------------------------------------------

class ChangeConcertPitch : public UndoCommand {
      Score* score;
      bool val;
      void flip();

   public:
      ChangeConcertPitch(Score* s, bool val);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   EditText
//---------------------------------------------------------

class EditText : public UndoCommand {
      TextB* text;
      int undoLevel;

   public:
      EditText(TextB* t, int l) : text(t), undoLevel(l) {}
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   ChangePatch
//---------------------------------------------------------

class ChangePatch : public UndoCommand {
      Part* part;
      Channel* channel;
      MidiPatch patch;

      void flip();

   public:
      ChangePatch(Part* p, Channel* c, const MidiPatch* pt)
         : part(p), channel(c), patch(*pt) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeTuning
//---------------------------------------------------------

class ChangeTuning : public UndoCommand {
      Note* note;
      double tuning;

      void flip();

   public:
      ChangeTuning(Note* n, double t) : note(n), tuning(t) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeUserMirror
//---------------------------------------------------------

class ChangeUserMirror : public UndoCommand {
      Note* note;
      DirectionH dir;

      void flip();

   public:
      ChangeUserMirror(Note* n, DirectionH d) : note(n), dir(d) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangePageFormat
//---------------------------------------------------------

class ChangePageFormat : public UndoCommand {
      Score* score;
      PageFormat* pf;
      double spatium;

      void flip();

   public:
      ChangePageFormat(Score*, PageFormat*, double sp);
      ~ChangePageFormat();
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeStaff
//---------------------------------------------------------

class ChangeStaff : public UndoCommand {
      Staff* staff;
      bool small;
      bool invisible;
      StaffType* staffType;

      void flip();

   public:
      ChangeStaff(Staff*, bool small, bool invisible, StaffType*);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangePart
//---------------------------------------------------------

class ChangePart : public UndoCommand {
      Part* part;
      QTextDocument* longName;
      QTextDocument* shortName;
      Instrument instrument;

      void flip();

   public:
      ChangePart(Part*, const QTextDocument*, const QTextDocument*, const Instrument&);
      ~ChangePart();

      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeTextStyles
//---------------------------------------------------------

class ChangeTextStyles : public UndoCommand {
      Score* score;
      QVector<TextStyle*> styles;
      void flip();

   public:
      ChangeTextStyles(Score*, const QVector<TextStyle*>& styles);
      ~ChangeTextStyles();
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeStretch
//---------------------------------------------------------

class ChangeStretch : public UndoCommand {
      Measure* measure;
      double stretch;
      void flip();

   public:
      ChangeStretch(Measure*, double);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeStyle
//---------------------------------------------------------

class ChangeStyle : public UndoCommand {
      Score* score;
      Style style;
      void flip();

   public:
      ChangeStyle(Score*, const Style&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeSlurProperties
//---------------------------------------------------------

class ChangeSlurProperties : public UndoCommand {
      SlurTie* st;
      int lineType;
      void flip();

   public:
      ChangeSlurProperties(SlurTie*, int);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeChordStaffMove
//---------------------------------------------------------

class ChangeChordStaffMove : public UndoCommand {
      Chord* chord;
      int staffMove;
      void flip();

   public:
      ChangeChordStaffMove(Chord*, int);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeTupletProperties
//---------------------------------------------------------

class ChangeTupletProperties : public UndoCommand {
      Tuplet* tuplet;
      int numberType;
      int bracketType;
      void flip();

   public:
      ChangeTupletProperties(Tuplet*, int numberType, int bracketType);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeVelocity
//---------------------------------------------------------

class ChangeVelocity : public UndoCommand {
      Note* note;
      ValueType veloType;
      int velocity;
      int veloOffset;
      void flip();

   public:
      ChangeVelocity(Note*, ValueType, int, int);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeMStaffProperties
//---------------------------------------------------------

class ChangeMStaffProperties : public UndoCommand {
      MStaff* mstaff;
      bool visible;
      bool slashStyle;
      void flip();

   public:
      ChangeMStaffProperties(MStaff*, bool visible, bool slashStyle);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeMeasureProperties
//---------------------------------------------------------

class ChangeMeasureProperties : public UndoCommand {
      Measure* measure;
      Fraction sig;
      Fraction len;
      bool breakMM;
      int repeatCount;
      double stretch;
      int noOffset;
      bool irregular;

      void flip();
   public:
      ChangeMeasureProperties(Measure*, const Fraction&, const Fraction&, bool breakMM,
         int repeatCount, double stretch, int noOffset, bool irregular);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeNoteProperties
//---------------------------------------------------------

class ChangeNoteProperties : public UndoCommand {
      Note* note;

      ValueType _veloType;
      char _velocity;         ///< midi playback velocity (0 - 127);
      int _veloOffset;        ///< velocity user offset in promille

      int _onTimeUserOffset;  ///< start note user offset
      int _offTimeUserOffset; ///< stop note user offset

      void flip();

   public:
      ChangeNoteProperties(Note*, ValueType, int, int, int, int);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeMeasureTimesig
//---------------------------------------------------------

class ChangeMeasureTimesig : public UndoCommand {
      Measure* m;
      Fraction ts;

      void flip();

   public:
      ChangeMeasureTimesig(Measure*, const Fraction&);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeTimesig
//---------------------------------------------------------

class ChangeTimesig : public UndoCommand {
      TimeSig* timesig;
      bool showCourtesy;

      void flip();

   public:
      ChangeTimesig(TimeSig * _timesig, bool sc);
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   RemoveMeasures
//---------------------------------------------------------

class RemoveMeasures : public UndoCommand {
      Measure* fm;
      Measure* lm;

   public:
      RemoveMeasures(Measure*, Measure*);
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   InsertMeasures
//---------------------------------------------------------

class InsertMeasures : public UndoCommand {
      Measure* fm;
      Measure* lm;

   public:
      InsertMeasures(Measure* m1, Measure* m2) : fm(m1), lm(m2) {}
      virtual void undo();
      virtual void redo();
      };

//---------------------------------------------------------
//   ChangeClef
//---------------------------------------------------------

class ChangeClef : public UndoCommand {
      Clef* clef;
      bool showCourtesy;

      void flip();

   public:
      ChangeClef(Clef* _clef, bool sc) : clef(_clef), showCourtesy(sc) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeImage
//---------------------------------------------------------

class ChangeImage : public UndoCommand {
      Image* image;
      bool lockAspectRatio;
      bool autoScale;

      void flip();

   public:
      ChangeImage(Image* i, bool l, bool a) : image(i), lockAspectRatio(l), autoScale(a) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeHairpin
//---------------------------------------------------------

class ChangeHairpin : public UndoCommand {
      Hairpin* hairpin;
      int veloChange;
      DynamicType dynType;

      void flip();

   public:
      ChangeHairpin(Hairpin* h, int c, DynamicType t)
         : hairpin(h), veloChange(c), dynType(t) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

//---------------------------------------------------------
//   ChangeDuration
//---------------------------------------------------------

class ChangeDuration : public UndoCommand {
      ChordRest* cr;
      Fraction d;

      void flip();

   public:
      ChangeDuration(ChordRest* _cr, Fraction _d) : cr(_cr), d(_d) {}
      virtual void undo() { flip(); }
      virtual void redo() { flip(); }
      };

#endif


