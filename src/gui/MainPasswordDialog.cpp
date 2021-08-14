// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include "Settings.h"
#include <QFontDatabase>
#include "MainPasswordDialog.h"


#include "ui_mainpassworddialog.h"

namespace WalletGui
{

MainPasswordDialog::MainPasswordDialog(bool _error, QWidget *_parent) : QDialog(_parent), m_ui(new Ui::MainPasswordDialog)
{
  m_ui->setupUi(this);
  m_ui->m_version->setText(QString(tr("Conceal Desktop %1")).arg(Settings::instance().getVersion()));

  QString walletFile = Settings::instance().getWalletName();
  m_ui->m_currentWalletTitle->setText("Wallet: " + walletFile);

  int startingFontSize = Settings::instance().getFontSize();
  setStyles(startingFontSize);

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
  QDesktopServices::openUrl(QUrl("https://conceal.network/support/", QUrl::TolerantMode));
}

void MainPasswordDialog::changeClicked() {
  Q_EMIT changeSignal();
  this->reject();
}

MainPasswordDialog::~MainPasswordDialog()
{
}

QString MainPasswordDialog::getPassword() const
{
  return m_ui->m_passwordEdit->text();
}

QList<QWidget *> MainPasswordDialog::getWidgets()
{
  return m_ui->passwordBox->findChildren<QWidget *>();
}

QList<QPushButton *> MainPasswordDialog::getButtons()
{
  return m_ui->passwordBox->findChildren<QPushButton *>();
}

QList<QLabel *> MainPasswordDialog::getLabels()
{
  return m_ui->passwordBox->findChildren<QLabel *>();
}

void MainPasswordDialog::applyStyles() { m_ui->passwordBox->update(); }

} // namespace WalletGui
