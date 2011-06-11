//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id$
//
//  Copyright (C) 2002-2010 Werner Schweer and others
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

#ifndef __MEASURE_H__
#define __MEASURE_H__

/**
 \file
 Definition of classes MStaff, Measure and MeasureList.
*/

#include "measurebase.h"
#include "al/fraction.h"
#include "libmscore/segmentlist.h"

class Xml;
class Beam;
class Tuplet;
class Staff;
class Chord;
class Text;
class ChordRest;
class Score;
class ScoreView;
class System;
class Note;
class Spacer;
class SlurMap;
class TieMap;
class SpannerMap;

//---------------------------------------------------------
//   MStaff
//---------------------------------------------------------

/**
 Per staff values of measure.
*/

struct MStaff {
      double distanceUp;
      double distanceDown;
      StaffLines*  lines;
      Spacer* _vspacerUp;
      Spacer* _vspacerDown;
      bool hasVoices;         ///< indicates that MStaff contains more than one voice,
                              ///< this changes some layout rules
      bool _visible;
      bool _slashStyle;

      MStaff();
      ~MStaff();
      bool visible() const         { return _visible;    }
      void setVisible(bool val)    { _visible = val;     }
      bool slashStyle() const      { return _slashStyle; }
      void setSlashStyle(bool val) { _slashStyle = val;  }
      void setScore(Score*);
      void setTrack(int);
      };

enum {
      RepeatEnd         = 1,
      RepeatStart       = 2,
      RepeatMeasureFlag = 4,
      RepeatJump        = 8
      };

//---------------------------------------------------------
//   Measure
//    one measure in a system
//---------------------------------------------------------

class Measure : public MeasureBase {
      Q_DECLARE_TR_FUNCTIONS(Measure)

      SegmentList _segments;

      Fraction _timesig;
      Fraction _len;          ///< actual length of measure

      int _repeatCount;       ///< end repeat marker und repeat count
      int _repeatFlags;       ///< or'd RepeatType's

      QList<MStaff*>  staves;
      QList<Tuplet*>  _tuplets;

      int    _no;             ///< Measure number, counting from zero
      int    _noOffset;       ///< Offset to measure number
      Text* _noText;          ///< Measure number text object

      double _userStretch;

      bool _irregular;              ///< Irregular measure, do not count
      bool _breakMultiMeasureRest;  ///< set by user
      bool _breakMMRest;            ///< set by layout

      bool _endBarLineGenerated;
      bool _endBarLineVisible;
      BarLineType _endBarLineType;

      BarLineType _mmEndBarLineType;       ///< bar line type if this measure is presented
                                   ///< as multi measure rest

      int _multiMeasure;      // set from layout();
                              //   0 - normal measure
                              // > 0 - multi measure rest;
                              // < 0 - skipped measure

      int _playbackCount;     // temp. value used in RepeatList
                              // counts how many times this measure was already played

      QColor _endBarLineColor;

      void push_back(Segment* e);
      void push_front(Segment* e);

   public:
      Measure(Score*);
      Measure(const Measure&);
      ~Measure();
      virtual Measure* clone() const   { return new Measure(*this); }
      virtual ElementType type() const { return MEASURE; }
      virtual void setScore(Score* s);
      Measure* cloneMeasure(Score*, SlurMap*, TieMap*, SpannerMap*);

      virtual void read(QDomElement, int idx);
      virtual void write(Xml&, int, bool writeSystemElements) const;
      virtual void write(Xml&) const;
      void writeBox(Xml&) const;
      void readBox(QDomElement);
      virtual bool isEditable() const { return false; }

      virtual void add(Element*);
      virtual void remove(Element*);
      virtual bool genPropertyMenu(QMenu*) const;
      virtual void propertyAction(ScoreView*, const QString&);

      System* system() const               { return (System*)parent(); }
      QList<MStaff*>* staffList()          { return &staves;      }
      MStaff* mstaff(int staffIdx)         { return staves[staffIdx]; }
      StaffLines* staffLines(int staffIdx) { return staves[staffIdx]->lines; }
      QList<Tuplet*>* tuplets()            { return &_tuplets;    }
      int no() const                       { return _no;          }
      bool irregular() const               { return _irregular;   }
      void setIrregular(bool val)          { _irregular = val;    }
      int noOffset() const                 { return _noOffset;    }
      Text* noText() const                 { return _noText;      }
      void setNo(int n)                    { _no = n;             }
      void setNoOffset(int n)              { _noOffset = n;       }
      virtual double distanceUp(int i) const   { return staves[i]->distanceUp; }
      virtual double distanceDown(int i) const { return staves[i]->distanceDown; }
      virtual Spatium userDistanceUp(int i) const;
      virtual Spatium userDistanceDown(int i) const;

