// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "NewPasswordDialog.h"

#include "Settings.h"
#include "ui_newpassworddialog.h"

namespace WalletGui
{
  NewPasswordDialog::NewPasswordDialog(QWidget *_parent)
      : QDialog(_parent), m_ui(new Ui::NewPasswordDialog)
  {
    m_ui->setupUi(this);
    int startingFontSize = Settings::instance().getFontSize();
    EditableStyle::setStyles(startingFontSize);
    m_ui->m_errorLabel->setText("");
  }

  NewPasswordDialog::~NewPasswordDialog() { }

  QString NewPasswordDialog::getPassword() const { return m_ui->m_passwordEdit->text(); }

  void NewPasswordDialog::checkPassword(const QString &_password)
  {
    bool passwordIsConfirmed =
        !(m_ui->m_passwordEdit->text().trimmed().isEmpty() ||
          m_ui->m_passwordConfirmationEdit->text().trimmed().isEmpty() ||
          m_ui->m_passwordEdit->text().compare(m_ui->m_passwordConfirmationEdit->text()));
    m_ui->m_errorLabel->setText(passwordIsConfirmed ? "" : tr("Password not confirmed"));
    m_ui->b1_okButton->setEnabled(passwordIsConfirmed);
  }

  QList<QWidget *> NewPasswordDialog::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> NewPasswordDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> NewPasswordDialog::getLabels()
  {
    return m_ui->groupBox->findChildren<QLabel *>();
  }

  void NewPasswordDialog::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui
