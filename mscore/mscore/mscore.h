//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: mscore.h,v 1.54 2006/04/12 14:58:10 wschweer Exp $
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

#ifndef __MSCORE_H__
#define __MSCORE_H__

#include "globals.h"
#include "ui_measuresdialog.h"

class Canvas;
class Element;
class ToolButton;
class PreferenceDialog;
class EditStyle;
class InstrumentsDialog;
class Instrument;
class MidiFile;
class TextStyleDialog;
class PlayPanel;
class InstrumentListEditor;
class PageListEditor;
class MeasureListEditor;
class Score;
class PageSettings;
class Pad;
class Xml;

extern QString mscoreGlobalShare;
static const int PROJECT_LIST_LEN = 6;

enum {
      STATE_NORMAL,
      STATE_NOTE_ENTRY,
      STATE_EDIT
      };

//---------------------------------------------------------
//   MeasuresDialog
//---------------------------------------------------------

class MeasuresDialog : public QDialog, public Ui::MeasuresDialogBase {
      Q_OBJECT

   private slots:
      virtual void accept();

   public:
      MeasuresDialog(QWidget* parent = 0);
      };

//---------------------------------------------------------
//   TabBar
//---------------------------------------------------------

class TabBar : public QTabBar {
      Q_OBJECT

      virtual void mouseDoubleClickEvent(QMouseEvent* ev) {
            emit doubleClick(currentIndex());
            QTabBar::mouseDoubleClickEvent(ev);
            }
   signals:
      void doubleClick(int);

   public:
      TabBar() : QTabBar() {}
      };


//---------------------------------------------------------
//   Shortcut
//    hold the basic values for configurable shortcuts
//---------------------------------------------------------

struct Shortcut {
      const char* xml;        //! xml tag name for configuration file
      const char* descr;      //! descriptor, shown in editor
      QKeySequence key;       //! shortcut
      Qt::ShortcutContext context;
      QString text;
      QString help;
      QIcon* icon;
      QAction* action;        //! cached action

      Shortcut() {
            xml     = 0;
            descr   = 0;
            key     = 0;
            context = Qt::WindowShortcut;
            icon    = 0;
            action  = 0;
            }
      Shortcut(const char* name, const char* d, const QKeySequence& k,
         Qt::ShortcutContext cont = Qt::ApplicationShortcut,
         const QString& txt = 0, const QString& h = 0, QIcon* i = 0)
         : descr(d), key(k), context(cont), text(txt), help(h) {
            xml    = name;
            icon   = i;
            action = 0;
            }
      };

//---------------------------------------------------------
//   MuseScore
//---------------------------------------------------------

class MuseScore : public QMainWindow {
      Q_OBJECT

      Score* cs;              // current score
      Canvas* canvas;
      QVBoxLayout* layout;
      TabBar* tab;

      QMenu* menuDisplay;
      QMenu* menuEdit;
      QMenu* openRecent;

      QComboBox* mag;
      QActionGroup* transportAction;

      QAction* padId;
      QAction* playId;
      QAction* navigatorId;
      QAction* visibleId;
      QAction* transportId;
      QAction* inputId;

      PreferenceDialog* preferenceDialog;
      QToolBar* fileTools;
      QToolBar* transportTools;
      QToolBar* entryTools;
      QToolBar* voiceTools;
      EditStyle* editStyleWin;
      InstrumentsDialog* instrList;
      TextStyleDialog* textStyleDialog;
      MeasuresDialog* measuresDialog;
      PlayPanel* playPanel;
      InstrumentListEditor* iledit;
      PageListEditor* pageListEdit;
      MeasureListEditor* measureListEdit;
      PageSettings* pageSettings;

      QWidget* symbolPalette1;
      QWidget* clefPalette;
      QWidget* keyPalette;
      QWidget* timePalette;
      QWidget* linePalette;
      QWidget* bracketPalette;
      QWidget* barPalette;
      QWidget* fingeringPalette;
      QWidget* noteAttributesPalette;
      QWidget* accidentalsPalette;
      QWidget* dynamicsPalette;
      QWidget* layoutBreakPalette;
      QStatusBar* _statusBar;
      QLabel* _modeText;

      Pad* pad;
      QList<Score*> scoreList;
      bool _midiinEnabled;
      bool _speakerEnabled;

      virtual void closeEvent(QCloseEvent*);

      void playVisible(bool flag);
      void navigatorVisible(bool flag);
      void launchBrowser(const QString whereTo);

      void addScore(const QString& name);
      void genRecentPopup(QMenu*) const;
      void saveScoreList();
      void loadScoreList();
      void loadInstrumentTemplates();
      void editInstrList();
      void symbolMenu1();
      void clefMenu();
      void keyMenu();
      void timeMenu();
      void dynamicsMenu();
      void loadFile();
      bool saveFile();
      bool saveAs();
      void newFile();

   private slots:
      void helpBrowser();
      void about();
      void aboutQt();
      void padVisible(bool);
      void openRecentMenu();
      void selectScore(QAction*);
      void updateMag();
      void selectionChanged(int);
      void startPreferenceDialog();
      void startInstrumentListEditor();
      void startPageListEditor();
      void preferencesChanged();
      void setStop(bool);
      void setPlay(bool);
      void exportMidi();
      void importMidi();
      void importMusicXml();
      void exportMusicXml();
      void editStyle();
      void saveStyle();
      void loadStyle();
      void editTextStyle();
      void seqStarted();
      void seqStopped();
      void closePlayPanel();

      void lineMenu();
      void bracketMenu();
      void barMenu();
      void fingeringMenu();
      void noteAttributesMenu();
      void accidentalsMenu();
      void midiReceived();
      void cmdAddTitle();
      void cmdAddSubTitle();
      void cmdAddComposer();
      void cmdAddPoet();
      void addLyrics();
      void addExpression();
      void addTechnik();
      void addTempo();
      void addMetronome();
      void cmdAppendMeasures();
      void resetUserStretch();
      void showLayoutBreakPalette();
      void resetUserOffsets();
      void magChanged(int);
      void showPageSettings();
      void pageSettingsChanged();
      void textStyleChanged();
      void showInvisibleClicked();
      void closePad();
      void midiinToggled(bool);
      void speakerToggled(bool);
      void removeTab(int);
      void cmd(QAction*);
      void clipboardChanged();

   public slots:
      void setCurrentScore(int);
      void startNoteEntry();
      void showPlayPanel(bool);
      void showPad(bool);
      void showNavigator(bool);

   public:
      MuseScore();
      Canvas* getCanvas() { return canvas; }
      bool checkDirty(Score*);
      void clearScore();
      bool saveFile(QFileInfo&);
      bool saveFile(QFile*);
      PlayPanel* getPlayPanel() const { return playPanel; }
      Pad* getKeyPad() const          { return pad; }
      QMenu* genCreateMenu();
      void appendScore(Score*);
      QString getScore(int idx) const;
      void midiNoteReceived(int pitch, bool chord);
      void showElementContext(Element* el);
	void cmdAppendMeasures(int);
      bool midiinEnabled() const;
      bool playEnabled() const;
      Score* currentScore() const { return cs; }
      void setState(int);
      static Shortcut sc[];
      };

extern QMenu* genCreateMenu(QWidget* parent);
extern MuseScore* mscore;

extern void writeShortcuts(Xml& xml);
extern void readShortcuts(QDomNode);
extern QAction* getAction(const char*);
extern QMap<QString, Shortcut*> shortcuts;

#endif