      Fraction timesig() const             { return _timesig;     }
      void setTimesig(const Fraction& f)   { _timesig = f;        }
      Fraction len() const                 { return _len;         }
      Fraction stretchedLen(Staff*) const;
      void setLen(const Fraction& f)       { _len = f;            }
      virtual int ticks() const            { return _len.ticks(); }

      int size() const                     { return _segments.size();        }
      Segment* first() const               { return _segments.first();       }
      Segment* first(SegmentTypes t) const { return _segments.first(t);      }

      Segment* last() const                { return _segments.last();        }
      Segment* firstCRSegment() const      { return _segments.firstCRSegment(); }
      void remove(Segment* s);
      SegmentList* segments()              { return &_segments; }

      double userStretch() const           { return _userStretch; }
      void setUserStretch(double v)        { _userStretch = v;    }

      void layoutX(double stretch);
      void layout(double width);
      void layout2();

      Chord* findChord(int tick, int track, int gl);
      ChordRest* findChordRest(int tick, int track);
      int snap(int tick, const QPointF p) const;
      int snapNote(int tick, const QPointF p, int staff) const;

      void insertStaff(Staff*, int staff);
      void insertMStaff(MStaff* staff, int idx);
      void removeMStaff(MStaff* staff, int idx);

      virtual void moveTicks(int diff);

      void cmdRemoveStaves(int s, int e);
      void cmdAddStaves(int s, int e, bool createRest);
      void removeStaves(int s, int e);
      void insertStaves(int s, int e);

      double tick2pos(int) const;
      Segment* tick2segment(int, bool grace = false) const;

      void sortStaves(QList<int>& dst);

      void dump() const;
      virtual bool acceptDrop(ScoreView*, const QPointF&, int, int) const;
      virtual Element* drop(const DropData&);

      int repeatCount() const         { return _repeatCount; }
      void setRepeatCount(int val)    { _repeatCount = val; }

      Segment* getSegment(Element* el, int tick);
      Segment* getSegment(SegmentType st, int tick);
      Segment* getSegment(SegmentType st, int tick, int gl);
      Segment* findSegment(SegmentType st, int t);

      bool createEndBarLines();
      void setEndBarLineType(BarLineType val, bool g, bool visible = true, QColor color = Qt::black);
      BarLineType endBarLineType() const        { return _endBarLineType; }
      void setMmEndBarLineType(BarLineType v)   { _mmEndBarLineType = v;    }
      bool setStartRepeatBarLine(bool);
      bool endBarLineGenerated() const          { return _endBarLineGenerated; }
      void setEndBarLineGenerated(bool v)       { _endBarLineGenerated = v;    }
      bool endBarLineVisible() const            { return _endBarLineVisible;   }
      QColor endBarLineColor() const            { return _endBarLineColor;     }

      void cmdRemoveEmptySegment(Segment* s);
      virtual void scanElements(void* data, void (*func)(void*, Element*));
      virtual void scanVisibleElements(void* data, void (*func)(void*, Element*), bool onlyVisible);
      void createVoice(int track);
      void adjustToLen(int, int);
      int repeatFlags() const                   { return _repeatFlags; }
      void setRepeatFlags(int val);
      int findAccidental(Note*) const;
      int findAccidental2(Note*) const;
      void exchangeVoice(int, int, int, int);
      void checkMultiVoices(int staffIdx);
      bool hasVoice(int track) const;
      bool isMeasureRest(int staffIdx);
      bool isFullMeasureRest();
      bool visible(int staffIdx) const;
      bool slashStyle(int staffIdx) const;

      bool breakMultiMeasureRest() const        { return _breakMultiMeasureRest | _breakMMRest; }
      bool breakMMRest() const                  { return _breakMMRest; }
      void setBreakMMRest(bool v)               { _breakMMRest = v;    }
      bool getBreakMultiMeasureRest() const     { return _breakMultiMeasureRest; }
      void setBreakMultiMeasureRest(bool val)   { _breakMultiMeasureRest = val;  }

      bool isEmpty() const;

      int multiMeasure() const                  { return _multiMeasure; }
      void setMultiMeasure(int val)             { _multiMeasure = val;  }
      void layoutChords0(Segment* segment, int startTrack);
      void layoutChords10(Segment* segment, int startTrack, char* tversatz);
      void updateAccidentals(Segment* segment, int staffIdx, char* tversatz);
      void layoutStage1();
      int playbackCount() const      { return _playbackCount; }
      void setPlaybackCount(int val) { _playbackCount = val; }
      QRectF staffabbox(int staffIdx) const;
      };

extern void initLineList(char* ll, int key);

#endif

