// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>
#include <QNetworkAccessManager>
#include <QStyledItemDelegate>
#include <IWalletLegacy.h>

namespace Ui
{
class OverviewFrame;
}

namespace WalletGui
{

class PriceProvider;
class RecentTransactionsModel;
class TransactionsListModel;
class MessagesFrame;
class DepositListModel;
class VisibleMessagesModel;
class AddressProvider;
class ExchangeProvider;

class OverviewFrame : public QFrame
{
  Q_OBJECT
  Q_DISABLE_COPY(OverviewFrame)

public:
  explicit OverviewFrame(QWidget *_parent);
  ~OverviewFrame();
  void setAddress(const QString &_address);
  void setPaymentId(const QString &_paymendId);
  bool fromPay = true;
  QModelIndex index;
  void scrollToTransaction(const QModelIndex& _index);

public slots:
  void onCustomContextMenu(const QPoint &point);

public Q_SLOTS:
  void addABClicked();
  void editABClicked();
  void copyABClicked();
  void copyABPaymentIdClicked();
  void deleteABClicked();
  void payToABClicked();

private:
  QNetworkAccessManager m_networkManager;
  QScopedPointer<Ui::OverviewFrame> m_ui;
  QSharedPointer<RecentTransactionsModel> m_transactionModel;
  QScopedArrayPointer<DepositListModel> m_depositModel;
  QScopedPointer<VisibleMessagesModel> m_visibleMessagesModel;
  QScopedPointer<TransactionsListModel> m_transactionsModel;
  PriceProvider *m_priceProvider;
  AddressProvider *m_addressProvider;
  ExchangeProvider *m_exchangeProvider;
  QString remote_node_fee_address;
  quint64 totalBalance = 0;
  float ccxusd = 0;
  float ccxeur = 0;
  QString wallet_address;
  quint64 remote_node_fee;
  quint64 m_actualBalance = 0;
  int subMenu = 0;
  int currentChart = 1;
  bool walletSynced = false;
  QMenu* contextMenu;
  bool paymentIDRequired = false;
  QString exchangeName = "";


  void onPriceFound(const QString& _btcccx, const QString& _usdccx, const QString& _usdbtc, const QString& _usdmarketcap, const QString& _usdvolume, const QString &_eurccx, const QString &_eurbtc, const QString &_eurmarketcap, const QString &_eurvolume);
  void onExchangeFound(QString &_exchange);
  void transactionsInserted(const QModelIndex &_parent, int _first, int _last);
  void transactionsRemoved(const QModelIndex &_parent, int _first, int _last);
  void downloadFinished(QNetworkReply *reply);
  void layoutChanged();
  void setStatusBarText(const QString &_text);
  void updateWalletAddress(const QString &_address);
  void walletSynchronized(int _error, const QString &_error_text);
  void actualBalanceUpdated(quint64 _balance);
  void pendingBalanceUpdated(quint64 _balance);
  void actualDepositBalanceUpdated(quint64 _balance);
  void pendingDepositBalanceUpdated(quint64 _balance);
  void actualInvestmentBalanceUpdated(quint64 _balance);
  void pendingInvestmentBalanceUpdated(quint64 _balance);
  void showCurrentWalletName();
  void syncInProgressMessage();
  void walletActualBalanceUpdated(quint64 _balance);
  static bool isValidPaymentId(const QByteArray &_paymentIdString);
  void reset();
  void onAddressFound(const QString &_address);
  void updatePortfolio();
  void sendTransactionCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString &_errorText);
  void sendMessageCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString &_errorText);
  void delay();
  bool checkWalletPassword();

  Q_SLOT void copyClicked();
  Q_SLOT void bankingClicked();
  Q_SLOT void transactionHistoryClicked();
  Q_SLOT void dashboardClicked();
  Q_SLOT void inboxClicked();
  Q_SLOT void newWalletClicked();
  Q_SLOT void closeWalletClicked();
  Q_SLOT void newTransferClicked();
  Q_SLOT void newMessageClicked();
  Q_SLOT void qrCodeClicked();
  Q_SLOT void aboutClicked();
  Q_SLOT void showTransactionDetails(const QModelIndex &_index);
  Q_SLOT void showMessageDetails(const QModelIndex &_index);
  Q_SLOT void settingsClicked();
  Q_SLOT void addressBookClicked();
  Q_SLOT void sendFundsClicked();
  Q_SLOT void sendMessageClicked();
  Q_SLOT void clearAllClicked();
  Q_SLOT void clearMessageClicked();
  Q_SLOT void ttlValueChanged(int _ttlValue);
  Q_SLOT void recalculateMessageLength();
  Q_SLOT void messageTextChanged();
  Q_SLOT void addressBookMessageClicked();
  Q_SLOT void newDepositClicked();
  Q_SLOT void aboutQTClicked();
  Q_SLOT void depositParamsChanged();
  Q_SLOT void showDepositDetails(const QModelIndex &_index);
  Q_SLOT void timeChanged(int _value);
  Q_SLOT void withdrawClicked();
  Q_SLOT void importSeedButtonClicked();
  Q_SLOT void openWalletButtonClicked();
  Q_SLOT void importTrackingButtonClicked();
  Q_SLOT void importPrivateKeysButtonClicked();
  Q_SLOT void createNewWalletButtonClicked();
  Q_SLOT void backupClicked();
  Q_SLOT void backupFileClicked();  
  Q_SLOT void minToTrayClicked();
  Q_SLOT void closeToTrayClicked();
  Q_SLOT void optimizeClicked();
  Q_SLOT void autoOptimizeClicked(); 
  Q_SLOT void saveLanguageCurrencyClicked();
  Q_SLOT void saveConnectionClicked();
  Q_SLOT void rescanClicked();
  Q_SLOT void currentAddressChanged(const QModelIndex& _index);
  Q_SLOT void addressDoubleClicked(const QModelIndex& _index);
  Q_SLOT void addressChanged(QString);
  Q_SLOT void discordClicked();
  Q_SLOT void telegramClicked();  
  Q_SLOT void twitterClicked();
  Q_SLOT void githubClicked();
  Q_SLOT void redditClicked();
  Q_SLOT void mediumClicked();
  Q_SLOT void lockWallet();
  Q_SLOT void exportCSV();
  Q_SLOT void unlockWallet();
  Q_SLOT void encryptWalletClicked();
  Q_SLOT void stexClicked();
  Q_SLOT void hotbitClicked();
  Q_SLOT void tradeogreClicked();
  Q_SLOT void qtradeClicked();
  Q_SLOT void helpClicked();

Q_SIGNALS:
  void sendSignal();
  void depositSignal();
  void backupSignal();
  void backupFileSignal();  
  void rescanSignal();
  void openWalletSignal();
  void newWalletSignal();
  void closeWalletSignal();
  void qrSignal(const QString &_address);
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
  void importTrackingKeySignal();
  void importSecretKeysSignal();
  void encryptWalletSignal();
  void connectionSettingsSignal();
  void languageSettingsSignal();
  void addressBookSignal();
  void payToSignal(const QModelIndex& _index);  
};
} // namespace WalletGui
