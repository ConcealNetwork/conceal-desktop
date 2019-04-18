// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QClipboard>
#include "SendMessageFrame.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "Message.h"
#include "MessagesModel.h"
#include "Settings.h"
#include "WalletAdapter.h"
#include "AddressBookDialog.h"
#include "WalletEvents.h"
#include "ui_sendmessageframe.h"

namespace WalletGui {

Q_DECL_CONSTEXPR quint64 MESSAGE_AMOUNT = 100;
Q_DECL_CONSTEXPR quint64 MESSAGE_CHAR_PRICE = 10;
Q_DECL_CONSTEXPR quint64 MINIMAL_MESSAGE_FEE = 10;
Q_DECL_CONSTEXPR int DEFAULT_MESSAGE_MIXIN = 4;
Q_DECL_CONSTEXPR quint32 MINUTE_SECONDS = 60;
Q_DECL_CONSTEXPR quint32 HOUR_SECONDS = 60 * MINUTE_SECONDS;
Q_DECL_CONSTEXPR int MIN_TTL = 5 * MINUTE_SECONDS;
Q_DECL_CONSTEXPR int MAX_TTL = 14 * HOUR_SECONDS;
Q_DECL_CONSTEXPR int TTL_STEP = 5 * MINUTE_SECONDS;

SendMessageFrame::SendMessageFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::SendMessageFrame) 
{
  m_ui->setupUi(this);
  m_ui->m_feeSpin->setMinimum(CurrencyAdapter::instance().formatAmount(MESSAGE_AMOUNT + MINIMAL_MESSAGE_FEE).toDouble());
  m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());
  m_ui->m_ttlSlider->setVisible(false);
  m_ui->m_ttlLabel->setVisible(false);
  m_ui->m_ttlSlider->setMinimum(1);
  m_ui->m_ttlSlider->setMaximum(MAX_TTL / MIN_TTL);
  ttlValueChanged(m_ui->m_ttlSlider->value());
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendMessageCompletedSignal, this, &SendMessageFrame::sendMessageCompleted, Qt::QueuedConnection);
  reset();
}

SendMessageFrame::~SendMessageFrame() {
}

/* Add your wallet address to the message so the reciever 
   can reply */
void SendMessageFrame::setAddress(const QString& _address) 
{
  m_ui->m_addReplyToCheck->setChecked(true);
}

void SendMessageFrame::sendMessageCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString& _errorText) 
{
  Q_UNUSED(_transactionId);
  if (_error) 
  {
    QCoreApplication::postEvent(
      &MainWindow::instance(),
      new ShowMessageEvent(_errorText, QtCriticalMsg));
  } 
  else 
  {
    reset();
  }
}

void SendMessageFrame::reset() 
{
  m_ui->m_feeSpin->setValue(MESSAGE_AMOUNT + MINIMAL_MESSAGE_FEE);
  m_ui->m_messageTextEdit->clear();
}

/* Extract wallet address from the provided string */
QString SendMessageFrame::extractAddress(const QString& _addressString) const 
{
  QString address = _addressString;
  if (_addressString.contains('<')) 
  {
    int startPos = _addressString.indexOf('<');
    int endPos = _addressString.indexOf('>');
    address = _addressString.mid(startPos + 1, endPos - startPos - 1);
  }
  return address;
}

void SendMessageFrame::recalculateFeeValue() 
{
  QString messageText = m_ui->m_messageTextEdit->toPlainText();
  quint32 messageSize = messageText.length() ;
  if (messageSize > 0) 
  {
    --messageSize;
  }

  quint64 fee = MINIMAL_MESSAGE_FEE;
  quint64 fee2 = MESSAGE_AMOUNT;
  if (m_ui->m_ttlCheck->checkState() == Qt::Checked) 
  {
    fee = 0;
    fee2 = 100;
  }

  m_ui->m_feeSpin->setMinimum(CurrencyAdapter::instance().formatAmount(fee2 + fee + messageSize * MESSAGE_CHAR_PRICE).toDouble());
  m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());
}

void SendMessageFrame::messageTextChanged() 
{
  recalculateFeeValue();
}

