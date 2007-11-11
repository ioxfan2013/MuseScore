//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: importxml.cpp,v 1.81 2006/04/13 07:36:48 wschweer Exp $
//
//  Copyright (C) 2002-2007 Werner Schweer and others
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

/**
 MusicXML import.
 */

#include "config.h"
#include "mscore.h"
#include "musicxml.h"
#include "file.h"
#include "score.h"
#include "rest.h"
#include "chord.h"
#include "sig.h"
#include "key.h"
#include "clef.h"
#include "note.h"
#include "element.h"
#include "sym.h"
#include "slur.h"
#include "hairpin.h"
#include "tuplet.h"
#include "segment.h"
#include "dynamics.h"
#include "page.h"
#include "staff.h"
#include "part.h"
#include "measure.h"
#include "style.h"
#include "bracket.h"
#include "layout.h"
#include "timesig.h"
#include "xml.h"
#include "barline.h"
#include "lyrics.h"
#include "volta.h"
#include "keysig.h"
#include "pitchspelling.h"
#include "layoutbreak.h"
#include "tremolo.h"

//---------------------------------------------------------
//   xmlSetPitch
//---------------------------------------------------------

/**
 Convert MusicXML \a step / \a alter / \a octave to midi pitch,
 set line and user accidental.
 */

static void xmlSetPitch(Note* n, int /*tick*/, char step, int alter, int octave, int /*accidental*/)
      {
//      printf("xmlSetPitch(n=%p, tick=%d, st=%c, alter=%d, octave=%d, accidental=%d)",
//             n, tick, step, alter, octave, accidental);
      int istep = step - 'A';
      //                       a  b   c  d  e  f  g
      static int table[7]  = { 9, 11, 0, 2, 4, 5, 7 };
      if (istep < 0 || istep > 6) {
            printf("xmlSetPitch: illegal pitch %d, <%c>\n", istep, step);
            return;
            }
      int pitch = table[istep] + alter + (octave+1) * 12;

      if (pitch < 0)
            pitch = 0;
      if (pitch > 127)
            pitch = 127;
      n->setPitch(pitch);

      //                        a  b  c  d  e  f  g
      static int table1[7]  = { 5, 6, 0, 1, 2, 3, 4 };
      int line = table1[istep];
      int tpc  = line2tpc(line, alter);
      n->setTpc(tpc);
      }

//---------------------------------------------------------
//   xml2voltaType
//---------------------------------------------------------

/**
 Convert MusicXML ending number to volta subtype
 and return subtype (1..3) or 0 on failure
 */

static int xml2voltaType(QString en)
      {
      int res = 0;
      if (en == "1")
            res = 1;
      else if (en == "2")
            res = 2;
      else if (en == "3")
            res = 3;
      return res;
      }

//---------------------------------------------------------
//   MusicXml
//---------------------------------------------------------

/**
 MusicXml constructor.
 */

MusicXml::MusicXml(QDomDocument* d)
      {
      doc = d;
      maxLyrics = 0;
      }

//---------------------------------------------------------
//   LoadMusicXml
//---------------------------------------------------------

/**
 LoadMusicXml constructor.
 */

class LoadMusicXml : public LoadFile {
      QDomDocument* _doc;

   public:
      LoadMusicXml() {
            _doc = new QDomDocument();
            }
      ~LoadMusicXml() {
            delete _doc;
            }
      virtual bool loader(QFile* f);
      QDomDocument* doc() const { return _doc; }
      };

//---------------------------------------------------------
//   loader
//---------------------------------------------------------

/**
 Load MusicXML file \a qf, return false if OK and true on error.
 */

bool LoadMusicXml::loader(QFile* qf)
      {
      int line, column;
      QString err;
      if (!_doc->setContent(qf, false, &err, &line, &column)) {
            QString col, ln;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n at line " + ln + " column " + col;
            printf("error: %s\n", error.toLatin1().data());
            return true;
            }
      docName = qf->fileName();
      return false;
      }

//---------------------------------------------------------
//   importMusicXml
//---------------------------------------------------------

/**
 Import MusicXML file \a name into the Score.
 */

void Score::importMusicXml(const QString& name)
      {
      LoadMusicXml lx;
      lx.load(name);
      setSaved(false);
      MusicXml musicxml(lx.doc());
      musicxml.import(this);
      _layout->connectTies();
      layoutAll = true;
      _created = true;
      }

//---------------------------------------------------------
//   import
//      scorePartwise
//        part-list
//        part
//        work
//        identification
//---------------------------------------------------------

/**
 Parse the MusicXML file, which must be in score-partwise format.
 */

void MusicXml::import(Score* s)
      {
      score  = s;
      tie    = 0;
      for (int i = 0; i < MAX_SLURS; ++i)
            slur[i] = 0;
      tuplet = 0;

      for (QDomElement e = doc->documentElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "score-partwise")
                  scorePartwise(e.firstChildElement());
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   scorePartwise
//---------------------------------------------------------

/**
 Read the MusicXML score-partwise element.
 */

void MusicXml::scorePartwise(QDomElement e)
      {
      for (;!e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            if (tag == "part-list")
                  xmlPartList(e.firstChildElement());
            else if (tag == "part")
                  xmlPart(e.firstChildElement(), e.attribute(QString("id")));
            else if (tag == "work") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "work-number")
                              subTitle = ee.text();
                        else if (ee.tagName() == "work-title")
                              title = ee.text();
                        else
                              domError(ee);
                        }
                  }
            else if (tag == "identification") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "creator") {
                              // type is an arbitrary label
                              QString type = ee.attribute(QString("type"));
                              QString str = ee.text();
                              if (type == "composer")
                                    composer = str;
                              else if (type == "poet")
                                    poet = str;
                              else if (type == "translator")
                                    translator = str;
                              else if (type == "transcriber")
                                    ;
                              else
                                    printf("unknown creator <%s>\n", type.toLatin1().data());
                              }
                        else if (e.tagName() == "rights") {
                              QTextDocument* doc = new QTextDocument(0);
                              doc->setPlainText(ee.text());
                              score->setCopyright(doc);
                              }
                        else if (ee.tagName() == "encoding")
                              domNotImplemented(ee);
                        else if (e.tagName() == "source")
                              domNotImplemented(ee);
                        else
                              domError(ee);
                        }
                  }
            else if (tag == "defaults") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        QString tag(ee.tagName());
                        if (tag == "scaling") {
                              double millimeter = _spatium/10.0;
                              double tenths = 1.0;
                              for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                    QString tag(eee.tagName());
                                    if (tag == "millimeters")
                                          millimeter = eee.text().toDouble();
                                    else if (tag == "tenths")
                                          tenths = eee.text().toDouble();
                                    else
                                          domError(eee);
                                    }
                              _spatium = DPMM * (millimeter * 10.0 / tenths);
                              }
                        else if (tag == "page-layout")
                              score->pageFormat()->read(ee);
                        else if (tag == "system-layout") {
                              for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                    QString tag(eee.tagName());
                                    Spatium val(eee.text().toDouble() / 10.0);
                                    if (tag == "system-margins")
                                          ;
                                    else if (tag == "system-distance") {
                                          score->style()->systemDistance = val;
                                          printf("system distance %f\n", val.val());
                                          }
                                    else if (tag == "top-system-distance")
                                          ;
                                    else
                                          domError(eee);
                                    }
                              }
                        else if (tag == "staff-layout") {
                              for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                    QString tag(eee.tagName());
                                    Spatium val(eee.text().toDouble() / 10.0);
                                    if (tag == "staff-distance")
                                          score->style()->staffDistance = val;
                                    else
                                          domError(eee);
                                    }
                              }
                        else if (tag == "music-font")
                              domNotImplemented(ee);
                        else if (tag == "word-font")
                              domNotImplemented(ee);
                        else if (tag == "lyric-font")
                              domNotImplemented(ee);
                        else
                              domError(ee);
                        }
                  }
            else if (tag == "movement-number")
                  score->movementNumber = e.text();
            else if (tag == "movement-title")
                  score->movementTitle = e.text();
            else if (tag == "credit")
                  domNotImplemented(e);
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   xmlPartList
//---------------------------------------------------------

