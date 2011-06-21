//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "input.h"
#include "segment.h"
#include "part.h"
#include "staff.h"
#include "score.h"
#include "chordrest.h"

class DrumSet;

//---------------------------------------------------------
//   InputState
//---------------------------------------------------------

InputState::InputState() :
   _duration(Duration::V_INVALID),
   _drumNote(-1),
   _drumset(0),
   _track(0),
   _segment(0),
   _repitchMode(false),
   rest(false),
   pad(0),
   pitch(72),
   noteType(NOTE_NORMAL),
   beamMode(BEAM_AUTO),
   noteEntryMode(false),
   slur(0)
      {
      }

//---------------------------------------------------------
//   drumset
//---------------------------------------------------------

Drumset* InputState::drumset() const
      {
      if (_segment == 0)
            return 0;
      return _segment->score()->staff(_track/VOICES)->part()->instr(_segment->tick())->drumset();
      }

//---------------------------------------------------------
//   tick
//---------------------------------------------------------

int InputState::tick() const
      {
      return _segment ? _segment->tick() : 0;
      }

//---------------------------------------------------------
//   cr
//---------------------------------------------------------

ChordRest* InputState::cr() const
      {
      return _segment ? static_cast<ChordRest*>(_segment->element(_track)) : 0;
      }