void SendMessageFrame::sendClicked() 
{
  /* Exit if the wallet is not open */
  if (!WalletAdapter::instance().isOpen()) 
  {
    return;
  }

  QVector<CryptoNote::WalletLegacyTransfer> transfers;
  QVector<CryptoNote::WalletLegacyTransfer> feeTransfer;
  CryptoNote::WalletLegacyTransfer walletTransfer;
  QVector<CryptoNote::TransactionMessage> messages;
  QVector<CryptoNote::TransactionMessage> feeMessage;
  QString address = m_ui->m_addressEdit->text().toUtf8();

  QString messageString = m_ui->m_messageTextEdit->toPlainText();
  if (m_ui->m_addReplyToCheck->isChecked()) 
  {
    MessageHeader header;
    header.append(qMakePair(QString(MessagesModel::HEADER_REPLY_TO_KEY), WalletAdapter::instance().getAddress()));
    messageString = Message::makeTextMessage(messageString, header);
  }

  /* Start building the transaction */
  walletTransfer.address = address.toStdString();
  uint64_t amount = MESSAGE_AMOUNT;
  walletTransfer.amount = amount;
  transfers.push_back(walletTransfer);
  messages.append({messageString.toStdString(), address.toStdString()});

  /* Calculate fees */
  quint64 fee = CurrencyAdapter::instance().parseAmount(m_ui->m_feeSpin->cleanText());
  fee -= MESSAGE_AMOUNT * transfers.size();
  if (fee < MINIMAL_MESSAGE_FEE) 
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Incorrect fee"), QtCriticalMsg));
    return;
  }

  /* Check if this is a self destructive message */
  bool selfDestructiveMessage = false;
  quint64 ttl = 0;
  if (m_ui->m_ttlCheck->checkState() == Qt::Checked) 
  {
    ttl = QDateTime::currentDateTimeUtc().toTime_t() + m_ui->m_ttlSlider->value() * MIN_TTL;
    fee = 0;
    selfDestructiveMessage = true;
  }

  /* Add the remote node fee transfer to the transaction if the connection
     is a remote node with an address and this is not a self-destructive message */
  QString remote_node_fee_address = Settings::instance().getCurrentFeeAddress();
  if ((!remote_node_fee_address.isEmpty()) && (selfDestructiveMessage = false))
  {
    QString connection = Settings::instance().getConnection();
    if((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0)) {
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = remote_node_fee_address.toStdString();
      walletTransfer.amount = 1000;
      transfers.push_back(walletTransfer);
    }
  }

  /* If this is a self destructive message, send a second transaction with the fee
     and discourage spam in the transaction pool. We do this regardless of the 
     connection type. The amount is set to 10X and the fee to 100X so the total cost is 110X */
  QString sdm_fee_address = "ccx7TijjcuNXpumtJS6iH9TQL5YiZVggYBUSUGcEWKDeXUjEwaF1JMXQhkrL3amjY6i5miaHaUw4oYeD6W2Kvj9b2nMEQKU7Mt";
  if (selfDestructiveMessage = true)
  {
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = sdm_fee_address.toStdString();
      walletTransfer.amount = 10;
      feeTransfer.push_back(walletTransfer);
  }

  /* Send the message. If it is a self-destructive message, send the fee transaction */
  if (WalletAdapter::instance().isOpen()) 
  {
    WalletAdapter::instance().sendMessage(transfers, fee, DEFAULT_MESSAGE_MIXIN, messages, ttl);
    if (selfDestructiveMessage = true) 
    {
      WalletAdapter::instance().sendMessage(feeTransfer, 100, DEFAULT_MESSAGE_MIXIN, feeMessage, 0);
    }    
  }
}

/* recalculate the fee when the ttl checkbox state changes */
void SendMessageFrame::ttlCheckStateChanged(int _state) 
{
  recalculateFeeValue();
}

/* Generate the time display for the TTL change */
void SendMessageFrame::ttlValueChanged(int _ttlValue) 
{
  quint32 value = _ttlValue * MIN_TTL;
  quint32 hours = value / HOUR_SECONDS;
  quint32 minutes = value % HOUR_SECONDS / MINUTE_SECONDS;
  m_ui->m_ttlLabel->setText(QString("%1h %2m").arg(hours).arg(minutes));
}

/* Back to the Overview frame */
void SendMessageFrame::backClicked() 
{
  Q_EMIT backSignal();
}

void SendMessageFrame::addressBookClicked() 
{
  AddressBookDialog dlg(&MainWindow::instance());
  if(dlg.exec() == QDialog::Accepted) {
    m_ui->m_addressEdit->setText(dlg.getAddress());
  }
}

}
