// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ReceiveFrame.h"

#include <Common/StringTools.h>

#include <QClipboard>
#include <QMessageBox>
#include <string>

#include "WalletAdapter.h"
#include "ui_receiveframe.h"
#include "MainWindow.h"

namespace WalletGui {

ReceiveFrame::ReceiveFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::ReceiveFrame) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::updateWalletAddressSignal, this, &ReceiveFrame::updateWalletAddress);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &ReceiveFrame::walletOpened, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &ReceiveFrame::walletClosed, Qt::QueuedConnection);
}

ReceiveFrame::~ReceiveFrame() {
}

void ReceiveFrame::updateWalletAddress(const QString& _address) {
}


/* generate and display the gui key, secret keys, mnemonic seed */

void ReceiveFrame::walletOpened(int _error) {
  if (_error != 0) {
    return;
  }

  std::string mnemonic_seed;
  WalletAdapter::instance().getMnemonicSeed(mnemonic_seed);

  CryptoNote::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);
  CryptoNote::AccountKeys trkeys;
  WalletAdapter::instance().getAccountKeys(trkeys);
  trkeys.spendSecretKey = boost::value_initialized<Crypto::SecretKey>();
  QString trackingWalletKeys = QString::fromStdString(Common::podToHex(trkeys));
  m_ui->m_guiKey->setText(trackingWalletKeys);

  m_ui->m_spendKey->setText(QString::fromStdString(Common::podToHex(keys.spendSecretKey)));
  m_ui->m_viewKey->setText(QString::fromStdString(Common::podToHex(keys.viewSecretKey)));
  m_ui->m_seed->setText(QString::fromStdString(mnemonic_seed));  

  m_ui->seedBox->hide();
  m_ui->introBox->show();
  m_ui->privateKeyBox->hide();
  m_ui->guiKeyBox->hide();

}

void ReceiveFrame::walletClosed() {
  m_ui->m_guiKey->clear();
}

void ReceiveFrame::backClicked() {
  Q_EMIT backSignal();
}

void ReceiveFrame::copyGUIClicked() {
    QApplication::clipboard()->setText(m_ui->m_guiKey->toPlainText());
    QMessageBox::information(&MainWindow::instance(), tr("Tracking Key"), "Tracking key copied to clipboard"); 
}

void ReceiveFrame::copySpendKeyClicked() {
    QApplication::clipboard()->setText(m_ui->m_spendKey->toPlainText());
    QMessageBox::information(&MainWindow::instance(), tr("Private Spend-Key"), "Private spend-key copied to clipboard"); 
}

void ReceiveFrame::copyViewKeyClicked() {
    QApplication::clipboard()->setText(m_ui->m_viewKey->toPlainText());
    QMessageBox::information(&MainWindow::instance(), tr("Private View-Key"), "Private view-key copied to clipboard");    
}

void ReceiveFrame::copySeedClicked() {
    QApplication::clipboard()->setText(m_ui->m_seed->toPlainText());
    QMessageBox::information(&MainWindow::instance(), tr("Seed"), "Seed copied to clipboard");
}

void ReceiveFrame::showSeed() {
  m_ui->seedBox->show();
  m_ui->introBox->hide();
  m_ui->privateKeyBox->hide();
  m_ui->guiKeyBox->hide();
}

void ReceiveFrame::showGUI() {
  m_ui->guiKeyBox->show();
  m_ui->seedBox->hide();
  m_ui->introBox->hide();
  m_ui->privateKeyBox->hide();
}

void ReceiveFrame::showPrivate() {
  m_ui->guiKeyBox->hide();
  m_ui->seedBox->hide();
  m_ui->introBox->hide();
  m_ui->privateKeyBox->show();
}

void ReceiveFrame::backupClicked() {
  m_ui->seedBox->hide();
  m_ui->introBox->show();
  m_ui->privateKeyBox->hide();
  m_ui->guiKeyBox->hide();  
  Q_EMIT backupSignal();
}

void ReceiveFrame::back2Clicked() {
  m_ui->seedBox->hide();
  m_ui->introBox->show();
  m_ui->privateKeyBox->hide();
  m_ui->guiKeyBox->hide();  
}

}