/**
 Read the MusicXML part-list element.
 */

void MusicXml::xmlPartList(QDomElement e)
      {
      for (;!e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "score-part")
                  xmlScorePart(e.firstChildElement(), e.attribute(QString("id")));
            else if (e.tagName() == "part-group")
                  ;
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   xmlScorePart
//---------------------------------------------------------

/**
 Read the MusicXML score-part element.
 */

void MusicXml::xmlScorePart(QDomElement e, QString id)
      {
      Part* part = new Part(score);
      part->setId(id);

// printf("create track id:<%s>\n", id.toLatin1().data());

      for (;!e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "part-name") {
                  part->setLongName(e.text());
                  part->setTrackName(e.text());
                  }
            else if (e.tagName() == "part-abbreviation")
                  ;
            else if (e.tagName() == "score-instrument") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "instrument-name") {
                              // part-name or instrument-name?
                              if (part->longName().isEmpty())
                                    part->setLongName(ee.text());
                              }
                        else
                              domError(ee);
                        }
                  }
            else if (e.tagName() == "midi-instrument") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "midi-channel")
                              part->setMidiChannel(ee.text().toInt() - 1);
                        else if (ee.tagName() == "midi-program")
                              part->setMidiProgram(ee.text().toInt() - 1);
                        else
                              domError(ee);
                        }
                  }
            else
                  domError(e);
            }
      score->parts()->push_back(part);
      Staff* staff = new Staff(score, part, 0);
      part->staves()->push_back(staff);
      score->staves().push_back(staff);
      }

//---------------------------------------------------------
//   xmlPart
//---------------------------------------------------------

/**
 Read the MusicXML part element.
 */

void MusicXml::xmlPart(QDomElement e, QString id)
      {
      Part* part = 0;
      foreach(Part* p, *score->parts()) {
            if (p->id() == id) {
                  part = p;
                  break;
                  }
            }
      if (part == 0) {
            printf("Import MusicXml:xmlPart: cannot find part %s\n", id.toLatin1().data());
            exit(-1);
            }
      tick           = 0;
      maxtick        = 0;
      lastMeasureLen = 0;

      if (!score->mainLayout()->first()) {
            Measure* measure  = new Measure(score);
            measure->setTick(tick);
            score->mainLayout()->push_back(measure);
            if (!title.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_TITLE);
                  text->setText(title);
                  measure->add(text);
                  }
            else if (!score->movementTitle.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_TITLE);
                  text->setText(score->movementTitle);
                  measure->add(text);
                  }
            if (!subTitle.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_SUBTITLE);
                  text->setText(subTitle);
                  measure->add(text);
                  }
            else if (!score->movementNumber.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_SUBTITLE);
                  text->setText(score->movementNumber);
                  measure->add(text);
                  }
            if (!composer.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_COMPOSER);
                  text->setText(composer);
                  measure->add(text);
                  }
            if (!poet.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_POET);
                  text->setText(poet);
                  measure->add(text);
                  }
            if (!translator.isEmpty()) {
                  Text* text = new Text(score);
                  text->setSubtype(TEXT_TRANSLATOR);
                  text->setText(translator);
                  measure->add(text);
                  }
            }

      for (; !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "measure") {
                  xmlMeasure(part, e, e.attribute(QString("number")).toInt()-1);
                  }
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   xmlMeasure
//---------------------------------------------------------

/**
 Read the MusicXML measure element.
 */

