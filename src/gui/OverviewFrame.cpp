// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "CurrencyAdapter.h"
#include "OverviewFrame.h"
#include "TransactionFrame.h"
#include "RecentTransactionsModel.h"
#include "MessagesModel.h"
#include "WalletAdapter.h"
#include "AddressProvider.h"
#include "DepositsFrame.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"
#include "VisibleMessagesModel.h"
#include "DepositDetailsDialog.h"
#include "DepositListModel.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"
#include "VisibleMessagesModel.h"
#include "AddressBookModel.h"
#include "AddressProvider.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "Settings.h"
#include "NodeAdapter.h"
#include "transactionconfirmation.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include <Common/Base58.h>
#include <CryptoNoteCore/CryptoNoteTools.h>
#include <Common/Util.h>
#include <Common/Base58.h>
#include "Common/StringTools.h"
#include "Common/CommandLine.h"
#include "AddressBookDialog.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/Account.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "WalletLegacy/WalletHelper.h"
#include "Common/SignalHandler.h"
#include "Common/PathTools.h"
#include "Common/Util.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "Common/DnsTools.h"
#include "DepositModel.h"
#include "PriceProvider.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "QRLabel.h"
#include "SortedTransactionsModel.h"
#include "TransactionDetailsDialog.h"
#include "TransactionsListModel.h"
#include "TransactionsModel.h"
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QAction>
#include <QClipboard>
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>

#include "ui_overviewframe.h"

namespace WalletGui
{

Q_DECL_CONSTEXPR quint64 MESSAGE_AMOUNT = 100;
Q_DECL_CONSTEXPR quint64 MESSAGE_CHAR_PRICE = 10;
Q_DECL_CONSTEXPR quint64 MINIMAL_MESSAGE_FEE = 10;
Q_DECL_CONSTEXPR int DEFAULT_MESSAGE_MIXIN = 4;
Q_DECL_CONSTEXPR quint32 MINUTE_SECONDS = 60;
Q_DECL_CONSTEXPR quint32 HOUR_SECONDS = 60 * MINUTE_SECONDS;
Q_DECL_CONSTEXPR int MIN_TTL = 5 * MINUTE_SECONDS;
Q_DECL_CONSTEXPR int MAX_TTL = 14 * HOUR_SECONDS;
Q_DECL_CONSTEXPR int TTL_STEP = 5 * MINUTE_SECONDS;

/* Convert months to the number of blocks */
QString monthsToBlocks(int _months)
{
  QString resTempate("%1 %2");
  /* A maxmimum of 12 months */
  if (_months < 13)
  {
    return resTempate.arg(_months * 21900).arg(QObject::tr("blocks"));
  }
  return QString();
}

class RecentTransactionsDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  RecentTransactionsDelegate(QObject *_parent) : QStyledItemDelegate(_parent)
  {
  }
  ~RecentTransactionsDelegate()
  {
  }

  QWidget *createEditor(QWidget *_parent, const QStyleOptionViewItem &_option, const QModelIndex &_index) const Q_DECL_OVERRIDE
  {
    if (!_index.isValid())
    {
      return nullptr;
    }
    return new TransactionFrame(_index, _parent);
  }

  QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const Q_DECL_OVERRIDE
  {
    return QSize(346, 32);
  }
};

