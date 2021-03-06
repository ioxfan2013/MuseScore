//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id$
//
//  Copyright (C) 2008-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "score.h"
#include "stafftext.h"
#include "system.h"
#include "staff.h"

//---------------------------------------------------------
//   StaffText
//---------------------------------------------------------

StaffText::StaffText(Score* s)
   : Text(s)
      {
      setSubtype(TEXT_STAFF);
      setTextStyle(TEXT_STYLE_STAFF);
      _setAeolusStops = false;
      clearAeolusStops();
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StaffText::write(Xml& xml) const
      {
      xml.stag("StaffText");
      foreach(ChannelActions s, _channelActions) {
            int channel = s.channel;
            foreach(QString name, s.midiActionNames)
                  xml.tagE(QString("MidiAction channel=\"%1\" name=\"%2\"").arg(channel).arg(name));
            }
      for (int voice = 0; voice < VOICES; ++voice) {
            if (!_channelNames[voice].isEmpty())
                  xml.tagE(QString("channelSwitch voice=\"%1\" name=\"%2\"").arg(voice).arg(_channelNames[voice]));
            }
      if (_setAeolusStops) {
            for (int i = 0; i < 4; ++i)
                  xml.tag(QString("aeolus group=\"%1\"").arg(i), aeolusStops[i]);
            }
      Text::writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StaffText::read(QDomElement e)
      {
      for (int voice = 0; voice < VOICES; ++voice)
            _channelNames[voice].clear();
      clearAeolusStops();
      for (e = e.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            QString tag(e.tagName());
            if (tag == "MidiAction") {
                  int channel = e.attribute("channel", 0).toInt();
                  QString name = e.attribute("name");
                  bool found = false;
                  int n = _channelActions.size();
                  for (int i = 0; i < n; ++i) {
                        ChannelActions* a = &_channelActions[i];
                        if (a->channel == channel) {
                              a->midiActionNames.append(name);
                              found = true;
                              break;
                              }
                        }
                  if (!found) {
                        ChannelActions a;
                        a.channel = channel;
                        a.midiActionNames.append(name);
                        _channelActions.append(a);
                        }
                  }
            else if (tag == "channelSwitch" || tag == "articulationChange") {
                  int voice = e.attribute("voice", "-1").toInt();
                  if (voice >= 0 && voice < VOICES)
                        _channelNames[voice] = e.attribute("name");
                  else if (voice == -1) {
                        // no voice applies channel to all voices for
                        // compatibility
                        for (int i = 0; i < VOICES; ++i)
                              _channelNames[i] = e.attribute("name");
                        }
                  }
            else if (tag == "aeolus") {
                  int group = e.attribute("group", "-1").toInt();
                  if (group >= 0 && group <= 4) {
                        aeolusStops[group] = e.text().toInt();
                        }
                  _setAeolusStops = true;
                  }
            else if (!Text::readProperties(e))
                  domError(e);
            }
      cursorPos = 0;
      }

//---------------------------------------------------------
//   clearAeolusStops
//---------------------------------------------------------

void StaffText::clearAeolusStops()
      {
      for (int i = 0; i < 4; ++i)
            aeolusStops[i] = 0;
      }

//---------------------------------------------------------
//   setAeolusStop
//---------------------------------------------------------

void StaffText::setAeolusStop(int group, int idx, bool val)
      {
      if (val)
            aeolusStops[group] |= (1 << idx);
      else
            aeolusStops[group] &= ~(1 << idx);
      }

//---------------------------------------------------------
//   getAeolusStop
//---------------------------------------------------------

bool StaffText::getAeolusStop(int group, int idx) const
      {
      return aeolusStops[group] & (1 << idx);
      }