Measure* MusicXml::xmlMeasure(Part* part, QDomElement e, int number)
      {
      int staves = score->nstaves();
      if (staves == 0) {
            printf("no staves!\n");
            return 0;
            }

      // search measure for tick
      Measure* measure = 0;
      Measure* lastMeasure = 0;
      for (MeasureBase* mb = score->mainLayout()->first(); mb; mb = mb->next()) {
            if (mb->type() != MEASURE)
                  continue;
            Measure* m = (Measure*)mb;
            lastMeasure = m;
            if (m->tick() == tick) {
                  measure = m;
                  break;
                  }
            }
      if (!measure) {
            //
            // DEBUG:
            if (lastMeasure->tick() > tick) {
                  printf("Measure at position %d not found!\n", tick);
                  }
            measure  = new Measure(score);
            measure->setTick(tick);
            measure->setNo(number);
            score->mainLayout()->push_back(measure);
            }

      // initialize voice list
      // for (int i = 0; i < part->nstaves(); ++i) {
      for (int i = 0; i < MAX_STAVES; ++i)
            voicelist[i].clear();

      // must remember volta to handle <ending type="discontinue">
      Volta* lastVolta = 0;

      QString implicit = e.attribute("implicit", "no");
      if (implicit == "yes")
            measure->setIrregular(true);

      int staff = score->staff(part);
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "attributes")
                  xmlAttributes(measure, staff, e.firstChildElement());
            else if (e.tagName() == "note")
                  xmlNote(measure, staff, e.firstChildElement());
            else if (e.tagName() == "backup") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "duration") {
                              int val = ee.text().toInt();
                              if (val == 0)     // neuratron scanner produces sometimes 0 !?
                                    val = 1;
                              val = (val * ::division) / divisions;
                              tick -= val;
                              lastLen = val;    // ?
                              }
                        else
                              domError(ee);
                        }
                  }
            else if (e.tagName() == "direction") {
                  direction(measure, staff, e);
                  }
            else if (e.tagName() == "print") {
                  QString newSystem = e.attribute("new-system", "no");
                  QString newPage   = e.attribute("new-page", "no");
                  //
                  // in MScore the break happens _after_ the marked measure:
                  //
                  Measure* pm = (Measure*)(measure->prev());      // TODO: MeasureBase
                  if (pm == 0)
                        printf("ImportXml: warning: break on first measure\n");
                  else {
                        if (newSystem == "yes" || newPage == "yes") {
                              LayoutBreak* lb = new LayoutBreak(score);
                              lb->setStaff(score->staff(staff));
                              lb->setSubtype(
                                 newSystem == "yes" ? LAYOUT_BREAK_LINE : LAYOUT_BREAK_PAGE
                                 );
                              pm->add(lb);
                              }
                        }
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "system-layout") {
                              }
                        else if (ee.tagName() == "staff-layout") {
                              }
                        else
                              domError(ee);
                        }
                  }
            else if (e.tagName() == "forward") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "duration") {
                              int val = (ee.text().toInt() * ::division)/divisions;
                              tick += val;
                              if (tick > maxtick)
                                    maxtick = tick;
                              lastLen = val;    // ?
                              }
                        else if (ee.tagName() == "voice")
                              ;
                        else if (ee.tagName() == "staff")
                              ;
                        else
                              domError(ee);
                        }
                  }
            else if (e.tagName() == "barline") {
                  QString loc = e.attribute("location", "right");
                  QString barStyle;
                  QString endingNumber;
                  QString endingType;
                  QString repeat;
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "bar-style")
                              barStyle = ee.text();
                        else if (ee.tagName() == "ending") {
                              endingNumber = ee.attribute("number");
                              endingType   = ee.attribute("type");
                              }
                        else if (ee.tagName() == "repeat")
                              repeat = ee.attribute("direction");
                        else
                              domError(ee);
                        }
                  if ((barStyle != "") || (repeat != "")) {
                        BarLine* barLine = new BarLine(score);
                        if (barStyle == "light-heavy" && repeat == "backward") {
                              barLine->setSubtype(END_REPEAT);
                              }
                        else if (barStyle == "heavy-light" && repeat == "forward") {
                              barLine->setSubtype(START_REPEAT);
                              }
                        else if (barStyle == "light-heavy" && repeat.isEmpty())
                              barLine->setSubtype(END_BAR);
                        else if (barStyle == "regular")
                              barLine->setSubtype(NORMAL_BAR);
                        else if (barStyle == "dotted")
                              barLine->setSubtype(BROKEN_BAR);
                        else if (barStyle == "light-light")
                              barLine->setSubtype(DOUBLE_BAR);
      /*                  else if (barStyle == "heavy-light")
                              ;
                        else if (barStyle == "heavy-heavy")
                              ;
      */
                        else if (barStyle == "none")
                              barLine->setSubtype(NORMAL_BAR);
                        else if (barStyle == "") {
                              if (repeat == "backward")
                                    barLine->setSubtype(END_REPEAT);
                              else if (repeat == "forward")
                                    barLine->setSubtype(START_REPEAT);
                              else
                                    printf("ImportXml: warning: empty bar type\n");
                              }
                        else
                              printf("unsupported bar type <%s>\n", barStyle.toLatin1().data());
                        barLine->setStaff(score->staff(staff));
                        if (barLine->subtype() == START_REPEAT) {
                              barLine->setTick(measure->tick());
                              Segment* s = measure->getSegment(barLine);
                              s->add(barLine);
                              }
                        else
                              measure->setEndBarLine(barLine);
                        }
                  if (!(endingNumber.isEmpty() && endingType.isEmpty())) {
                        if (endingNumber.isEmpty())
                              printf("ImportXml: warning: empty ending number\n");
                        else if (endingType.isEmpty())
                              printf("ImportXml: warning: empty ending type\n");
                        else {
                              int subtype = xml2voltaType(endingNumber);
                              if (!subtype)
                                    printf("ImportXml: warning: unsupported ending type <%s>\n",
                                            endingType.toLatin1().data());
                              else {
                                    if (endingType == "start") {
                                          Volta* volta = new Volta(score);
                                          volta->setStaff(score->staff(staff));
                                          volta->setSubtype(subtype);
                                          measure->add(volta);
                                          lastVolta = volta;
                                          }
                                    else if (endingType == "stop") {
                                          lastVolta = 0;
                                          }
                                    else if (endingType == "discontinue") {
                                          if ((subtype == 2) && lastVolta && (lastVolta->subtype() == 2))
                                                lastVolta->setSubtype(4);
                                          lastVolta = 0;
                                          }
                                    else
                                          printf("ImportXml: warning: unsupported ending type <%s>\n",
                                                  endingType.toLatin1().data());
                                    }
                              }
                        }
                  }
            else if (e.tagName() == "sound")
                  domNotImplemented(e);
            else if (e.tagName() == "harmony")
                  domNotImplemented(e);
            else
                  domError(e);
            }
      staves         = part->nstaves();
      int measureLen = maxtick - measure->tick();

      if (lastMeasureLen != measureLen) {
            SigList* sigmap = score->sigmap;
            int tick        = measure->tick();
            SigEvent se = sigmap->timesig(tick);

            if (measureLen != sigmap->ticksMeasure(tick)) {
                  SigEvent se = sigmap->timesig(tick);

// printf("Add Sig %d  len %d  %d / %d\n", tick, measureLen, z, n);
                  score->sigmap->add(tick, measureLen, se.nominator2, se.denominator2);
                  int tm = ticks_measure(se.nominator, se.denominator);
                  if (tm != measureLen) {
                        if (!measure->irregular()) {
                            /* MusicXML's implicit means "don't print measure number",
                              set it only if explicitly requested, not when the measure length
                              is not what is expected. See MozartTrio.xml measures 12..13.
                            */
                            //  measure->setIrregular(true);
                            /*
                              printf("irregular Measure %d Len %d at %d   lastLen: %d -> should be: %d (tm=%d)\n",
                                 number, measure->tick(), maxtick,
                                 lastMeasureLen, measureLen, tm);
                              */
                              }
                        }
                  }
            }
      lastMeasureLen = measureLen;
      tick = maxtick;
      return measure;
      }

//---------------------------------------------------------
//   direction
//---------------------------------------------------------

/**
 Read the MusicXML direction element.
 */

// LVI FIXME: introduce offset concept to mscore.
// offset changes only the print position (not the tick), but unlike relative-x
// it is expressed in terms of divisions (MusicXML direction.dtd)
// even though the DTD does not mention it, practically speaking
// offset and relative-x are mutually exclusive

