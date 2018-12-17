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
  void actualInvestmentBalanceUpdated(quint64 _balance);
  void reset();

  Q_SLOT void depositClicked(); /* new deposits */
  Q_SLOT void investmentClicked(); /* new investment */
  Q_SLOT void depositParamsChanged();
  Q_SLOT void investmentParamsChanged();  
  Q_SLOT void showDepositDetails(const QModelIndex& _index);
  Q_SLOT void timeChanged(int _value);
  Q_SLOT void timeChanged2(int _value);  
  Q_SLOT void withdrawClicked();
  Q_SLOT void backClicked(); /* back to overview */
  Q_SLOT void allButtonClicked(); /* add all funds */
  Q_SLOT void investmentsClicked(); /* show investments tab */
  Q_SLOT void depositsClicked(); /* show deposits tab */

Q_SIGNALS:
  void backSignal();

};

}
