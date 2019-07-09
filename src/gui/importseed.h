// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui {
  class ImportSeed;
}

namespace WalletGui {

  class ImportSeed : public QDialog {
    Q_OBJECT

    public:
      explicit ImportSeed(QWidget* _parent);
      ~ImportSeed();
      QString getKeyString() const;
      QString getFilePath() const;

    private:
      QScopedPointer<Ui::ImportSeed> m_ui;
      Q_SLOT void selectPathClicked();
  };
}