void MusicXml::direction(Measure* measure, int staff, QDomElement e)
      {
      QString placement = e.attribute("placement");

      QString dirType;
      QString type;
      QString txt;
      QString lang;
      QString weight;
      int offset = 0;
      int rstaff = 0;
      QStringList dynamics;
      int spread = 0;
      qreal rx = 0.0;
      qreal ry = 0.0;
      qreal yoffset = 0.0;
      qreal xoffset = 0.0;
      qreal size = score->textStyle(TEXT_STYLE_TECHNIK)->size;
      QString tempo = "";

      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "direction-type") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        dirType = ee.tagName();
                        //
                        // TODO: whats the difference between relative-x and default-x
                        //       in handling?
                        //
                        ry      = ee.attribute(QString("relative-y"), "0").toDouble() * -.1;
                        rx      = ee.attribute(QString("relative-x"), "0").toDouble() * .1;
                        yoffset = ee.attribute("default-y", "0.0").toDouble() * -0.1;
                        xoffset = ee.attribute("default-x", "0.0").toDouble() * 0.1;
                        if (dirType == "words") {
                              txt    = ee.text();
                              lang   = ee.attribute(QString("xml:lang"), "it");
                              weight = ee.attribute(QString("font-weight"));
                              if (ee.hasAttribute("font-size"))
                                    size = ee.attribute("font-size").toDouble();
                              }
                        else if (dirType == "pedal") {
                              type = ee.attribute(QString("type"));
                              }
                        else if (dirType == "dynamics") {
                              QDomElement eee = ee.firstChildElement();
                              if (!eee.isNull()) {
                                    if (eee.tagName() == "other-dynamics")
                                          dynamics.push_back(eee.text());
                                    else
                                          dynamics.push_back(eee.tagName());
                                    }
                              }
                        else if (dirType == "wedge") {
                              type   = ee.attribute(QString("type"));
                              spread = ee.attribute(QString("spread"), "0").toInt();
                              }
                        else if (dirType == "dashes")
                              domNotImplemented(ee);
                        else if (dirType == "bracket")
                              domNotImplemented(ee);
                        else if (dirType == "metronome")
                              domNotImplemented(ee);
                        else if (dirType == "octave-shift")
                              domNotImplemented(ee);
                        else if (dirType == "segno")
                              domNotImplemented(ee);
                        else
                              domError(ee);
                        }
                  }
            else if (e.tagName() == "sound") {
                  // attr: dynamics, tempo
                  tempo = e.attribute("tempo");
                  }
            else if (e.tagName() == "offset")
                  offset = (e.text().toInt() * ::division)/divisions;
            else if (e.tagName() == "staff") {
                  // DEBUG: <staff>0</staff>
                  rstaff = e.text().toInt() - 1;
                  if (rstaff < 0)         // ???
                        rstaff = 0;
                  }
            else
                  domError(e);
            }
      if (placement == "above")
            ry -= 2;
      else
            ry += 2;

      if (dirType == "words") {
            // LVIFIX: tempotext font is incorrect
            // printf("words txt='%s' tempo='%s'\n", txt.toLatin1().data(), tempo.toLatin1().data());
            Text* t;
            if (tempo != "") {
                  t = new TempoText(score);
                  ((TempoText*) t)->setTempo(tempo.toDouble());
                  t->setStyle(TEXT_STYLE_TEMPO);
                  }
            else
                  t = new Text(score);
                  t->setStyle(TEXT_STYLE_TECHNIK);
            t->setTick(tick);
            if (weight == "bold") {
                  // Text text(txt, TEXT_STYLE_TECHNIK, true, size);
                  t->setText(txt);
                  }
            else
                  t->setText(txt);
            if (placement == "above")
                  ry -= 3;
            else
                  ry += t->bbox().height()/_spatium - 2.3;

            t->setUserOff(QPointF(rx + xoffset, ry + yoffset));
            t->setMxmlOff(offset);
            t->setStaff(score->staff(staff + rstaff));
            measure->add(t);
            }
      else if (dirType == "pedal") {
            Symbol* s = new Symbol(score);
            s->setTick(tick);
            if (type == "start")
                  s->setSym(pedalPedSym);
            else if (type == "stop")
                  s->setSym(pedalasteriskSym);
            else
                  printf("unknown pedal %s\n", type.toLatin1().data());
            if (placement == "above")
                  ry += 0.1;
            else
                  ry += 5.3;
            s->setUserOff(QPointF(rx + xoffset, ry + yoffset));
            s->setMxmlOff(offset);
            s->setStaff(score->staff(staff + rstaff));
            measure->add(s);
            }
      else if (dirType == "dynamics") {
            // more than one dynamic ???
            // LVIFIX: check import/export of <other-dynamics>unknown_text</...>
            for (QStringList::Iterator it = dynamics.begin(); it != dynamics.end(); ++it ) {
                  Dynamic* dyn = new Dynamic(score);
                  dyn->setSubtype(*it);
                  if (placement == "above")
                        ry -= 5;
                  else
                        ry += dyn->bbox().height() / _spatium - 7.5;
                  dyn->setUserOff(QPointF(rx + xoffset, ry + yoffset));
                  dyn->setMxmlOff(offset);

                  dyn->setStaff(score->staff(staff + rstaff));
                  dyn->setTick(tick);
                  measure->add(dyn);
                  }
            }
      else if (dirType == "wedge") {
            if (placement == "above")
                  ry -= 7;
            else
                  ry -= 1.7;
            if (type == "crescendo")
                  addWedge(0, tick, rx, ry, 0);
            else if (type == "stop")
                  genWedge(0, tick, measure, staff+rstaff);
            else if (type == "diminuendo")
                  addWedge(0, tick, rx, ry, 1);
            else
                  printf("unknown wedge type: %s\n", type.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   xmlAttributes
//---------------------------------------------------------

/**
 Read the MusicXML attributes element.
 */

// Standard order of attributes as written by Dolet for Finale is divisions,
// key, time, staves and clef(s). For the first measure this means number of
// staves is unknown when the time attributes is read. As this translates
// into a time signature that must be inserted into every staff of the
// part, delay insertion of time signatures until after all attributes
// have been read.

void MusicXml::xmlAttributes(Measure* measure, int staff, QDomElement e)
      {
      QString beats = "";
      QString beatType = "";
      QString timeSymbol = "";

      for (;!e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "divisions")
                  divisions = e.text().toInt();
            else if (e.tagName() == "key") {
                  int number  = e.attribute(QString("number"), "-1").toInt();
                  int staffIdx = staff;
                  if (number != -1)
                        staffIdx += number - 1;
                  int key = 0;
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "fifths")
                              key = ee.text().toInt();
                        else if (ee.tagName() == "mode")
                              domNotImplemented(ee);
                        else
                              domError(ee);
                        }
                  if (number == -1) {
                        //
                        //   apply key to all staves in the part
                        //
                        int staves = score->part(staff)->nstaves();
                        for (int i = 0; i < staves; ++i) {
                              int oldkey = score->staff(staffIdx+i)->keymap()->key(tick);
                              if (oldkey != key) {
                                    // new key differs from key in effect at this tick
                                    (*score->staff(staffIdx+i)->keymap())[tick] = key;
                                    // dont generate symbol for tick 0
                                    if (tick) {
                                          // apply to all staves in part
                                          KeySig* keysig = new KeySig(score);
                                          keysig->setTick(tick);
                                          keysig->setStaff(score->staff(staffIdx + i));
                                          keysig->setSubtype(key);
                                          Segment* s = measure->getSegment(keysig);
                                          s->add(keysig);
                                          }
                                    }
                              }
                        }
                  else {
                        //
                        //    apply key to staff(staffIdx) only
                        //
                        int oldkey = score->staff(staffIdx)->keymap()->key(tick);
                        if (oldkey != key) {
                              // new key differs from key in effect at this tick
                              (*score->staff(staffIdx)->keymap())[tick] = key;
                              // dont generate symbol for tick 0
                              if (tick) {
                                    KeySig* keysig = new KeySig(score);
                                    keysig->setTick(tick);
                                    keysig->setStaff(score->staff(staffIdx));
                                    keysig->setSubtype(key);
                                    Segment* s = measure->getSegment(keysig);
                                    s->add(keysig);
                                    }
                              }
                        }
                  }
            else if (e.tagName() == "time") {
                  timeSymbol = e.attribute("symbol");
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "beats")
                              beats = ee.text();
                        else if (ee.tagName() == "beat-type") {
                              beatType = ee.text();
                              }
                        else
                              domError(ee);
                        }
                  }
            else if (e.tagName() == "clef") {
                  int clef   = 0;
                  int clefno = e.attribute(QString("number"), "1").toInt() - 1;
                  QString c;
                  int i = 0;
                  int line = -1;
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "sign")
                              c = ee.text();
                        else if (ee.tagName() == "line")
                              line = ee.text().toInt();
                        else if (ee.tagName() == "clef-octave-change") {
                              i = ee.text().toInt();
                              if (i && !(c == "F" || c == "G"))
                                    printf("clef-octave-change only implemented for F and G key\n");
                              }
                        else
                              domError(ee);
                        }
                  if (c == "G" && i == 0)
                        clef = 0;
                  else if (c == "G" && i == 1)
                        clef = 1;
                  else if (c == "G" && i == 2)
                        clef = 2;
                  else if (c == "G" && i == -1)
                        clef = 3;
                  else if (c == "F" && i == 0)
                        clef = 4;
                  else if (c == "F" && i == -1)
                        clef = 5;
                  else if (c == "F" && i == -2)
                        clef = 6;
                  else if (c == "C") {
                        if (line == 4)
                              clef = CLEF_C4;
                        else if (line == 3)
                              clef = CLEF_C3;
                        else if (line == 2)
                              clef = CLEF_C2;
                        else if (line == 1)
                              clef = CLEF_C1;
                        }
                  Staff* part = score->staff(staff + clefno);
                  ClefList* ct = part->clef();
                  (*ct)[tick] = clef;
                  if (tick) {
                        // dont generate symbol for tick 0
                        Clef* clefs = new Clef(score, clef | clefSmallBit);
                        clefs->setTick(tick);
                        clefs->setStaff(part);
                        Segment* s = measure->getSegment(clefs);
                        s->add(clefs);
                        ++clefno;
                        }
                  }
            else if (e.tagName() == "staves") {
                  int staves = e.text().toInt();
                  Part* part = score->part(staff);
                  part->setStaves(staves);
                  Staff* staff = part->staff(0);
                  if (staff && staves == 2) {
                        staff->setBracket(0, BRACKET_AKKOLADE);
                        staff->setBracketSpan(0, 2);
                        }
                  }
            else if (e.tagName() == "staff-details")
                  domNotImplemented(e);
            else if (e.tagName() == "instruments")
                  domNotImplemented(e);
            else if (e.tagName() == "transpose")
                  domNotImplemented(e);
            else
                  domError(e);
            }
      if (beats != "" && beatType != "") {
            // determine if timesig is valid
            int st = 0;  // timesig subtype calculated
            int bts[4];  // the beats (max 4 separated by "+") as integer
            int btp = 0; // beat-type as integer
            if (beats == "2" && beatType == "2" && timeSymbol == "cut") {
                  st = TSIG_ALLA_BREVE;
                  }
            else if (beats == "4" && beatType == "4" && timeSymbol == "common") {
                  st = TSIG_FOUR_FOUR;
                  }
            else if (timeSymbol == "") {
                  btp = beatType.toInt();
                  QStringList list = beats.split("+");
                  for (int i = 0; i < 4; i++) bts[i] = 0;
                  for (int i = 0; i < list.size() && i < 4; i++) {
                        bts[i] = list.at(i).toInt();
                        }
                  // the beat type and at least one beat must be non-zero
                  if (btp && (bts[0] || bts[1] || bts[2] || bts[3])) {
                        TimeSig ts = TimeSig(score, btp, bts[0], bts[1], bts[2], bts[3]);
                        st = ts.subtype();
                        }
                  }
            if (st) {
                  // add timesig to all staves
                  int n;
                  int z;
                  TimeSig::getSig(st, &n, &z);
                  score->sigmap->add(tick, z, n);
                  Part* part = score->part(staff);
                  int staves = part->nstaves();
                  for (int i = 0; i < staves; ++i) {
                        TimeSig* timesig = new TimeSig(score);
                        timesig->setTick(tick);
                        timesig->setSubtype(st);
                        timesig->setStaff(score->staff(staff + i));
                        Segment* s = measure->getSegment(timesig);
                        s->add(timesig);
                        }
                  }
            else
                  printf("unknown time signature, beats=<%s> beat-type=<%s> symbol=<%s>\n",
                         beats.toLatin1().data(), beatType.toLatin1().data(),
                         timeSymbol.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   xmlLyric
//---------------------------------------------------------

void MusicXml::xmlLyric(Measure* measure, int staff, QDomElement e)
      {
      int lyricNo = e.attribute(QString("number"), "1").toInt() - 1;
      if (lyricNo > MAX_LYRICS)
            printf("too much lyrics (>%d)\n", MAX_LYRICS);
      Lyrics* l = new Lyrics(score);
      l->setNo(lyricNo);
      l->setTick(tick);

      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "syllabic") {
                  if (e.text() == "single")
                        l->setSyllabic(Lyrics::SINGLE);
                  else if (e.text() == "begin")
                        l->setSyllabic(Lyrics::BEGIN);
                  else if (e.text() == "end")
                        l->setSyllabic(Lyrics::END);
                  else if (e.text() == "middle")
                        l->setSyllabic(Lyrics::MIDDLE);
                  else
                        printf("unknown syllabic %s\n", qPrintable(e.text()));
                  }
            else if (e.tagName() == "text")
                  l->setText(e.text());
            else if (e.tagName() == "extend")
                  ;
            else if (e.tagName() == "end-line")
                  ;
            else if (e.tagName() == "end-paragraph")
                  ;
            else
                  domError(e);
            }
      l->setStaff(score->staff(staff));
      Segment* segment = measure->getSegment(l);
      segment->add(l);
      }