OverviewFrame::OverviewFrame(QWidget *_parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame),
                                                 m_priceProvider(new PriceProvider(this)),
                                                 m_transactionModel(new RecentTransactionsModel),
                                                 m_transactionsModel(new TransactionsListModel),
                                                 m_depositModel(new DepositListModel),
                                                 m_visibleMessagesModel(new VisibleMessagesModel)
{
  m_ui->setupUi(this);

  m_ui->m_transactionsView->setModel(m_transactionsModel.data());
  m_ui->m_depositView->setModel(m_depositModel.data());
  m_ui->m_messagesView->setModel(m_visibleMessagesModel.data());

  /* New message box */
  m_ui->m_ttlSlider->setVisible(false);
  m_ui->m_ttlLabel->setVisible(false);
  m_ui->m_ttlSlider->setMinimum(1);
  m_ui->m_ttlSlider->setMaximum(MAX_TTL / MIN_TTL);

  /* Deposit box */
  m_ui->m_timeSpin->setMinimum(1);
  m_ui->m_timeSpin->setMaximum(12);
  m_ui->m_timeSpin->setSuffix(QString(" %1").arg(tr("Months")));
  m_ui->m_amountSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());

  m_ui->m_messagesView->header()->resizeSection(MessagesModel::COLUMN_DATE, 140);
  m_ui->m_transactionsView->header()->setSectionResizeMode(TransactionsModel::COLUMN_STATE, QHeaderView::Fixed);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_STATE, 15);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_DATE, 140);
  m_ui->m_transactionsView->header()->moveSection(3, 5);
  m_ui->m_transactionsView->header()->moveSection(0, 1);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_HASH, 300);

  /* Load the new app-wide font */
  int id = QFontDatabase::addApplicationFont(":/fonts/Raleway-Regular.ttf");
  QFont font;
  font.setFamily("Raleway");
  font.setPointSize(13);
  m_ui->m_messagesView->setFont(font);
  m_ui->m_depositView->setFont(font);
  m_ui->m_transactionsView->setFont(font);

  /* Connect signals */
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, &OverviewFrame::sendTransactionCompleted, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendMessageCompletedSignal, this, &OverviewFrame::sendMessageCompleted, Qt::QueuedConnection);

  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &OverviewFrame::actualBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingBalanceUpdatedSignal, this, &OverviewFrame::pendingBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal, this, &OverviewFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingDepositBalanceUpdatedSignal, this, &OverviewFrame::pendingDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualInvestmentBalanceUpdatedSignal, this, &OverviewFrame::actualInvestmentBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingInvestmentBalanceUpdatedSignal, this, &OverviewFrame::pendingInvestmentBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &OverviewFrame::reset, Qt::QueuedConnection);
  connect(m_transactionModel.data(), &QAbstractItemModel::rowsInserted, this, &OverviewFrame::transactionsInserted);
  connect(m_transactionModel.data(), &QAbstractItemModel::layoutChanged, this, &OverviewFrame::layoutChanged);

  connect(&WalletAdapter::instance(), &WalletAdapter::updateWalletAddressSignal, this, &OverviewFrame::updateWalletAddress);
  connect(m_priceProvider, &PriceProvider::priceFoundSignal, this, &OverviewFrame::onPriceFound);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletStateChangedSignal, this, &OverviewFrame::setStatusBarText);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &OverviewFrame::walletSynchronized, Qt::QueuedConnection);

  /* Initialize basic ui elements */
  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel4->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel5->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_recentTransactionsView->setItemDelegate(new RecentTransactionsDelegate(this));
  m_ui->m_recentTransactionsView->setModel(m_transactionModel.data());

  int subMenu = 0;

  walletSynced = false;

  /* Hide the second chart */
  int currentChart = 2;
  m_ui->m_chart->show();
  m_ui->m_chart_2->hide();

  /* Pull the chart */
  QNetworkAccessManager *nam = new QNetworkAccessManager(this);
  connect(nam, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished);
  const QUrl url = QUrl::fromUserInput("http://explorer.conceal.network/services/charts/price.png?vsCurrency=usd&days=7&priceDecimals=2&xPoints=12&width=511&height=191");
  QNetworkRequest request(url);
  nam->get(request);

  /* Pull the alternate chart */
  QNetworkAccessManager *nam2 = new QNetworkAccessManager(this);
  connect(nam2, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished2);
  const QUrl url2 = QUrl::fromUserInput("http://explorer.conceal.network/services/charts/price.png?vsCurrency=btc&days=7&priceDecimals=6&priceSymbol=btc&xPoints=12&width=511&height=191");
  QNetworkRequest request2(url2);
  nam2->get(request2);

  QString connection = Settings::instance().getConnection();
  if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
  {
    QString remoteNodeUrl = Settings::instance().getCurrentRemoteNode() + "/feeaddress";
    m_addressProvider->getAddress(remoteNodeUrl);
    connect(m_addressProvider, &AddressProvider::addressFoundSignal, this, &OverviewFrame::onAddressFound, Qt::QueuedConnection);
  }

  dashboardClicked();
  depositParamsChanged();
  reset();
}

OverviewFrame::~OverviewFrame()
{
}

void OverviewFrame::walletSynchronized(int _error, const QString &_error_text)
{
  showCurrentWallet();
  walletSynced = true;

  /* Show total portfolio */
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
}

void OverviewFrame::transactionsInserted(const QModelIndex &_parent, int _first, int _last)
{
  for (quint32 i = _first; i <= _last; ++i)
  {
    QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
    m_priceProvider->getPrice();
  }
}

