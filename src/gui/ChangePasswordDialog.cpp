// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ChangePasswordDialog.h"

#include "ui_changepassworddialog.h"

namespace WalletGui {

ChangePasswordDialog::ChangePasswordDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::ChangePasswordDialog) {
  m_ui->setupUi(this);
  m_ui->m_errorLabel->setText("");
  m_ui->m_newPasswordEdit->setAttribute(Qt::WA_MacShowFocusRect, 0);   
}

ChangePasswordDialog::~ChangePasswordDialog() {
}

QString ChangePasswordDialog::getNewPassword() const {
  return m_ui->m_newPasswordEdit->text();
}

QString ChangePasswordDialog::getOldPassword() const {
  return m_ui->m_oldPasswordEdit->text();
}

void ChangePasswordDialog::checkPassword(const QString& _password) {
  bool passwordIsConfirmed = !(m_ui->m_newPasswordEdit->text().trimmed().isEmpty() ||
    m_ui->m_newPasswordConfirmationEdit->text().trimmed().isEmpty() ||
    m_ui->m_newPasswordEdit->text().compare(m_ui->m_newPasswordConfirmationEdit->text()));
  m_ui->m_errorLabel->setText(passwordIsConfirmed ? "" : tr("Password not confirmed"));
  m_ui->m_okButton->setEnabled(passwordIsConfirmed);
}

}
