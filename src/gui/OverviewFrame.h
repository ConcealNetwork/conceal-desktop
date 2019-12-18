// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>
#include <QNetworkAccessManager>
#include <QStyledItemDelegate>
#include <IWalletLegacy.h>

namespace Ui {
  class OverviewFrame;
}

namespace WalletGui {

class PriceProvider;
class RecentTransactionsModel;
class TransactionsListModel;
class MessagesFrame;
class DepositListModel;
class VisibleMessagesModel;
class AddressProvider;

class OverviewFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(OverviewFrame)

public:
  explicit OverviewFrame(QWidget* _parent);
  ~OverviewFrame();
  void scrollToTransaction(const QModelIndex& _index);
  void setAddress(const QString& _address);
  void setPaymentId(const QString& _paymendId);  

private:
  QNetworkAccessManager m_networkManager;
  QScopedPointer<Ui::OverviewFrame> m_ui;
  QSharedPointer<RecentTransactionsModel> m_transactionModel;
  QScopedArrayPointer<DepositListModel> m_depositModel;  
  QScopedPointer<VisibleMessagesModel> m_visibleMessagesModel;
  QScopedPointer<TransactionsListModel> m_transactionsModel;
  PriceProvider* m_priceProvider; 
  AddressProvider* m_addressProvider;   
  QString remote_node_fee_address;
  quint64 remote_node_fee;  
  quint64 m_actualBalance = 0;  
  int subMenu = 0;
  int currentChart = 1;
  bool walletSynced = false;

  void onPriceFound(const QString& _btcccx, const QString& _usdccx, const QString& _usdbtc, const QString& _usdmarketcap, const QString& _usdvolume);
  void transactionsInserted(const QModelIndex& _parent, int _first, int _last);
  void transactionsRemoved(const QModelIndex& _parent, int _first, int _last);
  void downloadFinished(QNetworkReply *reply);
  void downloadFinished2(QNetworkReply *reply2);  
  void layoutChanged();
  void setStatusBarText(const QString& _text);
  void updateWalletAddress(const QString& _address);
  void walletSynchronized(int _error, const QString& _error_text);
  void actualBalanceUpdated(quint64 _balance);
  void pendingBalanceUpdated(quint64 _balance);
  void actualDepositBalanceUpdated(quint64 _balance);
  void pendingDepositBalanceUpdated(quint64 _balance);  
  void actualInvestmentBalanceUpdated(quint64 _balance);
  void pendingInvestmentBalanceUpdated(quint64 _balance);    
  void showCurrentWallet();
  void syncMessage();
  void walletActualBalanceUpdated(quint64 _balance);  
  static bool isValidPaymentId(const QByteArray& _paymentIdString);  
  void reset();
  void onAddressFound(const QString& _address);
  void sendTransactionCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString& _errorText);

  Q_SLOT void sendClicked();  
  Q_SLOT void copyClicked();
  Q_SLOT void depositClicked();    
  Q_SLOT void transactionClicked();      
  Q_SLOT void dashboardClicked();
  Q_SLOT void messageClicked();      
  Q_SLOT void newWalletClicked();
  Q_SLOT void closeWalletClicked();
  Q_SLOT void newTransferClicked();
  Q_SLOT void newMessageClicked();
  Q_SLOT void qrCodeClicked();
  Q_SLOT void aboutClicked();
  Q_SLOT void showTransactionDetails(const QModelIndex& _index);  
  Q_SLOT void showMessageDetails(const QModelIndex& _index);    
  Q_SLOT void walletClicked();    
  Q_SLOT void chartButtonClicked();      
  Q_SLOT void settingsClicked();
  Q_SLOT void addressBookClicked();    
  Q_SLOT void sendFundsClicked();
  Q_SLOT void clearAllClicked();  

Q_SIGNALS:
  void sendSignal();
  void depositSignal();  
  void backupSignal();    
  void rescanSignal();
  void openWalletSignal();  
  void newWalletSignal();
  void closeWalletSignal();
  void qrSignal(const QString& _address);
  void newMessageSignal();
  void newTransferSignal();
  void transactionSignal();
  void messageSignal();
  void optimizeSignal();
  void resetWalletSignal();
  void importSignal();
  void aboutSignal();
  void aboutQTSignal();
  void disclaimerSignal();
  void settingsSignal();
  void linksSignal();  
  void importSeedSignal();
  void importGUIKeySignal();
  void importSecretKeysSignal();
  void encryptWalletSignal();
  void connectionSettingsSignal();
  void languageSettingsSignal();  
  void addressBookSignal();  


};
}
