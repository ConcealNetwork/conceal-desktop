// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

#include <IWalletLegacy.h>

namespace Ui {
  class SendFrame;
}

namespace WalletGui {

class AddressProvider;

class SendFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(SendFrame)

public:
  SendFrame(QWidget* _parent);
  ~SendFrame();

  void setAddress(const QString& _address);
  void setPaymentId(const QString& _paymendId);  

private:
  QScopedPointer<Ui::SendFrame> m_ui;
  AddressProvider* m_addressProvider;

  QString remote_node_fee_address;
  quint64 remote_node_fee;
  void sendTransactionCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString& _errorText);
  void walletActualBalanceUpdated(quint64 _balance);
  void onAddressFound(const QString& _address);
  static bool isValidPaymentId(const QByteArray& _paymentIdString);


  Q_SLOT void clearAllClicked();
  Q_SLOT void backClicked();  
  Q_SLOT void addressBookClicked();    
  Q_SLOT void sendClicked();
  Q_SLOT void updateFee();

Q_SIGNALS:
  void backSignal();
  void addressFoundSignal();  
  void addressBookSignal();  

};

}