void OverviewFrame::updateWalletAddress(const QString &_address)
{
  m_ui->m_copyAddressButton->setStyleSheet("Text-align:left");
}

void OverviewFrame::showCurrentWallet()
{
  /* Show the name of the opened wallet */
  QString walletFile = Settings::instance().getWalletName();
  m_ui->m_currentWalletTitle->setText("" + walletFile.toUpper());
}

void OverviewFrame::downloadFinished(QNetworkReply *reply)
{
  /* Download is done
       set the chart as the pixmap */
  QPixmap pm;
  pm.loadFromData(reply->readAll());
  m_ui->m_chart->setPixmap(pm);
}

void OverviewFrame::downloadFinished2(QNetworkReply *reply2)
{
  /* Download is done
       set the chart as the pixmap */
  QPixmap pm2;
  pm2.loadFromData(reply2->readAll());
  m_ui->m_chart_2->setPixmap(pm2);
}

void OverviewFrame::layoutChanged()
{
  for (quint32 i = 0; i <= m_transactionModel->rowCount(); ++i)
  {
    QModelIndex recent_index = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recent_index);
    m_priceProvider->getPrice();
  }
  showCurrentWallet();
}

void OverviewFrame::actualBalanceUpdated(quint64 _balance)
{
  m_ui->m_actualBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  m_ui->m_balanceLabel->setText("Available Balance: " + CurrencyAdapter::instance().formatAmount(_balance) + " CCX");
  m_actualBalance = _balance;
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_totalBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingBalance) + " CCX");
  m_priceProvider->getPrice();
}

void OverviewFrame::pendingBalanceUpdated(quint64 _balance)
{
  m_ui->m_pendingBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_totalBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualBalance) + " CCX");
  m_priceProvider->getPrice();
}

void OverviewFrame::actualDepositBalanceUpdated(quint64 _balance)
{
  m_ui->m_unlockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_totalDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingDepositBalance) + " CCX");
  m_priceProvider->getPrice();
}

void OverviewFrame::actualInvestmentBalanceUpdated(quint64 _balance)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_unlockedInvestmentsLabel->setText(CurrencyAdapter::instance().formatAmount(actualInvestmentBalance));
  m_ui->m_totalInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(pendingInvestmentBalance + actualInvestmentBalance) + " CCX");
  m_priceProvider->getPrice();
}

void OverviewFrame::pendingDepositBalanceUpdated(quint64 _balance)
{
  m_ui->m_lockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_totalDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualDepositBalance) + " CCX");
  m_priceProvider->getPrice();
}

void OverviewFrame::pendingInvestmentBalanceUpdated(quint64 _balance)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_lockedInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(pendingInvestmentBalance));
  m_ui->m_totalInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(pendingInvestmentBalance + actualInvestmentBalance) + " CCX");
  m_priceProvider->getPrice();
}

void OverviewFrame::onPriceFound(const QString &_btcccx, const QString &_usdccx, const QString &_usdbtc, const QString &_usdmarketcap, const QString &_usdvolume)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  float ccxusd = _usdccx.toFloat();
  float total = ccxusd * (float)totalBalance;
  m_ui->m_ccxusd->setText("$" + _usdccx);
  m_ui->m_btcusd->setText("$" + _usdbtc);
  m_ui->m_marketCap->setText("$" + _usdmarketcap);
  m_ui->m_volume->setText(_btcccx + " sats");
  m_ui->m_totalPortfolioLabelUSD->setText("TOTAL " + CurrencyAdapter::instance().formatAmount(totalBalance) + " CCX | " + QString::number(total / 1000000, 'f', 2) + " USD");
}

