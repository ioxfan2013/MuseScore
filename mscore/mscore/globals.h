//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: globals.h,v 1.17 2006/03/02 17:08:34 wschweer Exp $
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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

extern bool debugMode;
extern bool layoutDebug;
extern bool noSeq;            ///< Dont use sequencer; cmd line option.
extern bool noMidi;           ///< Dont use midi; cmd line option.
extern bool dumpMidi;         ///< debug option: dump midi input

static const QString mimeSymbolFormat("application/mscore/symbol");

static const qreal INCH = 25.4;
static const qreal PPI  = 72.0;     // printer points per inch

extern qreal DPI;       // drawing resolution
extern qreal DPMM;      // dots/mm

// used for stem and slur:
enum Direction { AUTO, UP, DOWN };

enum Placement {
      PLACE_AUTO, PLACE_ABOVE, PLACE_BELOW
      };

enum DurationType {
      D_QUARTER, D_EIGHT, D_256TH, D_128TH, D_64TH, D_32ND,
      D_16TH, D_HALF, D_WHOLE, D_BREVE, D_LONG
      };

const int VOICES = 4;
const int MAX_STAVES = 4;

static const qreal DPMM_DISPLAY = 4;   // 100 DPI
static const qreal PALETTE_SPATIUM = 1.9 * DPMM_DISPLAY;

extern QString language;

extern int appDpiX;
extern int appDpiY;

extern QTextStream cout, eout;
#endif
