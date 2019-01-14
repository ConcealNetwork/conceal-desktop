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
  int subMenu = 0;

  void onPriceFound(const QString& _ccxusd, const QString& _ccxbtc, const QString& _btc, const QString& _diff, const QString& _hashrate, const QString& _reward, const QString& _deposits, const QString& _supply);
  void transactionsInserted(const QModelIndex& _parent, int _first, int _last);
  void transactionsRemoved(const QModelIndex& _parent, int _first, int _last);
  void downloadFinished(QNetworkReply *reply);
  void layoutChanged();
  void setStatusBarText(const QString& _text);
  void updateWalletAddress(const QString& _address);
  void actualBalanceUpdated(quint64 _balance);
  void pendingBalanceUpdated(quint64 _balance);
  void poolUpdate(quint64 _dayPoolAmount, quint64 _totalPoolAmount);
  void actualDepositBalanceUpdated(quint64 _balance);
  void pendingDepositBalanceUpdated(quint64 _balance);  
  void actualInvestmentBalanceUpdated(quint64 _balance);
  void pendingInvestmentBalanceUpdated(quint64 _balance);    
  void showCurrentWallet();
  void reset();
  
  Q_SLOT void sendClicked();  
  Q_SLOT void copyClicked();
  Q_SLOT void depositClicked();    
  Q_SLOT void transactionClicked();      
  Q_SLOT void messageClicked();      
  Q_SLOT void addressBookClicked();      
  Q_SLOT void miningClicked();      
  Q_SLOT void newWalletClicked();
  Q_SLOT void newTransferClicked();
  Q_SLOT void newMessageClicked();
  Q_SLOT void qrCodeClicked();
  Q_SLOT void importClicked();
  Q_SLOT void aboutClicked();
  Q_SLOT void walletClicked();    
  Q_SLOT void settingsClicked();
  Q_SLOT void subButton1Clicked();
  Q_SLOT void subButton2Clicked();
  Q_SLOT void subButton3Clicked();
  Q_SLOT void subButton4Clicked();
  Q_SLOT void subButton5Clicked();  

Q_SIGNALS:
  void sendSignal();
  void depositSignal();  
  void backupSignal();    
  void rescanSignal();
  void openWalletSignal();  
  void newWalletSignal();
  void qrSignal(const QString& _address);
  void newMessageSignal();
  void newTransferSignal();
  void transactionSignal();
  void messageSignal();
  void optimizeSignal();
  void resetWalletSignal();
  void importSignal();
  void miningSignal();
  void addressBookSignal();
  void aboutSignal();
  void aboutQTSignal();
  void disclaimerSignal();
  void linksSignal();  
  void importSeedSignal();
  void importGUIKeySignal();
  void importSecretKeysSignal();
  void encryptWalletSignal();
  void connectionSettingsSignal();
};
}