void OverviewFrame::sendClicked()
{
  if (walletSynced == true)
  {
    Q_EMIT sendSignal();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::depositClicked()
{
  if (walletSynced == true)
  {
    m_ui->m_myConcealWalletTitle->setText("BANKING");
    m_ui->bankingBox->raise();
    m_ui->m_newTransferButton->hide();
    m_ui->m_newMessageButton->hide();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::transactionClicked()
{
  m_ui->m_myConcealWalletTitle->setText("TRANSACTIONS");
  m_ui->transactionsBox->raise();
  m_ui->m_newTransferButton->hide();
  m_ui->m_newMessageButton->hide();
}

void OverviewFrame::dashboardClicked()
{
  m_ui->m_myConcealWalletTitle->setText("MY CONCEAL WALLET");
  m_ui->overviewBox->raise();
  m_ui->m_newTransferButton->show();
  m_ui->m_newMessageButton->show();
}

void OverviewFrame::aboutClicked()
{
  m_ui->m_myConcealWalletTitle->setText("ABOUT");
  m_ui->aboutBox->raise();
  m_ui->m_newTransferButton->show();
  m_ui->m_newMessageButton->show();
}

void OverviewFrame::settingsClicked()
{
  m_ui->m_myConcealWalletTitle->setText("WALLET SETTINGS");
  m_ui->settingsBox->raise();
  m_ui->m_newTransferButton->hide();
  m_ui->m_newMessageButton->hide();
}

void OverviewFrame::walletClicked()
{
  m_ui->m_myConcealWalletTitle->setText("WALLET OPTIONS");
  m_ui->walletBox->raise();
  m_ui->m_newTransferButton->hide();
  m_ui->m_newMessageButton->hide();
}

void OverviewFrame::qrCodeClicked()
{
  Q_EMIT qrSignal(m_ui->m_copyAddressButton->text());
}

void OverviewFrame::messageClicked()
{
  m_ui->m_myConcealWalletTitle->setText("INBOX");
  m_ui->messageBox->raise();
  m_ui->m_newTransferButton->hide();
  m_ui->m_newMessageButton->hide();
}

void OverviewFrame::newWalletClicked()
{
  Q_EMIT newWalletSignal();
}

void OverviewFrame::closeWalletClicked()
{
  Q_EMIT closeWalletSignal();
}

void OverviewFrame::newTransferClicked()
{
  if (walletSynced == true)
  {
    m_ui->m_myConcealWalletTitle->setText("SEND FUNDS");
    m_ui->sendBox->raise();
    m_ui->m_newTransferButton->hide();
    m_ui->m_newMessageButton->hide();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::newMessageClicked()
{
  if (walletSynced == true)
  {
    m_ui->m_myConcealWalletTitle->setText("NEW MESSAGE");
    m_ui->newMessageBox->raise();
    m_ui->m_newTransferButton->hide();
    m_ui->m_newMessageButton->hide();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::reset()
{
  actualBalanceUpdated(0);
  pendingBalanceUpdated(0);
  actualDepositBalanceUpdated(0);
  pendingDepositBalanceUpdated(0);
  actualInvestmentBalanceUpdated(0);
  pendingInvestmentBalanceUpdated(0);
  m_priceProvider->getPrice();
  Q_EMIT resetWalletSignal();
}

void OverviewFrame::setStatusBarText(const QString &_text)
{
  m_ui->m_statusBox->setText(_text);
}

void OverviewFrame::copyClicked()
{
  QApplication::clipboard()->setText(m_ui->m_copyAddressButton->text());
  QMessageBox::information(this, tr("Wallet"), "Address copied to clipboard");
}

void OverviewFrame::syncMessage()
{
  QMessageBox::information(this, tr("Synchronization"), "Synchronization is in progress. This option is not available until your wallet is synchronized with the network.");
}

void OverviewFrame::chartButtonClicked()
{
  if (currentChart == 1)
  {
    m_ui->m_chart->hide();
    m_ui->m_chart_2->show();
    currentChart = 2;
  }
  else
  {
    m_ui->m_chart->show();
    m_ui->m_chart_2->hide();
    currentChart = 1;
  }
}

// TRANSACTION HISTORY

void OverviewFrame::showTransactionDetails(const QModelIndex &_index)
{
  if (!_index.isValid())
  {
    return;
  }

  TransactionDetailsDialog dlg(_index, this);
  dlg.exec();
}

// MESSAGE LIST

void OverviewFrame::showMessageDetails(const QModelIndex &_index)
{
  if (!_index.isValid())
  {
    return;
  }

  MessageDetailsDialog dlg(_index, this);
  dlg.exec();
}

// SEND FUNDS

/* incoming data from address book frame */
void OverviewFrame::setAddress(const QString &_address)
{
  m_ui->m_addressEdit->setText(_address);
  m_ui->m_myConcealWalletTitle->setText("SEND FUNDS");
  m_ui->sendBox->raise();
  m_ui->m_newTransferButton->hide();
  m_ui->m_newMessageButton->hide();
}

/* incoming data from address book frame */
void OverviewFrame::setPaymentId(const QString &_paymentId)
{
  m_ui->m_paymentIdEdit->setText(_paymentId);
}

/* Set the variable to the fee address, save the address in settings so
   other functions can use, and show the fee if a fee address is found */
void OverviewFrame::onAddressFound(const QString &_address)
{
  OverviewFrame::remote_node_fee_address = _address;
  Settings::instance().setCurrentFeeAddress(_address);
  m_ui->m_sendFee->setText("Fee: 0.011000 CCX");
  m_ui->m_messageFee->setText("Fee: 0.011000 CCX");
  m_ui->m_depositFeeLabel->setText("0.011000 CCX");
  
}

/* clear all fields */
void OverviewFrame::clearAllClicked()
{
  m_ui->m_paymentIdEdit->clear();
  m_ui->m_addressEdit->clear();
  m_ui->m_addressLabel->clear();
  m_ui->m_messageEdit->clear();
  m_ui->m_amountEdit->setText("0.000000");
}

void OverviewFrame::sendFundsClicked()
{
  /* Get the most up-to-date fee based on characters in the message */
  QVector<CryptoNote::WalletLegacyTransfer> walletTransfers;
  CryptoNote::WalletLegacyTransfer walletTransfer;
  QVector<CryptoNote::TransactionMessage> walletMessages;
  bool isIntegrated = false;
  std::string paymentID;
  std::string spendPublicKey;
  std::string viewPublicKey;
  QByteArray paymentIdString;

  QString address = m_ui->m_addressEdit->text().toUtf8();
  QString int_address = m_ui->m_addressEdit->text().toUtf8();

  /* Integrated address check */
  if (address.toStdString().length() == 186)
  {
    isIntegrated = true;
    const uint64_t paymentIDLen = 64;

    /* Extract and commit the payment id to extra */
    std::string decoded;
    uint64_t prefix;
    if (Tools::Base58::decode_addr(address.toStdString(), prefix, decoded))
    {
      paymentID = decoded.substr(0, paymentIDLen);
    }

    /* Create the address from the public keys */
    std::string keys = decoded.substr(paymentIDLen, std::string::npos);
    CryptoNote::AccountPublicAddress addr;
    CryptoNote::BinaryArray ba = Common::asBinaryArray(keys);

    CryptoNote::fromBinaryArray(addr, ba);

    std::string address_string = CryptoNote::getAccountAddressAsStr(CryptoNote::parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, addr);
    address = QString::fromStdString(address_string);
  }

  /* Conceal ID check */
  if (CurrencyAdapter::instance().isValidOpenAliasAddress(address))
  {
    /*Parse the record and set address to the actual CCX address*/
    std::vector<std::string> records;
    if (!Common::fetch_dns_txt(address.toStdString(), records))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Failed to lookup Conceal ID"), QtCriticalMsg));
    }
    std::string realAddress;
    for (const auto &record : records)
    {
      if (CurrencyAdapter::instance().processServerAliasResponse(record, realAddress))
      {
        address = QString::fromStdString(realAddress);
        m_ui->m_addressEdit->setText(address);
      }
    }
  }
  if (!CurrencyAdapter::instance().validateAddress(address))
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
    return;
  }

  /* Start building the transaction */
  walletTransfer.address = address.toStdString();
  uint64_t amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountEdit->text());
  walletTransfer.amount = amount;
  walletTransfers.push_back(walletTransfer);
  QString label = m_ui->m_addressLabel->text();

  /* Payment id */
  if (isIntegrated == true)
  {
    m_ui->m_paymentIdEdit->setText(QString::fromStdString(paymentID));
  }

  paymentIdString = m_ui->m_paymentIdEdit->text().toUtf8();

  /* Check payment id validity, or about */
  if (!isValidPaymentId(paymentIdString))
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
    return;
  }

  /* Warn the user if there is no payment id */
  if (paymentIdString.toStdString().length() < 64)
  {
    if (QMessageBox::warning(&MainWindow::instance(), tr("Transaction Confirmation"),
                             tr("Please note that there is no payment ID, are you sure you want to proceed?"),
                             QMessageBox::Cancel,
                             QMessageBox::Ok) != QMessageBox::Ok)
    {
      return;
    }
  }

  /* Add the comment to the transaction */
  QString comment = m_ui->m_messageEdit->text();
  if (!comment.isEmpty())
  {
    walletMessages.append(CryptoNote::TransactionMessage{comment.toStdString(), address.toStdString()});
  }

  quint64 actualFee = 1000;
  quint64 totalFee = 1000;

  /* Remote node fee */
  QString connection = Settings::instance().getConnection();
  if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
  {
    if (!OverviewFrame::remote_node_fee_address.isEmpty())
    {
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = OverviewFrame::remote_node_fee_address.toStdString();
      walletTransfer.amount = 10000;
      walletTransfers.push_back(walletTransfer);
      totalFee = totalFee + 11000;
    }
  }

  /* Check if there are enough funds for the amount plus total fees */
  if (m_actualBalance < (amount + totalFee))
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Insufficient funds. Please ensure that you have enough funds for the amount plus fees."), QtCriticalMsg));
    return;
  }

  /* If the wallet is open we proceed */
  if (WalletAdapter::instance().isOpen())
  {
    /* Send the transaction */
    WalletAdapter::instance().sendTransaction(walletTransfers, actualFee, paymentIdString, 4, walletMessages);
    /* Add to the address book if a label is given */
    if ((!label.isEmpty()) && (m_ui->m_saveAddress->isChecked()))
    {
      if (isIntegrated == true)
      {
        AddressBookModel::instance().addAddress(label, int_address, "");
      }
      else
      {
        AddressBookModel::instance().addAddress(label, address, paymentIdString);
      }
    }
  }
}

void OverviewFrame::sendTransactionCompleted(CryptoNote::TransactionId _id, bool _error, const QString &_errorText)
{
  Q_UNUSED(_id);
  if (_error)
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(_errorText, QtCriticalMsg));
  }
  else
  {
    clearAllClicked();
    dashboardClicked();
  }
}

/* Check if the entered payment ID is valid */
bool OverviewFrame::isValidPaymentId(const QByteArray &_paymentIdString)
{
  if (_paymentIdString.isEmpty())
  {
    return true;
  }
  QByteArray paymentId = QByteArray::fromHex(_paymentIdString);
  return (paymentId.size() == sizeof(Crypto::Hash)) && (_paymentIdString.toUpper() == paymentId.toHex().toUpper());
}

/* Open address book */
void OverviewFrame::addressBookClicked()
{
  Q_EMIT addressBookSignal();
}

// SEND MESSAGE

void OverviewFrame::sendMessageCompleted(CryptoNote::TransactionId _id, bool _error, const QString &_errorText)
{
  Q_UNUSED(_id);
  if (_error)
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(_errorText, QtCriticalMsg));
  }
  else
  {
    clearMessageClicked();
    dashboardClicked();
  }
}