//---------------------------------------------------------
//   xmlNote
//---------------------------------------------------------

/**
 Read a MusicXML note.

 \a Staff is the number of first staff of the part this note belongs to.
 */

void MusicXml::xmlNote(Measure* measure, int staff, QDomElement e)
      {
      voice = 0;
      move  = 0;

      int duration = 0;
      bool rest    = false;
      int relStaff = 0;
      BeamMode bm  = BEAM_AUTO;
      Direction sd = AUTO;
      int dots     = 0;
      bool grace   = false;
      QString fermataType;
      QString tupletType;
      QString tupletPlacement;
      QString tupletBracket;
      QString step;
      QString fingering;
      QString graceSlash;
      int alter  = 0;
      int octave = 4;
      int accidental = 0;
      DurationType durationType = D_QUARTER;
      bool trillMark = false;
      QString strongAccentType;
      bool accent = false;
      bool staccatissimo = false;
      bool staccato = false;
      bool tenuto = false;
      bool turn = false;
      bool mordent = false;
      bool invertedMordent = false;
      bool invertedTurn = false;
      bool stopped = false;
      bool upbow = false;
      bool downbow = false;
      int actualNotes = 1;
      int normalNotes = 1;
      int tremolo = 0;

      for (; !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            QString s(e.text());

            if (tag == "pitch") {
                  step   = "C";
                  alter  = 0;
                  octave = 4;
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "step")          // A-G
                              step = ee.text();
                        else if (ee.tagName() == "alter")    // -1=flat 1=sharp (0.5=quarter sharp)
                              alter = ee.text().toInt();
                        else if (ee.tagName() == "octave")   // 0-9 4=middle C
                              octave = ee.text().toInt();
                        else
                              domError(ee);
                        }
                  }
            else if (tag == "duration")
                  duration = s.toInt();
            else if (tag == "type") {
                  if (s == "quarter")
                        durationType = D_QUARTER;
                  else if (s == "eighth")
                        durationType = D_EIGHT;
                  else if (s == "256th")
                        durationType = D_256TH;
                  else if (s == "128th")
                        durationType = D_128TH;
                  else if (s == "64th")
                        durationType = D_64TH;
                  else if (s == "32nd")
                        durationType = D_32ND;
                  else if (s == "16th")
                        durationType = D_16TH;
                  else if (s == "half")
                        durationType = D_HALF;
                  else if (s == "whole")
                        durationType = D_WHOLE;
                  else if (s == "breve")
                        durationType = D_BREVE;
                  else if (s == "long")
                        durationType = D_LONG;
                  else
                        printf("unknown note type <%s>\n", s.toLatin1().data());
                  }
            else if (tag == "chord")
                  tick -= lastLen;
            else if (tag == "voice")
                  voice = s.toInt() - 1;
            else if (tag == "stem") {
                  if (s == "up")
                        sd = UP;
                  else if (s == "down")
                        sd = DOWN;
                  else if (s == "none")  // ?
                        ;
                  else if (s == "double")
                        ;
                  else
                        printf("unknown stem direction %s\n", e.text().toLatin1().data());
                  }
            else if (tag == "staff") {
                  relStaff = s.toInt() - 1;
                  //
                  // Musicxml voices are counted for all staffs of an
                  // instrument. They are not limited. In mscore voices are associated
                  // with a staff. Every staff can have at most VOICES voices.
                  // The following lines map musicXml voices to mscore voices.
                  // If a voice crosses two staffs, this is expressed with the
                  // "move" parameter in mscore.
                  //
                  // Musicxml voices are unique within a part, but not across parts.
                  // LVIFIX: check: Thus the search for a given MusicXML voice number should be restricted
                  // the the staves of the part it belongs to.

//                  printf("voice mapper before: relStaff=%d voice=%d", relStaff, voice);
                  int found = false;
                  for (int s = 0; s < MAX_STAVES; ++s) {
                        int v = 0;
                        for (std::vector<int>::iterator i = voicelist[s].begin(); i != voicelist[s].end(); ++i, ++v) {
                              if (*i == voice) {
                                    int d = relStaff + staff - s;
                                    relStaff -= d;
                                    move += d;
                                    voice = v;
//                                    printf(" found at s=%d", s);
                                    found = true;
                                    break;
                                    }
                              }
                        }
                  if (!found) {
                        if (voicelist[staff+relStaff].size() >= unsigned(VOICES))
                              printf("ImportMusicXml: too many voices (> %d)\n", VOICES);
                        else {
                              voicelist[staff+relStaff].push_back(voice);
//                              printf(" append %d to voicelist[%d]", voice, staff+relStaff);
                              voice = voicelist[staff+relStaff].size() -1;
                              }
                        }
//                  printf(" after: relStaff=%d move=%d voice=%d\n", relStaff, move, voice);
                  }
            else if (tag == "beam") {
                  if (s == "begin") {
                        bm = BEAM_BEGIN;
                        }
                  else if (s == "end") {
                        bm = BEAM_END;
                        }
                  else if (s == "continue")
                        bm = BEAM_MID;
                  else if (s == "backward hook")
                        ;
                  else if (s == "forward hook")
                        ;
                  else
                        printf("unknown beam keyword <%s>\n", s.toLatin1().data());
                  }
            else if (tag == "rest")
                  rest = true;
            else if (tag == "lyric")
                  xmlLyric(measure, relStaff + staff, e);
            else if (tag == "dot")
                  ++dots;
            else if (tag == "accidental") {
                  QString editorial = e.attribute(QString("editorial"));
                  if (s == "natural")
                        accidental = 5;
                  else if (s == "flat")
                        accidental = 2;
                  else if (s == "sharp")
                        accidental = 1;
                  else if (s == "double-sharp")
                        accidental = 3;
                  else if (s == "sharp-sharp")
                        accidental = 3;
                  else if (s == "natural-flat")
                        ;
                  else if (s == "quarter-flat")
                        ;
                  else if (s == "quarter-sharp")
                        ;
                  else if (s == "three-quarters-flat")
                        ;
                  else if (s == "three-quarters-sharp")
                        ;
                  else if (s == "flat-flat")
                        accidental = 4;
                  else if (s == "natural-sharp")
                        ;
                  else
                        printf("unknown accidental %s\n", s.toLatin1().data());
                  if (1 <= accidental && accidental <= 5 && editorial == "yes")
                        accidental += 5;
                  }
            else if (tag == "notations") {
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "slur") {
                              int slurNo   = ee.attribute(QString("number"), "1").toInt() - 1;
                              QString slurType = ee.attribute(QString("type"));

                              int trk = (staff + relStaff) * VOICES;
                              if (slurType == "start") {
                                    bool endSlur = false;
                                    if (slur[slurNo] == 0)
                                          slur[slurNo] = new Slur(score);
                                    else
                                          endSlur = true;
                                    QString pl = ee.attribute(QString("placement"));
                                    if (pl == "above")
                                          slur[slurNo]->setSlurDirection(UP);
                                    else if (pl == "below")
                                          slur[slurNo]->setSlurDirection(DOWN);
                                    slur[slurNo]->setStart(tick, trk + voice);
                                    slur[slurNo]->setStaff(score->staff(staff + relStaff));
                                    slur[slurNo]->setParent(score->mainLayout());
                                    score->addElement(slur[slurNo]);
                                    if (endSlur)
                                          slur[slurNo] = 0;
                                    }
                              else if (slurType == "stop") {
                                    if (slur[slurNo] == 0) {
                                          slur[slurNo] = new Slur(score);
                                          slur[slurNo]->setEnd(tick, trk + voice);
                                          }
                                    else {
                                          slur[slurNo]->setEnd(tick, trk + voice);
                                          slur[slurNo] = 0;
                                          }
                                    }
                              else
                                    printf("unknown slur type %s\n", slurType.toLatin1().data());
                              }
                        else if (ee.tagName() == "tied") {
                              QString tiedType = ee.attribute(QString("type"));
                              if (tiedType == "start") {
                                    if (tie) {
                                          printf("Tie already active\n");
                                          }
                                    else {
                                          tie = new Tie(score);
                                          }
                                    QString tiedOrientation = e.attribute("orientation", "auto");
                                    if (tiedOrientation == "over")
                                          tie->setSlurDirection(UP);
                                    else if (tiedOrientation == "under")
                                          tie->setSlurDirection(DOWN);
                                    else if (tiedOrientation == "auto")
                                          ;
                                    else
                                          printf("unknown tied orientation: %s\n", tiedOrientation.toLatin1().data());
                                          ;
                                    }
                              else if (tiedType == "stop")
                                    ;
                              else
                                    printf("unknown tied type %s\n", tiedType.toLatin1().data());
                              }
                        else if (ee.tagName() == "tuplet") {
                              tupletType      = ee.attribute(QString("type"));
                              tupletPlacement = ee.attribute("placement");
                              tupletBracket   = ee.attribute("bracket");
                              }
                        else if (ee.tagName() == "dynamics") {
                              // int rx            = ee.attribute("relative-x").toInt();
                              QString placement = ee.attribute("placement");
                              }
                        else if (ee.tagName() == "articulations") {
                              for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                    if (eee.tagName() == "accent")
                                          accent = true;
                                    else if (eee.tagName() == "staccatissimo")
                                          staccatissimo = true;
                                    else if (eee.tagName() == "staccato")
                                          staccato = true;
                                    else if (eee.tagName() == "strong-accent")
                                          strongAccentType = eee.attribute(QString("type"));
                                    else if (eee.tagName() == "tenuto")
                                          tenuto = true;
                                    else
                                          domError(eee);
                                    }
                              }
                        else if (ee.tagName() == "fermata") {
                              fermataType = ee.attribute(QString("type"));
                              }
                        else if (ee.tagName() == "ornaments") {
					//	<trill-mark placement="above"/>
                              for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                    if (eee.tagName() == "trill-mark")
                                          trillMark = true;
                                    else if (eee.tagName() == "inverted-turn")
                                          invertedTurn = true;
                                    else if (eee.tagName() == "turn")
                                          turn = true;
                                    else if (eee.tagName() == "inverted-mordent")
                                          invertedMordent = true;
                                    else if (eee.tagName() == "mordent")
                                          mordent = true;
                                    else if (eee.tagName() == "tremolo")
                                          tremolo = eee.text().toInt();
                                    else if (eee.tagName() == "accidental-mark")
                                          domNotImplemented(eee);
                                    else if (eee.tagName() == "delayed-turn")
                                          domNotImplemented(eee);
                                    else
                                          domError(eee);
                                    }
                              }
                        else if (ee.tagName() == "technical") {
                              for (QDomElement eee = ee.firstChildElement(); !eee.isNull(); eee = eee.nextSiblingElement()) {
                                    if (eee.tagName() == "fingering")
                                          fingering = eee.text();
                                    else if (eee.tagName() == "fret")
                                          domNotImplemented(eee);
                                    else if (eee.tagName() == "string")
                                          domNotImplemented(eee);
                                    else if (eee.tagName() == "pull-off")
                                          domNotImplemented(eee);
                                    else if (eee.tagName() == "stopped")
                                          stopped = true;
                                    else if (eee.tagName() == "up-bow")
                                          upbow = true;
                                    else if (eee.tagName() == "down-bow")
                                          downbow = true;
                                    else
                                          domError(eee);
                                    }
                              }
                        else if (ee.tagName() == "arpeggiate")
                              domNotImplemented(ee);
                        else
                              domError(ee);
                        }
                  }
            else if (tag == "tie") {
                  QString tieType = e.attribute(QString("type"));
                  if (tieType == "start")
                        ;
                  else if (tieType == "stop")
                        ;
                  else
                        printf("unknown tie type %s\n", tieType.toLatin1().data());
                  }
            else if (tag == "grace") {
                  grace = true;
                  graceSlash = e.attribute(QString("slash"));
                  }
            else if (tag == "time-modification") {  // tuplets
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        if (ee.tagName() == "actual-notes")
                              actualNotes = ee.text().toInt();
                        else if (ee.tagName() == "normal-notes")
                              normalNotes = ee.text().toInt();
                        else if (ee.tagName() == "normal-type")
                              domNotImplemented(ee);
                        else
                              domError(ee);
                        }
                  }
            else if (tag == "notehead")
                  domNotImplemented(e);
            else if (tag == "instrument")
                  domNotImplemented(e);
            else if (tag == "cue")
                  domNotImplemented(e);
            else
                  domError(e);
            }
      int ticks = (::division * duration) / divisions;

      if (tick + ticks > maxtick)
            maxtick = tick + ticks;

