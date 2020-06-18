// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui {
  class ImportGUIKeyDialog;
}

namespace WalletGui {

  class ImportGUIKeyDialog : public QDialog {
    Q_OBJECT

    public:
      explicit ImportGUIKeyDialog(QWidget* _parent);
      ~ImportGUIKeyDialog();
      QString getKeyString() const;
      QString getFilePath() const;

    private:
      QScopedPointer<Ui::ImportGUIKeyDialog> m_ui;
      Q_SLOT void selectPathClicked();
  };
}