/* clear all fields */
void OverviewFrame::clearMessageClicked()
{
  m_ui->m_messageTextEdit->clear();
  m_ui->m_addressMessageEdit->clear();
}

/* Generate the time display for the TTL change */
void OverviewFrame::ttlValueChanged(int _ttlValue)
{
  quint32 value = _ttlValue * MIN_TTL;
  quint32 hours = value / HOUR_SECONDS;
  quint32 minutes = value % HOUR_SECONDS / MINUTE_SECONDS;
  m_ui->m_ttlLabel->setText(QString("%1h %2m").arg(hours).arg(minutes));
}

void OverviewFrame::sendMessageClicked()
{
  /* Exit if the wallet is not open */
  if (!WalletAdapter::instance().isOpen())
  {
    return;
  }

  QVector<CryptoNote::WalletLegacyTransfer> transfers;
  QVector<CryptoNote::WalletLegacyTransfer> feeTransfer;
  CryptoNote::WalletLegacyTransfer walletTransfer;
  QVector<CryptoNote::TransactionMessage> messages;
  QVector<CryptoNote::TransactionMessage> feeMessage;
  QString address = m_ui->m_addressEdit->text().toUtf8();

  QString messageString = m_ui->m_messageTextEdit->toPlainText();

  /* Start building the transaction */
  walletTransfer.address = address.toStdString();
  uint64_t amount = 100;
  walletTransfer.amount = amount;
  transfers.push_back(walletTransfer);
  messages.append({messageString.toStdString(), address.toStdString()});

  /* Calculate fees */
  quint64 fee = 1000;

  /* Check if this is a self destructive message */
  bool selfDestructiveMessage = false;
  quint64 ttl = 0;
  if (m_ui->m_ttlCheck->checkState() == Qt::Checked)
  {
    ttl = QDateTime::currentDateTimeUtc().toTime_t() + m_ui->m_ttlSlider->value() * MIN_TTL;
    fee = 0;
    selfDestructiveMessage = true;
  }

  /* Add the remote node fee transfer to the transaction if the connection
     is a remote node with an address and this is not a self-destructive message */
  QString remote_node_fee_address = OverviewFrame::remote_node_fee_address;
  if ((!remote_node_fee_address.isEmpty()) && (selfDestructiveMessage == false))
  {
    QString connection = Settings::instance().getConnection();
    if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
    {
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = remote_node_fee_address.toStdString();
      walletTransfer.amount = 10000;
      transfers.push_back(walletTransfer);
    }
  }

  /* Send the message. If it is a self-destructive message, send the fee transaction */
  if (WalletAdapter::instance().isOpen())
  {
    WalletAdapter::instance().sendMessage(transfers, fee, 4, messages, ttl);
    if (selfDestructiveMessage = true)
    {
      WalletAdapter::instance().sendMessage(feeTransfer, 100, 4, feeMessage, 0);
    }
  }
}

