// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui {
class BankingFrame;
}

namespace WalletGui {

class DepositListModel;

class BankingFrame : public QFrame {
  Q_OBJECT

public:
  BankingFrame(QWidget* _parent);
  ~BankingFrame();

private:
  QScopedPointer<Ui::BankingFrame> m_ui;
  QScopedArrayPointer<DepositListModel> m_depositModel;

  void actualDepositBalanceUpdated(quint64 _balance);
  void actualInvestmentBalanceUpdated(quint64 _balance);

  Q_SLOT void showDepositDetails(const QModelIndex& _index);
  Q_SLOT void withdrawClicked();
  Q_SLOT void backClicked(); /* back to overview */

Q_SIGNALS:
  void backSignal();

};

}
