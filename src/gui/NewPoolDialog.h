// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui {
class NewPoolDialog;
}

namespace WalletGui {

class NewPoolDialog : public QDialog {
  Q_OBJECT

public:
  explicit NewPoolDialog(QWidget* _parent);
  ~NewPoolDialog();

  QString getHost() const;
  quint16 getPort() const;

private:
  QScopedPointer<Ui::NewPoolDialog> m_ui;
};

}
