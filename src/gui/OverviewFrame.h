// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>
#include <QNetworkAccessManager>
#include <QStyledItemDelegate>

namespace Ui {
class OverviewFrame;
}

namespace WalletGui {

class PriceProvider;

class RecentTransactionsModel;

class OverviewFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(OverviewFrame)

public:
  OverviewFrame(QWidget* _parent);
  ~OverviewFrame();

private:
  QNetworkAccessManager m_networkManager;
  QScopedPointer<Ui::OverviewFrame> m_ui;
  QSharedPointer<RecentTransactionsModel> m_transactionModel;
  PriceProvider* m_priceProvider;  

  void onPriceFound(const QString& _ccxusd, const QString& _ccxbtc, const QString& _btc, const QString& _diff, const QString& _hashrate, const QString& _reward, const QString& _deposits, const QString& _supply);
  void transactionsInserted(const QModelIndex& _parent, int _first, int _last);
  void transactionsRemoved(const QModelIndex& _parent, int _first, int _last);
  void layoutChanged();
  void updateWalletAddress(const QString& _address);
  void actualBalanceUpdated(quint64 _balance);
  void pendingBalanceUpdated(quint64 _balance);
  void actualDepositBalanceUpdated(quint64 _balance);
  void pendingDepositBalanceUpdated(quint64 _balance);
  void reset();
  Q_SLOT void sendClicked();  
  Q_SLOT void depositClicked();  
  Q_SLOT void backupClicked();      
  Q_SLOT void rescanClicked();        

Q_SIGNALS:
  void sendSignal();
  void depositSignal();  
  void backupSignal();    
  void rescanSignal();

};

}