void OverviewFrame::addressBookMessageClicked()
{
  AddressBookDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_ui->m_addressMessageEdit->setText(dlg.getAddress());
  }
}

// DEPOSITS

/* New deposit */
void OverviewFrame::newDepositClicked()
{
  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());

  /* Insufficient funds */
  if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFeeBanking() > WalletAdapter::instance().getActualBalance())
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("You don't have enough balance in your account!"), QtCriticalMsg));
    return;
  }

  /* One week is set as 5040 blocks */
  quint32 term = m_ui->m_timeSpin->value() * 21900;

  /* Warn the user */
  if (QMessageBox::warning(&MainWindow::instance(), tr("Deposit Confirmation"),
                           tr("Please note that once funds are locked in a deposit, you will not have access until maturity. Are you sure you want to proceed?"),
                           QMessageBox::Cancel,
                           QMessageBox::Ok) != QMessageBox::Ok)
  {
    return;
  }

  /* Initiate the desposit */
  WalletAdapter::instance().deposit(term, amount, CurrencyAdapter::instance().getMinimumFeeBanking(), 4);

  /* Remote node fee */
  QVector<CryptoNote::WalletLegacyTransfer> walletTransfers;
  QString connection = Settings::instance().getConnection();
  if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
  {
    if (!OverviewFrame::remote_node_fee_address.isEmpty())
    {
      QVector<CryptoNote::TransactionMessage> walletMessages;
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = OverviewFrame::remote_node_fee_address.toStdString();
      walletTransfer.amount = 10000;
      walletTransfers.push_back(walletTransfer);
      /* If the wallet is open we proceed */
      if (WalletAdapter::instance().isOpen())
      {
        /* Send the transaction */
        WalletAdapter::instance().sendTransaction(walletTransfers, 1000, "", 4, walletMessages);
      }
    }
  }
}