//      printf("%s at %d voice %d dur = %d, beat %d/%d div %d pitch %d ticks %d\n",
//         rest ? "Rest" : "Note", tick, voice, duration, beats, beatType,
//         divisions, 0 /* pitch */, ticks);

      ChordRest* cr = 0;

      if (rest) {
            cr = new Rest(score, tick, ticks);
            // TODO: try to find out if this rest is part of a beam
            cr->setBeamMode(BEAM_NO);
//            cr->setBeamMode(BEAM_AUTO);
            cr->setVoice(voice);
            cr->setStaff(score->staff(staff + relStaff));
            ((Rest*)cr)->setMove(move);
            Segment* s = measure->getSegment(cr);
            s->add(cr);
            }
      else {
            char c     = step[0].toLatin1();
            Note* note = new Note(score);

            // xmlSetPitch(note, tick, c, alter, octave, accidental);

            note->setType(durationType);
            note->setDots(dots);
            note->setVoice(voice);
            note->setMove(move);

            if (!fingering.isEmpty()) {
                  Text* f = new Text(score);
                  f->setSubtype(TEXT_FINGERING);
                  f->setText(fingering);
                  note->add(f);
                  }

            if (tie) {
                  note->setTieFor(tie);
                  tie->setStartNote(note);
                  tie->setStaff(score->staff(staff + relStaff));
                  tie = 0;
                  }

            cr = measure->findChord(tick, staff + relStaff, voice, grace);
            if (cr == 0) {
                  cr = new Chord(score);
                  cr->setTick(tick);
                  cr->setVoice(voice);
                  cr->setTickLen(ticks);
                  ((Chord*)cr)->setGrace(grace);
                  cr->setBeamMode(bm);
                  cr->setStaff(score->staff(staff + relStaff));
                  Segment* s = measure->getSegment(cr);
                  s->add(cr);
                  }
            cr->add(note);
            if (grace)
                  cr->setSmall(grace);
//            printf("staff for new note: %p (staff=%d, relStaff=%d)\n",
//                   score->staff(staff + relStaff), staff, relStaff);
            xmlSetPitch(note, tick, c, alter, octave, accidental);

            if (cr->beamMode() == BEAM_NO)
                  cr->setBeamMode(bm);
            ((Chord*)cr)->setStemDirection(sd);
            }
      if (!fermataType.isEmpty()) {
            NoteAttribute* f = new NoteAttribute(score);
            if (fermataType == "upright") {
                  f->setSubtype(UfermataSym);
                  cr->add(f);
                  }
            else if (fermataType == "inverted") {
                  f->setSubtype(DfermataSym);
                  f->setUserYoffset(5.3); // force below note (albeit by brute force)
                  cr->add(f);
                  }
            else {
                  printf("unknown fermata type %s\n", fermataType.toLatin1().data());
                  delete f;
                  }
            }
      if (!strongAccentType.isEmpty()) {
            NoteAttribute* na = new NoteAttribute(score);
            if (strongAccentType == "up") {
                  na->setSubtype(UmarcatoSym);
                  cr->add(na);
                  }
            else if (strongAccentType == "down") {
                  na->setSubtype(DmarcatoSym);
//                  f->setUserYoffset(5.3); // force below note (albeit by brute force)
                  cr->add(na);
                  }
            else {
                  printf("unknown mercato type %s\n", strongAccentType.toLatin1().data());
                  delete na;
                  }
            }
      if (trillMark) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(TrillSym);
            cr->add(na);
            }
      if (invertedTurn) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(ReverseturnSym);
            cr->add(na);
            }
      if (turn) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(TurnSym);
            cr->add(na);
            }
      if (mordent) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(MordentSym);
            cr->add(na);
            }
      if (invertedMordent) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(PrallSym);
            cr->add(na);
            }
      if (accent) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(SforzatoaccentSym);
            cr->add(na);
            }
      if (staccatissimo) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(UstaccatissimoSym);
            cr->add(na);
            }
      if (staccato) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(StaccatoSym);
            cr->add(na);
            }
      if (tenuto) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(TenutoSym);
            cr->add(na);
            }
      if (stopped) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(PlusstopSym);
            cr->add(na);
            }
      if (upbow) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(UpbowSym);
            cr->add(na);
            }
      if (downbow) {
            NoteAttribute* na = new NoteAttribute(score);
            na->setSubtype(DownbowSym);
            cr->add(na);
            }
      if (!tupletType.isEmpty()) {
            if (tupletType == "start") {
                  tuplet = new Tuplet(score);
                  tuplet->setStaff(score->staff(staff + relStaff));
                  tuplet->setNormalNotes(normalNotes);
                  tuplet->setActualNotes(actualNotes);
                  tuplet->setBaseLen(cr->tickLen() * actualNotes / normalNotes);

                  // type, placement

                  measure->add(tuplet);
//                  cr->setTuplet(tuplet);
//                  tuplet->add(cr);
                  }
            else if (tupletType == "stop") {
                  cr->setTuplet(tuplet);
                  tuplet->add(cr);
                  tuplet = 0;
                  }
            else
                  printf("unknown tuplet type %s\n", tupletType.toLatin1().data());
            }
      if (tuplet) {
            cr->setTuplet(tuplet);
            tuplet->add(cr);
            }
      if (tremolo) {
            if (tremolo == 1 || tremolo == 2 || tremolo == 3) {
                  Tremolo * t = new Tremolo(score);
                  if (tremolo == 1) t->setSubtype(TREMOLO_1);
                  if (tremolo == 2) t->setSubtype(TREMOLO_2);
                  if (tremolo == 3) t->setSubtype(TREMOLO_3);
                  cr->add(t);
                  }
            else
                  printf("unknown tremolo type %d\n", tremolo);
            }

      lastLen = ticks;
      tick += ticks;
      }

