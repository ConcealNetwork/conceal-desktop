// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QMessageBox>

#include "AddressBookModel.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "NodeAdapter.h"
#include "SendFrame.h"
#include "transactionconfirmation.h"
#include "TransferFrame.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include <Common/Base58.h>
#include <CryptoNoteCore/CryptoNoteTools.h>
#include <Common/Util.h>
#include <Common/Base58.h>
#include "Common/StringTools.h"
#include "Common/CommandLine.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/Account.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "WalletLegacy/WalletHelper.h"
#include "Common/SignalHandler.h"
#include "Common/PathTools.h"
#include "Common/Util.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"


#include <boost/algorithm/string.hpp>
#include "ui_sendframe.h"

namespace WalletGui {

/* cost per message character */
Q_DECL_CONSTEXPR quint64 COMMENT_CHAR_PRICE = 10;

SendFrame::SendFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::SendFrame) {
  m_ui->setupUi(this);
  clearAllClicked();
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, &SendFrame::sendTransactionCompleted, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &SendFrame::walletActualBalanceUpdated, Qt::QueuedConnection);
  m_ui->m_feeSpin->setMinimum(CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getMinimumFeeV1()).toDouble());
}

SendFrame::~SendFrame() {
}

/* incoming data from address book frame */
void SendFrame::setAddress(const QString& _address) {
  m_ui->m_addressEdit->setText(_address);
}

/* incoming data from address book frame */
void SendFrame::setPaymentId(const QString& _paymentId) {
  m_ui->m_paymentIdEdit->setText(_paymentId);
}

void SendFrame::clearAllClicked() {
  /* clear all fields */
  m_ui->m_paymentIdEdit->clear();
  m_ui->m_addressEdit->clear();
  m_ui->m_labelEdit->clear();  
  m_ui->m_messageEdit->clear();  
  m_ui->m_amountEdit->setText("0.000000");
  m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());
}

void SendFrame::sendClicked() {
  updateFee();
  QVector<CryptoNote::WalletLegacyTransfer> walletTransfers;
  CryptoNote::WalletLegacyTransfer walletTransfer;
  QVector<CryptoNote::TransactionMessage> walletMessages;
  bool isIntegrated = false;
  std::string paymentID;
  std::string spendPublicKey;
  std::string viewPublicKey;
  QByteArray paymentIdString;

  QString address = m_ui->m_addressEdit->text().toUtf8();
  QString int_address = m_ui->m_addressEdit->text().toUtf8();

  /* integrated address check */
  if (address.toStdString().length() == 186) {
    isIntegrated = true;
    const uint64_t paymentIDLen = 64;

    /* extract and commit the payment id to extra */
    std::string decoded;
    uint64_t prefix;
    if (Tools::Base58::decode_addr(address.toStdString(), prefix, decoded)) {      
      paymentID = decoded.substr(0, paymentIDLen);
    }

    /* create the address from the public keys */
    std::string keys = decoded.substr(paymentIDLen, std::string::npos);
    CryptoNote::AccountPublicAddress addr;
    CryptoNote::BinaryArray ba = Common::asBinaryArray(keys);

    CryptoNote::fromBinaryArray(addr, ba);

    std::string address_string = CryptoNote::getAccountAddressAsStr(CryptoNote::parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, addr);   
    address = QString::fromStdString(address_string);
  }

  if (!CurrencyAdapter::instance().validateAddress(address)) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
    return;
  }

  /* build the transaction */
  walletTransfer.address = address.toStdString();
  uint64_t amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountEdit->text());
  walletTransfer.amount = amount;
  walletTransfers.push_back(walletTransfer);
  QString label = m_ui->m_labelEdit->text();

  /* payment id */
  if (isIntegrated == true) {
      m_ui->m_paymentIdEdit->setText(QString::fromStdString(paymentID));
  }

  paymentIdString = m_ui->m_paymentIdEdit->text().toUtf8();
  m_ui->m_paymentIdEdit->setText("");

  /* check payment id validity, or about */
  if (!isValidPaymentId(paymentIdString)) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
    return;
  }

  /* warn the user if there is no payment id */
  if (paymentIdString.toStdString().length() < 64) {
    if (QMessageBox::warning(&MainWindow::instance(), tr("Transaction Confirmation"),
      tr("Please note that there is no payment ID, are you sure you want to proceed?"), 
      QMessageBox::Cancel, 
      QMessageBox::Ok) != QMessageBox::Ok) {
      return;
    }
  }

  /* add the comment to the transaction */
  QString comment = m_ui->m_messageEdit->text();
  if (!comment.isEmpty()) {
    walletMessages.append(CryptoNote::TransactionMessage{comment.toStdString(), address.toStdString()});
  }  

  /* incorrect fee */
  quint64 fee = CurrencyAdapter::instance().parseAmount(m_ui->m_feeSpin->cleanText());
  if (fee < CurrencyAdapter::instance().getMinimumFeeV1()) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Incorrect fee value"), QtCriticalMsg));
    return;
  }

  /* if the wallet is open we proceed */
  if (WalletAdapter::instance().isOpen()) {    
    /* send the transaction */
    WalletAdapter::instance().sendTransaction(walletTransfers, fee, paymentIdString, 4, walletMessages);
    /* add to the address book if a label is given */
    if (!label.isEmpty()) {
      if (isIntegrated == true) {
        AddressBookModel::instance().addAddress(label, int_address, "");
      } else {
        AddressBookModel::instance().addAddress(label, address, paymentIdString);
      }
    }
  }
}

void SendFrame::sendTransactionCompleted(CryptoNote::TransactionId _id, bool _error, const QString& _errorText) {
  Q_UNUSED(_id);
  if (_error) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(_errorText, QtCriticalMsg));
  } else {
    clearAllClicked();
  }
}

void SendFrame::walletActualBalanceUpdated(quint64 _balance) {
  m_ui->m_balanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
}

void SendFrame::updateFee() {
  /* calculate fee based on number of characters in the message */
  quint64 commentsFee = 0;
  std::string words = (m_ui->m_messageEdit->text()).toStdString();
  commentsFee = words.length() * COMMENT_CHAR_PRICE;

  quint64 currentFee = CurrencyAdapter::instance().parseAmount(m_ui->m_feeSpin->cleanText());
  quint64 minCurrentFee = commentsFee + CurrencyAdapter::instance().getMinimumFeeV1();
  if (currentFee < minCurrentFee) {
    m_ui->m_feeSpin->setMinimum(CurrencyAdapter::instance().formatAmount(commentsFee + CurrencyAdapter::instance().getMinimumFeeV1()).toDouble());
    m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());   
  }
}

bool SendFrame::isValidPaymentId(const QByteArray& _paymentIdString) {
  if (_paymentIdString.isEmpty()) {
    return true;
  }
  QByteArray paymentId = QByteArray::fromHex(_paymentIdString);
  return (paymentId.size() == sizeof(Crypto::Hash)) && (_paymentIdString.toUpper() == paymentId.toHex().toUpper());
}

void SendFrame::backClicked() {
  /* back to overview frame */
  Q_EMIT backSignal();
}

void SendFrame::addressBookClicked() {
  /* show address book */
  Q_EMIT addressBookSignal();
}


}
