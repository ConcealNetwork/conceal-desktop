// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QDesktopServices>
#include <QUrl>
#include "MainPasswordDialog.h"
#include "Settings.h"

#include "ui_MainPasswordDialog.h"

namespace WalletGui
{

MainPasswordDialog::MainPasswordDialog(bool _error, QWidget *_parent) : QDialog(_parent), m_ui(new Ui::MainPasswordDialog)
{
  m_ui->setupUi(this);

  QString walletFile = Settings::instance().getWalletName();
  m_ui->m_currentWalletTitle->setText("Wallet: " + walletFile);

  if (!_error)
  {
    m_ui->m_errorLabel->hide();
  }

  adjustSize();
}

void MainPasswordDialog::quitClicked()
{
  QApplication::quit();
}

void MainPasswordDialog::helpClicked()
{
  QDesktopServices::openUrl(QUrl("https://conceal.network/wiki/doku.php?id=start", QUrl::TolerantMode));
}

MainPasswordDialog::~MainPasswordDialog()
{
}

QString MainPasswordDialog::getPassword() const
{
  return m_ui->m_passwordEdit->text();
}

} // namespace WalletGui
