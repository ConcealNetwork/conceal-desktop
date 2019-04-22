// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "PasswordDialog.h"
#include "Settings.h"

#include "ui_passworddialog.h"

namespace WalletGui {

PasswordDialog::PasswordDialog(bool _error, QWidget* _parent) : QDialog(_parent), m_ui(new Ui::PasswordDialog) {
  m_ui->setupUi(this);

  QString walletFile = Settings::instance().getWalletName();
  m_ui->m_currentWalletTitle->setText(walletFile.toUpper());

  if (!_error) {
    m_ui->m_errorLabel->hide();
  }

  adjustSize();
}

PasswordDialog::~PasswordDialog() {
}

QString PasswordDialog::getPassword() const {
  return m_ui->m_passwordEdit->text();
}
  
}
