// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui {
class DisclaimerDialog;
}

namespace WalletGui {

class DisclaimerDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY(DisclaimerDialog)

public:
  explicit DisclaimerDialog(QWidget* _parent);
  ~DisclaimerDialog();

private:
  QScopedPointer<Ui::DisclaimerDialog> m_ui;
};

}
