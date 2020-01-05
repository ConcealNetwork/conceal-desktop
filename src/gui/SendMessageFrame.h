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
class SendMessageFrame;
}

namespace WalletGui {

class SendMessageFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(SendMessageFrame)

public:
  SendMessageFrame(QWidget* _parent);
  ~SendMessageFrame();

  void setAddress(const QString& _address);

private:
  QScopedPointer<Ui::SendMessageFrame> m_ui;
  void sendMessageCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString& _errorText);
  void reset();

  QString extractAddress(const QString& _addressString) const;
  void recalculateFeeValue();

  Q_SLOT void messageTextChanged();
  Q_SLOT void sendClicked();
  Q_SLOT void addressBookClicked();  
  Q_SLOT void backClicked();  
  Q_SLOT void ttlCheckStateChanged(int _state);
  Q_SLOT void ttlValueChanged(int _ttlValue);

  Q_SIGNALS:
  void backSignal();

};

}