void OverviewFrame::showDepositDetails(const QModelIndex &_index)
{
  if (!_index.isValid())
  {
    return;
  }
  DepositDetailsDialog dlg(_index, this);
  dlg.exec();
}

void OverviewFrame::depositParamsChanged()
{
  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
  quint32 term = m_ui->m_timeSpin->value() * 21900;
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestEarnedLabel->setText(QString("%1 %2").arg(CurrencyAdapter::instance().formatAmount(interest)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
  m_ui->m_interestRateLabel->setText(QString("%3 %)").arg(QString::number(termRate * 100, 'f', 4)));
}

void OverviewFrame::timeChanged(int _value)
{
  m_ui->m_timeLabel->setText(monthsToBlocks(m_ui->m_timeSpin->value()));
}

void OverviewFrame::withdrawClicked() 
{
  QModelIndexList unlockedDepositIndexList = DepositModel::instance().match(DepositModel::instance().index(0, 0), DepositModel::ROLE_STATE, DepositModel::STATE_UNLOCKED, -1);
  if (unlockedDepositIndexList.isEmpty()) {
    return;
  }

  QVector<CryptoNote::DepositId> depositIds;
  Q_FOREACH (const QModelIndex& index, unlockedDepositIndexList) {
    depositIds.append(index.row());
  }

  WalletAdapter::instance().withdrawUnlockedDeposits(depositIds, CurrencyAdapter::instance().getMinimumFeeBanking());
}

} // namespace WalletGui

#include "OverviewFrame.moc"
