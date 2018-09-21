// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui {
class DepositsFrame;
}

namespace WalletGui {

class DepositListModel;

class DepositsFrame : public QFrame {
  Q_OBJECT

public:
  DepositsFrame(QWidget* _parent);
  ~DepositsFrame();

private:
  QScopedPointer<Ui::DepositsFrame> m_ui;
  QScopedArrayPointer<DepositListModel> m_depositModel;

  void actualDepositBalanceUpdated(quint64 _balance);
  void pendingDepositBalanceUpdated(quint64 _balance);
  void reset();

  Q_SLOT void depositClicked();
  Q_SLOT void depositParamsChanged();
  Q_SLOT void showDepositDetails(const QModelIndex& _index);
  Q_SLOT void timeChanged(int _value);
  Q_SLOT void withdrawClicked();
  Q_SLOT void backClicked();
  Q_SLOT void allButtonClicked();
  Q_SLOT void investmentsClicked();
  Q_SLOT void depositsClicked();  

Q_SIGNALS:
  void backSignal();

};

}
