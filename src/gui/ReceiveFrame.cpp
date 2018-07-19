// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QClipboard>

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
  m_ui->m_addressEdit->setText(_address);
  m_ui->m_qrLabel->showQRCode(_address);
}

void ReceiveFrame::walletOpened(int _error) {
  if (_error != 0) {
    return;
  }

  CryptoNote::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);
  std::string secretKeysData = std::string(reinterpret_cast<char*>(&keys.spendSecretKey), sizeof(keys.spendSecretKey)) + std::string(reinterpret_cast<char*>(&keys.viewSecretKey), sizeof(keys.viewSecretKey));
  QString privateKeys = QString::fromStdString(Tools::Base58::encode_addr(CurrencyAdapter::instance().getAddressPrefix(), secretKeysData));

  // check if the wallet is deterministic
  // we do this by generating a view key from the spend key and them comparing it to the existing view key
  Crypto::PublicKey unused_dummy_variable;
  Crypto::SecretKey deterministic_private_view_key;
  std::string mnemonic_seed = "";
  CryptoNote::AccountBase::generateViewFromSpend(keys.spendSecretKey, deterministic_private_view_key, unused_dummy_variable);
  bool deterministic_private_keys = deterministic_private_view_key == keys.viewSecretKey;
  
  if (deterministic_private_keys){
  crypto::ElectrumWords::bytes_to_words(keys.spendSecretKey, mnemonic_seed, "English");
  } else {
     mnemonic_seed = "Your wallet does not support the use of a mnemonic seed. Please create a new wallet.";
  }

  m_ui->m_keyEdit->setText(privateKeys);
  m_ui->m_keyEdit_3->setText(QString::fromStdString(Common::podToHex(keys.spendSecretKey)));
  m_ui->m_keyEdit_4->setText(QString::fromStdString(Common::podToHex(keys.viewSecretKey)));
  m_ui->m_seed->setText(QString::fromStdString(mnemonic_seed));  
}

void ReceiveFrame::walletClosed() {
  m_ui->m_addressEdit->clear();
  m_ui->m_keyEdit->clear();
  m_ui->m_qrLabel->clear();
}

}
