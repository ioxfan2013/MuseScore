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

#ifndef __IMPORTGTP_H__
#define __IMPORTGTP_H__

#include "al/fraction.h"

class Score;
class Note;
class Segment;
class Measure;
class Tuplet;

static const int GP_MAX_LYRIC_LINES = 5;
static const int GP_MAX_TRACK_NUMBER = 32;
static const int GP_MAX_STRING_NUMBER = 7;

static const int GP_INVALID_KEYSIG = 127;

struct GpTrack {
      int patch;
      uchar volume, pan, chorus, reverb, phase, tremolo;
      };

struct GpBar {
      Fraction timesig;
      int keysig;
      QString marker;
      int barLine;
      int repeatFlags;
      int repeats;

      GpBar();
      };

struct GpNote {
      bool slur;
      };

//---------------------------------------------------------
//   GuitarPro
//---------------------------------------------------------

class GuitarPro {
   protected:
      static const char* errmsg[];
      int version;
      int key;

      Score* score;
      QFile* f;
      int curPos;

      void skip(qint64 len);
      void read(void* p, qint64 len);
      int readUChar();
      int readChar();
      QString readPascalString(int);
      QString readWordPascalString();
      QString readBytePascalString();
      int readInt();
      QString readDelphiString();
      virtual void readChromaticGraph();
      virtual void readMixChange();
      virtual void readBeatEffects() = 0;
      void readLyrics();
      void readChannels();
      void setTuplet(Tuplet* tuplet, int tuple);
      Fraction len2fraction(int len);
      void addDynamic(Note*, int d);
      void setTempo(int n);
      void createMeasures();

   public:
      QString title, subtitle, artist, album, composer;
      QString transcriber, instructions;
      QStringList comments;
      GpTrack channelDefaults[GP_MAX_TRACK_NUMBER * 2];
      int staves;
      int measures;
      QList<GpBar> bars;

      enum GuitarProError { GP_NO_ERROR, GP_UNKNOWN_FORMAT,
         GP_EOF, GP_BAD_NUMBER_OF_STRINGS
            };

      GuitarPro(Score*, int v);
      ~GuitarPro();
      virtual void read(QFile*) = 0;
      QString error(GuitarProError n) const { return QString(errmsg[int(n)]); }
      };

//---------------------------------------------------------
//   GuitarPro1
//---------------------------------------------------------

class GuitarPro1 : public GuitarPro {

   protected:
      virtual void readChord(Segment*, int track);
      void readNote(int string, Note* note);
      virtual void readBeatEffects();

   public:
      GuitarPro1(Score* s, int v) : GuitarPro(s, v) {}
      virtual void read(QFile*);
      };

//---------------------------------------------------------
//   GuitarPro2
//---------------------------------------------------------

class GuitarPro2 : public GuitarPro1 {

   public:
      GuitarPro2(Score* s, int v) : GuitarPro1(s, v) {}
      virtual void read(QFile*);
      };

//---------------------------------------------------------
//   GuitarPro3
//---------------------------------------------------------

class GuitarPro3 : public GuitarPro1 {

   public:
      GuitarPro3(Score* s, int v) : GuitarPro1(s, v) {}
      virtual void read(QFile*);
      };

//---------------------------------------------------------
//   GuitarPro4
//---------------------------------------------------------

class GuitarPro4 : public GuitarPro {

      void readInfo();
      void readNote(int string, Note* note, GpNote*);
      virtual void readChord(Segment*, int track);
      virtual void readBeatEffects();
      virtual void readMixChange();
      virtual void readChromaticGraph();

   public:
      GuitarPro4(Score* s, int v) : GuitarPro(s, v) {}
      virtual void read(QFile*);
      };

//---------------------------------------------------------
//   GuitarPro5
//---------------------------------------------------------

class GuitarPro5 : public GuitarPro {

      void readInfo();
      void readPageSetup();
      virtual void readBeatEffects();
      virtual void readChromaticGraph();
      void readNote(int string, Note* note);
      virtual void readMixChange();
      virtual void readChord(Segment*, int track);
      void readMeasure(Measure* measure, int staffIdx, Tuplet*[]);
      void readArtificialHarmonic();
      void readTracks();
      void readMeasures();
      int readBeat(int tick, int voice, Measure* measure, int staffIdx, Tuplet** tuplets);
      void readNoteEffects();
      void readTremoloBar();

   public:
      GuitarPro5(Score* s, int v) : GuitarPro(s, v) {}
      virtual void read(QFile*);
      };

#endif
