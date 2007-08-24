//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2002-2007 Werner Schweer and others
//  Copyright (C) 2003 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
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

//
// C++ Interface: shortcutcapturedialog
//
// Description:
// Dialog window for capturing keyboard shortcuts
//

#include "ui_shortcutcapturedialog.h"

class Shortcut;

//---------------------------------------------------------
//   ShortcutCaptureDialog
//---------------------------------------------------------

class ShortcutCaptureDialog : public QDialog, public Ui::ShortcutCaptureDialogBase
      {
      Q_OBJECT
      private:
      Shortcut* s;
      void keyPressEvent(QKeyEvent* e);
      QKeySequence key;

    private slots:
      void clearClicked();

    public:
      ShortcutCaptureDialog(Shortcut* s = 0, QWidget* parent = 0);
      ~ShortcutCaptureDialog();
      QKeySequence getKey() const { return key; }
      };

