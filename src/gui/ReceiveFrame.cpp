// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QClipboard>
#include <QMessageBox>

#include <Common/Base58.h>
#include "Common/StringTools.h"
#include "ReceiveFrame.h"
#include "CryptoNoteCore/Account.h"
#include "Mnemonics/electrum-words.h"
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"
#include "ui_receiveframe.h"

#include <string>

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

  

  CryptoNote::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);
  std::string secretKeysData = std::string(reinterpret_cast<char*>(&keys.spendSecretKey), sizeof(keys.spendSecretKey)) + std::string(reinterpret_cast<char*>(&keys.viewSecretKey), sizeof(keys.viewSecretKey));
  QString privateKeys = QString::fromStdString(Tools::Base58::encode_addr(CurrencyAdapter::instance().getAddressPrefix(), std::string(reinterpret_cast<char*>(&keys), sizeof(keys))));
  //QString privateKeys = QString::fromStdString(Tools::Base58::encode_addr(CurrencyAdapter::instance().getAddressPrefix(), secretKeysData));

  /* check if the wallet is deterministic
     generate a view key from the spend key and them compare it to the existing view key */
  Crypto::PublicKey unused_dummy_variable;
  Crypto::SecretKey deterministic_private_view_key;
  std::string mnemonic_seed = "";
  CryptoNote::AccountBase::generateViewFromSpend(keys.spendSecretKey, deterministic_private_view_key, unused_dummy_variable);
  bool deterministic_private_keys = deterministic_private_view_key == keys.viewSecretKey;
  
  if (deterministic_private_keys) {
    crypto::ElectrumWords::bytes_to_words(keys.spendSecretKey, mnemonic_seed, "English");
  } else {
    mnemonic_seed = "Your wallet does not support the use of a mnemonic seed. Please create a new wallet.";
  }

  m_ui->m_guiKey->setText(privateKeys);
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
    QMessageBox::information(this, tr("GUI Key"), "GUI key copied to clipboard"); 
}

void ReceiveFrame::copySpendKeyClicked() {
    QApplication::clipboard()->setText(m_ui->m_spendKey->toPlainText());
    QMessageBox::information(this, tr("Private Spend-Key"), "Private spend-key copied to clipboard"); 
}

void ReceiveFrame::copyViewKeyClicked() {
    QApplication::clipboard()->setText(m_ui->m_viewKey->toPlainText());
    QMessageBox::information(this, tr("Private View-Key"), "Private view-key copied to clipboard");    
}

void ReceiveFrame::copySeedClicked() {
    QApplication::clipboard()->setText(m_ui->m_seed->toPlainText());
    QMessageBox::information(this, tr("Seed"), "Seed copied to clipboard");
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
