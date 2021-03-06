//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __STYLE_P_H__
#define __STYLE_P_H__

//
// private header for Style
//

#include "elementlayout.h"
#include "articulation.h"

class Xml;
struct ChordDescription;
class ChordList;

//---------------------------------------------------------
//   TextStyleData
//---------------------------------------------------------

class TextStyleData : public QSharedData, public ElementLayout {
   protected:
      QString name;
      QString family;
      qreal size;
      bool bold;
      bool italic;
      bool underline;
      bool hasFrame;

      bool sizeIsSpatiumDependent;        // text point size depends on _spatium unit

      qreal frameWidth;
      qreal paddingWidth;
      int frameRound;
      QColor frameColor;
      bool circle;
      bool systemFlag;
      QColor foregroundColor;

   public:
      TextStyleData(QString _name, QString _family, qreal _size,
         bool _bold, bool _italic, bool _underline,
         Align _align,
         qreal _xoff, qreal _yoff, OffsetType _ot,
         qreal _rxoff, qreal _ryoff,
         bool sizeSpatiumDependent,
         qreal fw, qreal pw, int fr,
         QColor co, bool circle, bool systemFlag,
         QColor fg);
      TextStyleData();

      void write(Xml&) const;
      void writeProperties(Xml& xml) const;
      void read(QDomElement);
      bool readProperties(QDomElement v);

      QFont font(qreal space) const;
      QFont fontPx(qreal spatium) const;
      QRectF bbox(qreal space, const QString& s) const { return fontMetrics(space).boundingRect(s); }
      QFontMetricsF fontMetrics(qreal space) const     { return QFontMetricsF(font(space)); }
      bool operator!=(const TextStyleData& s) const;
      friend class TextStyle;
      };

//---------------------------------------------------------
//   StyleData
//    this structure contains all style elements
//---------------------------------------------------------

class StyleData : public QSharedData {
   protected:
      QVector<StyleVal> _values;
      mutable ChordList* _chordList;
      QList<TextStyle> _textStyles;
      PageFormat* _pageFormat;
      qreal _spatium;
      ArticulationAnchor _articulationAnchor[ARTICULATIONS];

      bool _customChordList;        // if true, chordlist will be saved as part of score

      void set(const StyleVal& v)                         { _values[v.getIdx()] = v; }
      StyleVal value(StyleIdx idx) const                  { return _values[idx];     }
      const TextStyle& textStyle(TextStyleType idx) const { return _textStyles[idx]; }
      const TextStyle& textStyle(const QString&) const;
      TextStyleType textStyleType(const QString&) const;
      void setTextStyle(const TextStyle& ts);

   public:
      StyleData();
      StyleData(const StyleData&);
      ~StyleData();

      bool load(QFile* qf);
      void load(QDomElement e);
      void save(Xml& xml, bool optimize) const;
      bool isDefault(StyleIdx) const;

      const ChordDescription* chordDescription(int id) const;
      ChordList* chordList() const;
      void setChordList(ChordList*);      // Style gets ownership of ChordList
      PageFormat* pageFormat() const                             { return _pageFormat; }
      void setPageFormat(const PageFormat& pf);
      friend class Style;
      qreal spatium() const                                      { return _spatium; }
      void setSpatium(qreal v)                                   { _spatium = v;    }
      ArticulationAnchor articulationAnchor(int id) const        { return _articulationAnchor[id]; }
      void setArticulationAnchor(int id, ArticulationAnchor val) { _articulationAnchor[id] = val;  }
      };

#endif

