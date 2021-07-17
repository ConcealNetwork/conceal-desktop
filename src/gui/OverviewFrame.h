// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <IWalletLegacy.h>

#ifdef HAVE_CHART
#include <QtCharts>
#endif
#include <QFrame>
#include <QNetworkAccessManager>
#include <QStyledItemDelegate>
#include <QTimer>

#include "EditableStyle.h"

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

class OverviewFrame : public QFrame, EditableStyle
{
  Q_OBJECT
  Q_DISABLE_COPY(OverviewFrame)

public:
  explicit OverviewFrame(QWidget *_parent);
  ~OverviewFrame();
  void setAddress(const QString &_address);
  void setPaymentId(const QString &_paymendId);
  bool fromPay = true;
  quint64 m_actualFee = 0;
  QModelIndex index;
  void scrollToTransaction(const QModelIndex& _index);

public Q_SLOTS:
  void addABClicked();
  void editABClicked();
  void copyABClicked();
  void copyABPaymentIdClicked();
  void deleteABClicked();
  void payToABClicked();
  void dashboardClicked();

protected:
  void resizeEvent(QResizeEvent *event) override;
  QList<QWidget *> getWidgets() override;
  QList<QPushButton *> getButtons() override;
  QList<QLabel *> getLabels() override;
  void applyStyles() override;

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
  float ccxfiat = 0;
  QString wallet_address;
  quint64 remote_node_fee;
  quint64 m_actualBalance = 0;
  int subMenu = 0;
  int currentChart = 1;
  bool walletSynced = false;
  QMenu* contextMenu;
  bool paymentIDRequired = false;
  QString exchangeName = "";
  QTimer refreshDataTimer;
  const int REFRESH_INTERVAL = 15 * 60 * 1000;  // refresh interval set to 15 min (in ms)
#ifdef HAVE_CHART
  QChartView *m_chartView;
#else
  QLabel *m_chart;
#endif


  void onPriceFound(QJsonObject &result);
  void onExchangeFound(QString &_exchange);
  void transactionsInserted(const QModelIndex &_parent, int _first, int _last);
  void transactionsRemoved(const QModelIndex &_parent, int _first, int _last);
  void downloadFinished(QNetworkReply *reply);
  void layoutChanged();
  void loadChart();
  void setStatusBarText(const QString &_text, const QString &_height);
  void updateWalletAddress(const QString &_address);
  void calculateFee();
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
  void reset();
  void onAddressFound(const QString &_address);
  void updatePortfolio();
  void sendTransactionCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString &_errorText);
  void sendMessageCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString &_errorText);
  void delay();
  bool checkWalletPassword(bool _error=false);
  bool askForWalletPassword(bool _error = false);
  void change();
  void goToWelcomeFrame();
  void disableAddressBookButtons();

  Q_SLOT void copyClicked();
  Q_SLOT void bankingClicked();
  Q_SLOT void transactionHistoryClicked();
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
  Q_SLOT void setPercentage25();
  Q_SLOT void startMaximizedClicked();
  Q_SLOT void setPercentage50();
  Q_SLOT void setPercentage100();    
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
  Q_SLOT void helpDeskClicked();
  Q_SLOT void tradeogreClicked();
  Q_SLOT void wikiClicked();
  Q_SLOT void helpClicked();
  Q_SLOT void addressEditTextChanged(QString text);
  Q_SLOT void qtChartsLicenseClicked();
  Q_SLOT void openSslLicenseClicked();
  Q_SLOT void refreshDataClicked();
  Q_SLOT void autoRefreshButtonClicked();

Q_SIGNALS:
  void backupSignal();
  void backupFileSignal();  
  void rescanSignal();
  void openWalletSignal();
  void newWalletSignal();
  void closeWalletSignal();
  void qrSignal(const QString &_address);
  void resetWalletSignal();
  void aboutQTSignal();
  void importSeedSignal();
  void importGUIKeySignal();
  void importTrackingKeySignal();
  void importSecretKeysSignal();
  void encryptWalletSignal();
  void payToSignal(const QModelIndex& _index);
  void notifySignal(const QString& message);
  void welcomeFrameSignal();
};
} // namespace WalletGui