//---------------------------------------------------------
//   addWedge
//---------------------------------------------------------

/**
 Add a MusicXML wedge to the wedge list.

 Called when the wedge start is read. Stores all wedge parameters known at this time.
 */

void MusicXml::addWedge(int no, int startTick, qreal rx, qreal ry, int subType)
      {
      MusicXmlWedge wedge;
      wedge.number = no;
      wedge.startTick = startTick;
      wedge.rx = rx;
      wedge.ry = ry;
      wedge.subType = subType;

      if (int(wedgeList.size()) > no)
            wedgeList[no] = wedge;
      else
            wedgeList.push_back(wedge);
      }

//---------------------------------------------------------
//   genWedge
//---------------------------------------------------------

/**
 Add a MusicXML wedge to the score.

 Called when the wedge stop is read. Wedge stop tick was unknown until this time.
 */

void MusicXml::genWedge(int no, int endTick, Measure* /*measure*/, int staff)
      {
      Hairpin* hp = new Hairpin(score);

      hp->setTick(wedgeList[no].startTick);
      hp->setTick2(endTick);
      hp->setSubtype(wedgeList[no].subType);
      hp->setUserOff(QPointF(wedgeList[no].rx, wedgeList[no].ry));
      hp->setStaff(score->staff(staff));
      score->mainLayout()->add(hp);
//      printf("gen wedge staff %d\n", staff);
      }

