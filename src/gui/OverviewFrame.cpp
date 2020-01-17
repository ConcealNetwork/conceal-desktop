// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "AddressBookDialog.h"
#include "AddressBookModel.h"
#include "AddressProvider.h"
#include "Common/CommandLine.h"
#include "Common/DnsTools.h"
#include "Common/PathTools.h"
#include "Common/SignalHandler.h"
#include "Common/StringTools.h"
#include "Common/Util.h"
#include "Common/Base58.h"
#include "Common/Util.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/Account.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "CurrencyAdapter.h"
#include "DepositDetailsDialog.h"
#include "DepositListModel.h"
#include "DepositModel.h"
#include "ExchangeProvider.h"
#include "MainWindow.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "NewAddressDialog.h"
#include "NodeAdapter.h"
#include "OverviewFrame.h"
#include "PasswordDialog.h"
#include "PriceProvider.h"
#include "QRLabel.h"
#include "RecentTransactionsModel.h"
#include "Settings.h"
#include "SortedMessagesModel.h"
#include "SortedTransactionsModel.h"
#include "transactionconfirmation.h"
#include "TransactionDetailsDialog.h"
#include "TransactionFrame.h"
#include "TransactionsListModel.h"
#include "TransactionsModel.h"
#include "VisibleMessagesModel.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "WalletLegacy/WalletHelper.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QClipboard>
#include <QDesktopServices>
#include <QFont>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QStringList>
#include <QtCore>
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

  int maxPeriod = 13;
  uint32_t blocksPerDeposit = 21900;

  QString resTempate("%1 %2");
  if (_months < maxPeriod)
  {
    return resTempate.arg(_months * blocksPerDeposit).arg(QObject::tr("blocks"));
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
                                                 m_visibleMessagesModel(new VisibleMessagesModel),
                                                 m_exchangeProvider(new ExchangeProvider(this)),
                                                 m_addressProvider(new AddressProvider(this))
{
  m_ui->setupUi(this);

  m_ui->m_addressBookView->setModel(&AddressBookModel::instance());
  m_ui->m_addressBookView->header()->setStretchLastSection(false);
  m_ui->m_addressBookView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_ui->m_addressBookView->setSortingEnabled(true);
  m_ui->m_addressBookView->sortByColumn(0, Qt::AscendingOrder);

  //connect(m_ui->m_addressBookView->selectionModel(), &QItemSelectionModel::currentChanged, this, &OverviewFrame::currentAddressChanged);

  m_ui->m_addressBookView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui->m_addressBookView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));

  contextMenu = new QMenu();
  contextMenu->addAction(QString(tr("&Pay to")), this, SLOT(payToClicked()));
  contextMenu->addAction(QString(tr("Copy &label")), this, SLOT(copyLabelClicked()));
  contextMenu->addAction(QString(tr("Copy &address")), this, SLOT(copyClicked()));
  contextMenu->addAction(QString(tr("Copy Payment &ID")), this, SLOT(copyPaymentIdClicked()));
  contextMenu->addAction(QString(tr("&Edit")), this, SLOT(editClicked()));
  contextMenu->addAction(QString(tr("&Delete")), this, SLOT(deleteClicked()));

  if (!Settings::instance().isEncrypted())
    m_ui->m_lockWalletButton->hide();

  m_ui->m_transactionsView->setModel(m_transactionsModel.data());
  m_ui->m_depositView->setModel(m_depositModel.data());
  m_ui->m_messagesView->setModel(m_visibleMessagesModel.data());

  m_ui->m_timeSpin->setSuffix(QString(" %1").arg(tr("Month(s)")));
  m_ui->m_timeSpin->setMaximum(12);
  timeChanged(1);

  m_ui->darkness->hide();
  m_ui->lockBox->hide();
  m_ui->darkness->setStyleSheet("background-color: rgba(0,0,0,160);");

  m_ui->m_ttlSlider->setVisible(true);
  m_ui->m_ttlLabel->setVisible(true);
  m_ui->m_ttlSlider->setMinimum(1);
  m_ui->m_ttlSlider->setMaximum(MAX_TTL / MIN_TTL);
  ttlValueChanged(m_ui->m_ttlSlider->value());

  m_ui->m_amountSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());

  m_ui->m_messagesView->header()->resizeSection(MessagesModel::COLUMN_DATE, 140);
  m_ui->m_transactionsView->header()->setSectionResizeMode(TransactionsModel::COLUMN_STATE, QHeaderView::Fixed);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_STATE, 15);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_DATE, 140);
  m_ui->m_transactionsView->header()->moveSection(3, 5);
  m_ui->m_transactionsView->header()->moveSection(0, 1);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_HASH, 300);

  m_ui->m_depositView->header()->resizeSection(DepositModel::COLUMN_STATE, 75);
  m_ui->m_depositView->header()->resizeSection(DepositModel::COLUMN_AMOUNT, 100);
  m_ui->m_depositView->header()->resizeSection(DepositModel::COLUMN_UNLOCK_TIME, 200);
  m_ui->m_depositView->header()->resizeSection(DepositModel::COLUMN_TYPE, 50);

  int id = QFontDatabase::addApplicationFont(":/fonts/Poppins-Regular.ttf");
  QFont font;
  font.setFamily("Poppins");
  font.setPointSize(13);
  m_ui->m_messagesView->setFont(font);
  m_ui->m_depositView->setFont(font);
  m_ui->m_transactionsView->setFont(font);
  m_ui->m_encryptWalletButton->setVisible(false);
  m_ui->m_transactionsDescription->setFont(font);

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
  connect(m_addressProvider, &AddressProvider::addressFoundSignal, this, &OverviewFrame::onAddressFound, Qt::QueuedConnection);
  connect(m_exchangeProvider, &ExchangeProvider::exchangeFoundSignal, this, &OverviewFrame::onExchangeFound);

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

  m_ui->m_overviewWithdrawButton->hide();

  /* Hide the second chart */
  int currentChart = 2;
  m_ui->m_chart->show();
  m_ui->m_chart_2->hide();
  /* Pull the chart */
  QNetworkAccessManager *nam = new QNetworkAccessManager(this);
  connect(nam, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished);
  const QUrl url = QUrl::fromUserInput("https://explorer.conceal.network/services/charts/price.png?vsCurrency=usd&days=7&priceDecimals=2&xPoints=12&width=511&height=191&dateFormat=MM-DD");
  QNetworkRequest request(url);
  nam->get(request);

  /* Pull the alternate chart */
  //QNetworkAccessManager *nam2 = new QNetworkAccessManager(this);
  //connect(nam2, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished2);
  //const QUrl url2 = QUrl::fromUserInput("http://explorer.conceal.network/services/charts/price.png?vsCurrency=btc&days=7&priceDecimals=6&priceSymbol=btc&xPoints=12&width=511&height=191");
  //QNetworkRequest request2(url2);
  //nam2->get(request2);

  QString connection = Settings::instance().getConnection();

  /* Get current language */
  QString language = Settings::instance().getLanguage();
  if (language.compare("tr") == 0)
  {
    m_ui->m_turkish->setChecked(true);
  }
  else if (language.compare("ru") == 0)
  {
    m_ui->m_russian->setChecked(true);
  }
  else if (language.compare("cn") == 0)
  {
    m_ui->m_chinese->setChecked(true);
  }
  else
  {
    m_ui->m_english->setChecked(true);
  }

  /* Get current currency */
  QString currency = Settings::instance().getCurrentCurrency();
  if (currency.compare("EUR") == 0)
  {
    m_ui->m_eur->setChecked(true);
  }
  else
  {
    m_ui->m_usd->setChecked(true);
  }

  /* Set current connection options */
  QString remoteHost = Settings::instance().getCurrentRemoteNode();
  m_ui->m_hostEdit->setText(remoteHost);

  /* If the connection is a remote node, then load the current (or default)
      remote node into the text field. */
  if (connection.compare("remote") == 0)
  {
    m_ui->radioButton->setChecked(true);
  }

  if (connection.compare("autoremote") == 0)
  {
    m_ui->radioButton_3->setChecked(true);
  }
  /* It is an embedded node, so let only check that */
  else if (connection.compare("embedded") == 0)
  {
    m_ui->radioButton_2->setChecked(true);
  }

  if (Settings::instance().getAutoOptimizationStatus() == "enabled")
  {
    m_ui->m_autoOptimizeButton->setText(tr("CLICK TO DISABLE"));
  }
  else
  {
    m_ui->m_autoOptimizeButton->setText(tr("CLICK TO ENABLE"));
  }

  dashboardClicked();
  depositParamsChanged();
  reset();
  showCurrentWallet();
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
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;

  quint64 numUnlockedOutputs;
  numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
  if (numUnlockedOutputs>= 100)
  {
    m_ui->m_optimizationMessage->setText("(Optimization recommended [" + QString::number(numUnlockedOutputs) + " outputs])");
  }
  else
  {
    m_ui->m_optimizationMessage->setText("(Optimization not required [" + QString::number(numUnlockedOutputs) + " outputs])");
  }

  updatePortfolio();
}

void OverviewFrame::transactionsInserted(const QModelIndex &_parent, int _first, int _last)
{
  for (quint32 i = _first; i <= _last; ++i)
  {
    QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
  }
}

void OverviewFrame::updateWalletAddress(const QString &_address)
{
  m_ui->m_copyAddressButton->setStyleSheet("border: none; font-size: 14px;font-family: 'Poppins';color: orange; text-align: left;");
  OverviewFrame::wallet_address = _address;
  m_ui->m_copyAddressButton_3->setText(OverviewFrame::wallet_address);
}

void OverviewFrame::showCurrentWallet()
{
  /* Show the name of the opened wallet */
  QString walletFile = Settings::instance().getWalletName();
  m_ui->m_currentWalletTitle->setText(tr("Current Wallet") + ": " + walletFile.toUpper());
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
  }
  showCurrentWallet();
}

void OverviewFrame::actualBalanceUpdated(quint64 _balance)
{
  m_ui->m_actualBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));                   // Overview screen
  m_ui->m_balanceLabel->setText("Available Balance: " + CurrencyAdapter::instance().formatAmount(_balance)); // Send funds screen
  m_actualBalance = _balance;
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  updatePortfolio();
}

void OverviewFrame::pendingBalanceUpdated(quint64 _balance)
{
  m_ui->m_lockedBalance->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  updatePortfolio();
}

/* Unlocked deposits */
void OverviewFrame::actualDepositBalanceUpdated(quint64 _balance)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_unlockedDeposits->setText(CurrencyAdapter::instance().formatAmount(actualDepositBalance + actualInvestmentBalance));
  updatePortfolio();
  quint64 unlockedFunds = actualDepositBalance + actualInvestmentBalance;
  if (walletSynced == true)
  {
    if (unlockedFunds> 0)
    {
      m_ui->m_unlockedDeposits->setStyleSheet("color: orange; background: transparent; font-family: Poppins; font-size: 14px; border: none;");
      m_ui->m_overviewWithdrawButton->show();
    }
    else
    {
      m_ui->m_unlockedDeposits->setStyleSheet("color: #ddd; background: transparent; font-family: Poppins; font-size: 14px; border: none;");
      m_ui->m_overviewWithdrawButton->hide();
    }
  }
}

/* Unlocked investments */
void OverviewFrame::actualInvestmentBalanceUpdated(quint64 _balance)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_unlockedDeposits->setText(CurrencyAdapter::instance().formatAmount(actualDepositBalance + actualInvestmentBalance));
  updatePortfolio();
  quint64 unlockedFunds = actualDepositBalance + actualInvestmentBalance;
  if (walletSynced == true)
  {
    if (unlockedFunds> 0)
    {
      m_ui->m_unlockedDeposits->setStyleSheet("color: orange; background: transparent; font-family: Poppins; font-size: 14px; border: none;");
      m_ui->m_overviewWithdrawButton->show();
    }
    else
    {
      m_ui->m_unlockedDeposits->setStyleSheet("color: #ddd; background: transparent; font-family: Poppins; font-size: 14px; border: none;");
      m_ui->m_overviewWithdrawButton->hide();
    }
  }
}

void OverviewFrame::pendingDepositBalanceUpdated(quint64 _balance)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_lockedDeposits->setText(CurrencyAdapter::instance().formatAmount(pendingDepositBalance + pendingInvestmentBalance));
  updatePortfolio();
}

void OverviewFrame::pendingInvestmentBalanceUpdated(quint64 _balance)
{
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_lockedInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(pendingInvestmentBalance));
  m_ui->m_totalInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(pendingInvestmentBalance + actualInvestmentBalance));
  m_ui->m_lockedDeposits->setText(CurrencyAdapter::instance().formatAmount(pendingDepositBalance + pendingInvestmentBalance));
  updatePortfolio();
}

void OverviewFrame::onPriceFound(const QString &_btcccx, const QString &_usdccx, const QString &_usdbtc, const QString &_usdmarketcap, const QString &_usdvolume, const QString &_eurccx, const QString &_eurbtc, const QString &_eurmarketcap, const QString &_eurvolume)
{
  QString currentCurrency = Settings::instance().getCurrentCurrency();

  float total = 0;
  if (currentCurrency == "EUR")
  {
    ccxeur = _eurccx.toFloat();
    m_ui->m_ccxusd->setText("€" + _eurccx);
    m_ui->m_btcusd->setText("€" + _eurbtc);
    m_ui->m_marketCap->setText("€" + _eurmarketcap);
    m_ui->m_volume->setText("€" + _eurvolume);
  }
  else
  {
    ccxusd = _usdccx.toFloat();
    m_ui->m_ccxusd->setText("$" + _usdccx);
    m_ui->m_btcusd->setText("$" + _usdbtc);
    m_ui->m_marketCap->setText("$" + _usdmarketcap);
    m_ui->m_volume->setText("$" + _usdvolume);
  }

  updatePortfolio();
}

void OverviewFrame::onExchangeFound(QString &_exchange)
{
  exchangeName = _exchange;
}

void OverviewFrame::updatePortfolio()
{

  QString currentCurrency = Settings::instance().getCurrentCurrency();

  float total = 0;
  if (currentCurrency == "EUR")
  {
    total = ccxeur * (float)OverviewFrame::totalBalance;
  }
  else
  {
    total = ccxusd * (float)OverviewFrame::totalBalance;
  }
  m_ui->m_totalPortfolioLabelUSD->setText(tr("TOTAL") + " " + CurrencyAdapter::instance().formatAmount(OverviewFrame::totalBalance) + " CCX | " + CurrencyAdapter::instance().formatCurrencyAmount(total / 10000) + " " + Settings::instance().getCurrentCurrency());
}

void OverviewFrame::sendClicked()
{

  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

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
  m_ui->darkness->hide();
  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  if (walletSynced == true)
  {
    m_ui->m_myConcealWalletTitle->setText("BANKING");
    m_ui->bankingBox->raise();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::transactionClicked()
{
  m_ui->darkness->hide();
  m_ui->m_myConcealWalletTitle->setText("TRANSACTIONS");
  m_ui->transactionsBox->raise();
}

void OverviewFrame::dashboardClicked()
{
  m_ui->darkness->hide();
  m_ui->m_myConcealWalletTitle->setText("CONCEAL.NETWORK");
  m_ui->overviewBox->raise();
  m_ui->m_newTransferButton->show();
  m_ui->m_newMessageButton->show();
}

void OverviewFrame::aboutClicked()
{
  m_ui->darkness->hide();
  m_ui->m_myConcealWalletTitle->setText("ABOUT");
  m_ui->aboutBox->raise();
  m_ui->m_newTransferButton->show();
  m_ui->m_newMessageButton->show();
}

void OverviewFrame::settingsClicked()
{
  m_ui->darkness->hide();
  m_ui->m_myConcealWalletTitle->setText("WALLET SETTINGS");
  m_ui->settingsBox->raise();
}

void OverviewFrame::walletClicked()
{
  m_ui->darkness->hide();
  m_ui->m_myConcealWalletTitle->setText("WALLET OPTIONS");
  m_ui->walletBox->raise();

  if (!Settings::instance().isEncrypted())
    m_ui->m_encryptWalletButton->setVisible(true);
}

void OverviewFrame::qrCodeClicked()
{
  Q_EMIT qrSignal(OverviewFrame::wallet_address);
}

void OverviewFrame::messageClicked()
{
  m_ui->darkness->hide();
  m_ui->m_myConcealWalletTitle->setText("INBOX");
  m_ui->messageBox->raise();
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
  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  if (walletSynced == true)
  {
    m_ui->m_myConcealWalletTitle->setText("SEND FUNDS");
    m_ui->sendBox->raise();

    OverviewFrame::fromPay = true;
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::newMessageClicked()
{

  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  if (walletSynced == true)
  {
    m_ui->m_myConcealWalletTitle->setText("NEW MESSAGE");
    m_ui->newMessageBox->raise();

    OverviewFrame::fromPay = false;
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
  m_addressProvider->getAddress();
  Q_EMIT resetWalletSignal();
}

void OverviewFrame::setStatusBarText(const QString &_text)
{
  m_ui->m_statusBox->setText(_text);
}

void OverviewFrame::copyClicked()
{
  QApplication::clipboard()->setText(OverviewFrame::wallet_address);
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
  m_ui->darkness->show();
  m_ui->darkness->raise();
  TransactionDetailsDialog dlg(_index, this);
  dlg.exec();
  m_ui->darkness->hide();
}

// MESSAGE LIST

void OverviewFrame::showMessageDetails(const QModelIndex &_index)
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  if (!_index.isValid())
  {
    return;
  }

  MessageDetailsDialog dlg(_index, this);
  dlg.exec();
  m_ui->darkness->hide();
}

// SEND FUNDS

/* incoming data from address book frame */
void OverviewFrame::setAddress(const QString &_address)
{
  if (OverviewFrame::fromPay == true)
  {
    m_ui->m_addressEdit->setText(_address);
    m_ui->m_myConcealWalletTitle->setText("SEND FUNDS");
    m_ui->sendBox->raise();
  }
  else
  {
    m_ui->m_addressMessageEdit->setText(_address);
    m_ui->m_myConcealWalletTitle->setText("SEND MESSAGE");
    m_ui->newMessageBox->raise();
  }
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
  QString connection = Settings::instance().getConnection();
  if ((!_address.isEmpty()) && (connection != "embedded"))
  {
    OverviewFrame::remote_node_fee_address = _address;
    Settings::instance().setCurrentFeeAddress(_address);
    m_ui->m_sendFee->setText("Fee: 0.011000 CCX");
    m_ui->m_messageFee->setText("Fee: 0.011000 CCX");
    m_ui->m_depositFeeLabel->setText("0.011000 CCX");
  }
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
  if (!checkWalletPassword())
  {
    return;
  }

  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  QString paymentIDText = m_ui->m_paymentIdEdit->text();

  if (paymentIDText.isEmpty())
  {
    if (!exchangeName.isEmpty())
    {
      QMessageBox::information(this, tr("Payment ID Required"), "This address belongs to " + exchangeName + " and requires a Payment ID. Please enter the Payment ID provided by the exchange to proceed.");
      return;
    }
  }

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
  m_ui->m_myConcealWalletTitle->setText("ADDRESS BOOK");
  m_ui->addressBookBox->raise();
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

void OverviewFrame::addressChanged(QString _address)
{
  m_exchangeProvider->getExchange(_address);
}

void OverviewFrame::recalculateMessageLength()
{

  if (m_ui->m_messageTextEdit->toPlainText().length()> 261)
  {
    m_ui->m_messageTextEdit->setPlainText(m_ui->m_messageTextEdit->toPlainText().left(m_ui->m_messageTextEdit->toPlainText().length() - 1));
    m_ui->m_messageTextEdit->moveCursor(QTextCursor::End);
    QMessageBox::information(NULL, QString::fromUtf8("Warning"),
                             QString::fromUtf8("Warning: you have reached the maximum message size of 260 characters."),
                             QString::fromUtf8("Ok"));
  }

  QString messageText = m_ui->m_messageTextEdit->toPlainText();
  quint32 messageSize = messageText.length();
  if (messageSize> 0)
  {
    --messageSize;
  }

  m_ui->m_messageLength->setText(QString::number(messageSize));
}

void OverviewFrame::messageTextChanged()
{
  recalculateMessageLength();
}

void OverviewFrame::sendMessageClicked()
{
  /* Exit if the wallet is not open */
  if (!WalletAdapter::instance().isOpen())
  {
    return;
  }

  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  if (!checkWalletPassword())
  {
    return;
  }

  QVector<CryptoNote::WalletLegacyTransfer> transfers;
  QVector<CryptoNote::WalletLegacyTransfer> feeTransfer;
  CryptoNote::WalletLegacyTransfer walletTransfer;
  QVector<CryptoNote::TransactionMessage> messages;
  QVector<CryptoNote::TransactionMessage> feeMessage;
  QString address = m_ui->m_addressMessageEdit->text().toUtf8();
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
  if ((!OverviewFrame::remote_node_fee_address.isEmpty()) && (selfDestructiveMessage == false))
  {
    QString connection = Settings::instance().getConnection();
    if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
    {
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = OverviewFrame::remote_node_fee_address.toStdString();
      walletTransfer.amount = 10000;
      transfers.push_back(walletTransfer);
    }
  }

  QString messageText = m_ui->m_messageTextEdit->toPlainText();
  quint32 messageSize = messageText.length();
  if (messageSize> 0)
  {
    --messageSize;
  }

  if (messageSize> 260)
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Message too long. Please ensure that the message is less than 260 characters."), QtCriticalMsg));
    return;
  }

  /* Send the message. If it is a self-destructive message, send the fee transaction */
  if (WalletAdapter::instance().isOpen())
  {
    WalletAdapter::instance().sendMessage(transfers, fee, 4, messages, ttl);
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

void OverviewFrame::aboutQTClicked()
{
  Q_EMIT aboutQTSignal();
}

// DEPOSITS

/* New deposit */
void OverviewFrame::newDepositClicked()
{

  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());

  /* Insufficient funds */
  if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFeeBanking()> WalletAdapter::instance().getActualBalance())
  {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("You don't have enough balance in your account!"), QtCriticalMsg));
    return;
  }

  if (!checkWalletPassword())
  {
    return;
  }

  uint32_t blocksPerDeposit = 21900;

  if (NodeAdapter::instance().getLastKnownBlockHeight() < 413400)
  {
    blocksPerDeposit = 5040;
  }

  quint32 term = m_ui->m_timeSpin->value() * blocksPerDeposit;

  /* Warn the user */
  if (QMessageBox::warning(&MainWindow::instance(), tr("Deposit Confirmation"),
                           tr("Please note that once funds are locked in a deposit, you will not have access until maturity. Are you sure you want to proceed?"),
                           QMessageBox::Cancel,
                           QMessageBox::Ok) != QMessageBox::Ok)
  {
    return;
  }

  /* Initiate the desposit */
  WalletAdapter::instance().deposit(term, amount, 1000, 4);

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
  m_ui->darkness->show();
  m_ui->darkness->raise();
  DepositDetailsDialog dlg(_index, this);
  dlg.exec();
  m_ui->darkness->hide();
}

void OverviewFrame::depositParamsChanged()
{
  uint32_t blocksPerDeposit = 21900;

  if (NodeAdapter::instance().getLastKnownBlockHeight() < 413400)
  {
    blocksPerDeposit = 5040;
  }

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
  quint32 term = m_ui->m_timeSpin->value() * blocksPerDeposit;
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestEarnedLabel->setText(QString("%1 %2").arg(CurrencyAdapter::instance().formatAmount(interest)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
  m_ui->m_interestRateLabel->setText(QString("%3 %").arg(QString::number(termRate * 100, 'f', 4)));
}

void OverviewFrame::timeChanged(int _value)
{
  m_ui->m_timeLabel->setText(monthsToBlocks(m_ui->m_timeSpin->value()));
}

void OverviewFrame::withdrawClicked()
{
  QModelIndexList unlockedDepositIndexList = DepositModel::instance().match(DepositModel::instance().index(0, 0), DepositModel::ROLE_STATE, DepositModel::STATE_UNLOCKED, -1);
  if (unlockedDepositIndexList.isEmpty())
  {
    return;
  }

  QVector<CryptoNote::DepositId> depositIds;
  Q_FOREACH (const QModelIndex &index, unlockedDepositIndexList)
  {
    depositIds.append(index.row());
  }

  WalletAdapter::instance().withdrawUnlockedDeposits(depositIds, CurrencyAdapter::instance().getMinimumFeeBanking());
  actualBalanceUpdated(0);
  pendingBalanceUpdated(0);
  actualDepositBalanceUpdated(0);
  pendingDepositBalanceUpdated(0);
  actualInvestmentBalanceUpdated(0);
  pendingInvestmentBalanceUpdated(0);
}

void OverviewFrame::importSeedButtonClicked()
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  Q_EMIT importSeedSignal();
  dashboardClicked();
}

void OverviewFrame::openWalletButtonClicked()
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  Q_EMIT openWalletSignal();
  dashboardClicked();
}

void OverviewFrame::importTrackingButtonClicked()
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  Q_EMIT importTrackingKeySignal();
  dashboardClicked();
}

void OverviewFrame::importPrivateKeysButtonClicked()
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  Q_EMIT importSecretKeysSignal();
  dashboardClicked();
}

void OverviewFrame::createNewWalletButtonClicked()
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  Q_EMIT newWalletSignal();
  dashboardClicked();
}

void OverviewFrame::backupClicked()
{
  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
    return;
  }

  if (!checkWalletPassword())
  {
    return;
  }

  Q_EMIT backupSignal();
}

void OverviewFrame::backupFileClicked()
{

  if (!checkWalletPassword())
  {
    return;
  }

  Q_EMIT backupFileSignal();
}

void OverviewFrame::optimizeClicked()
{
  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
  }
  else
  {
    quint64 numUnlockedOutputs;
    numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
    WalletAdapter::instance().optimizeWallet();
    while (WalletAdapter::instance().getNumUnlockedOutputs()> 100)
    {
      numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
      if (numUnlockedOutputs == 0)
        break;
      WalletAdapter::instance().optimizeWallet();
      delay();
    }
    dashboardClicked();
  }
}

void OverviewFrame::autoOptimizeClicked()
{
  if (Settings::instance().isTrackingMode())
  {
    QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
  }
  else
  {
    if (Settings::instance().getAutoOptimizationStatus() == "enabled")
    {
      Settings::instance().setAutoOptimizationStatus("disabled");
      m_ui->m_autoOptimizeButton->setText(tr("CLICK TO ENABLE"));
      QMessageBox::information(this,
                               tr("Auto Optimization"),
                               tr("Auto Optimization Disabled."),
                               QMessageBox::Ok);
    }
    else
    {
      Settings::instance().setAutoOptimizationStatus("enabled");
      m_ui->m_autoOptimizeButton->setText(tr("CLICK TO DISABLE"));
      QMessageBox::information(this,
                               tr("Auto Optimization"),
                               tr("Auto Optimization Enabled. Your wallet will be optimized automatically every 15 minutes."),
                               QMessageBox::Ok);
    }
  }
}

void OverviewFrame::saveLanguageCurrencyClicked()
{
  QString language;
  if (m_ui->m_russian->isChecked())
  {
    language = "ru";
  }
  else if (m_ui->m_turkish->isChecked())
  {
    language = "tr";
  }
  else if (m_ui->m_chinese->isChecked())
  {
    language = "cn";
  }
  else
  {
    language = "en";
  }
  Settings::instance().setLanguage(language);

  QString currency;
  if (m_ui->m_eur->isChecked())
  {
    currency = "EUR";
  }
  else
  {
    currency = "USD";
  }
  Settings::instance().setCurrentCurrency(currency);

  QMessageBox::information(this,
                           tr("Language and Currency settings saved"),
                           tr("Please restart the wallet for the new settings to take effect."),
                           QMessageBox::Ok);
}

void OverviewFrame::saveConnectionClicked()
{
  QString connectionMode;
  if (m_ui->radioButton->isChecked())
  {
    connectionMode = "remote";
  }
  else if (m_ui->radioButton_2->isChecked())
  {
    connectionMode = "embedded";
  }
  else if (m_ui->radioButton_3->isChecked())
  {
    connectionMode = "autoremote";
  }
  Settings::instance().setConnection(connectionMode);

  QString remoteHost;
  /* If it is a remote connection, commit the entered remote node. There is no validation of the 
     remote node. If the connection is embedded then take no action */
  if (m_ui->radioButton->isChecked())
  {
    remoteHost = m_ui->m_hostEdit->text();
  }
  if (m_ui->radioButton_3->isChecked())
  {
    remoteHost = m_ui->m_hostEdit->text();
  }
  Settings::instance().setCurrentRemoteNode(remoteHost);

  QMessageBox::information(this,
                           tr("Connection settings saved"),
                           tr("Please restart the wallet for the new settings to take effect."),
                           QMessageBox::Ok);
}

void OverviewFrame::rescanClicked()
{
  Q_EMIT rescanSignal();
}

void OverviewFrame::delay()
{
  QTime dieTime = QTime::currentTime().addSecs(2);
  while (QTime::currentTime() < dieTime)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void OverviewFrame::onCustomContextMenu(const QPoint &point)
{
  index = m_ui->m_addressBookView->indexAt(point);
  if (!index.isValid())
    return;
  contextMenu->exec(m_ui->m_addressBookView->mapToGlobal(point));
}

void OverviewFrame::addABClicked()
{
  NewAddressDialog dlg(&MainWindow::instance());
  if (dlg.exec() == QDialog::Accepted)
  {
    QString label = dlg.getLabel();
    QString address = dlg.getAddress();
    QByteArray paymentid = dlg.getPaymentID().toUtf8();
    if (!CurrencyAdapter::instance().validateAddress(address))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid address"), QtCriticalMsg));
      return;
    }

    if (!isValidPaymentId(paymentid))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
      return;
    }

    QModelIndex contactIndex = AddressBookModel::instance().indexFromContact(label, 0);
    QString contactLabel = contactIndex.data(AddressBookModel::ROLE_LABEL).toString();
    if (label == contactLabel)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Contact with such label already exists."), QtCriticalMsg));
      //label = QString(label + "%1").arg(label.toInt()+1);
      NewAddressDialog dlg(&MainWindow::instance());
      dlg.setEditLabel(label);
      dlg.setEditAddress(address);
      dlg.setEditPaymentId(paymentid);
      if (dlg.exec() == QDialog::Accepted)
      {
        QString label = dlg.getLabel();
        QString address = dlg.getAddress();
        QByteArray paymentid = dlg.getPaymentID().toUtf8();
        if (!CurrencyAdapter::instance().validateAddress(address))
        {
          QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid address"), QtCriticalMsg));
          return;
        }

        if (!isValidPaymentId(paymentid))
        {
          QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
          return;
        }

        QModelIndex contactIndex = AddressBookModel::instance().indexFromContact(label, 0);
        QString contactLabel = contactIndex.data(AddressBookModel::ROLE_LABEL).toString();
        if (label == contactLabel)
        {
          QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Contact with such label already exists."), QtCriticalMsg));
          return;
        }
        AddressBookModel::instance().addAddress(label, address, paymentid);
      }
      return;
    }

    AddressBookModel::instance().addAddress(label, address, paymentid);
  }
}

void OverviewFrame::editABClicked()
{
  m_ui->darkness->show();
  m_ui->darkness->raise();
  NewAddressDialog dlg(&MainWindow::instance());
  dlg.setWindowTitle(QString(tr("Edit contact")));
  dlg.setEditLabel(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_LABEL).toString());
  dlg.setEditAddress(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_ADDRESS).toString());
  dlg.setEditPaymentId(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_PAYMENTID).toString());
  if (dlg.exec() == QDialog::Accepted)
  {
    QString label = dlg.getLabel();
    QString address = dlg.getAddress();
    QByteArray paymentid = dlg.getPaymentID().toUtf8();
    if (!CurrencyAdapter::instance().validateAddress(address))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid address"), QtCriticalMsg));
      return;
    }

    if (!isValidPaymentId(paymentid))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
      return;
    }

    QModelIndex contactIndex = AddressBookModel::instance().indexFromContact(label, 0);
    QString contactLabel = contactIndex.data(AddressBookModel::ROLE_LABEL).toString();
    if (label == contactLabel)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Contact with such label already exists."), QtCriticalMsg));
      return;
    }

    AddressBookModel::instance().addAddress(label, address, paymentid);

    deleteABClicked();
  }
  m_ui->darkness->hide();
}

void OverviewFrame::copyABClicked()
{
  QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_ADDRESS).toString());
  QMessageBox::information(this, tr("Address Book"), "Address copied to clipboard");
}

void OverviewFrame::copyABPaymentIdClicked()
{
  QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_PAYMENTID).toString());
  QMessageBox::information(this, tr("Address Book"), "Payment ID copied to clipboard");
}

void OverviewFrame::deleteABClicked()
{
  int row = m_ui->m_addressBookView->currentIndex().row();
  AddressBookModel::instance().removeAddress(row);
  m_ui->m_copyPaymentIdButton->setEnabled(false);
  currentAddressChanged(m_ui->m_addressBookView->currentIndex());
}

void OverviewFrame::payToABClicked()
{
  Q_EMIT payToSignal(m_ui->m_addressBookView->currentIndex());
}

void OverviewFrame::addressDoubleClicked(const QModelIndex &_index)
{
  if (!_index.isValid())
  {
    return;
  }

  Q_EMIT payToSignal(_index);
  m_ui->darkness->hide();
}

void OverviewFrame::currentAddressChanged(const QModelIndex &_index)
{
  m_ui->m_copyAddressButton_2->setEnabled(_index.isValid());
  m_ui->m_deleteAddressButton->setEnabled(_index.isValid());
  m_ui->m_editAddressButton->setEnabled(_index.isValid());
  m_ui->m_copyPaymentIdButton->setEnabled(!_index.data(AddressBookModel::ROLE_PAYMENTID).toString().isEmpty());
}

void OverviewFrame::discordClicked()
{
  QDesktopServices::openUrl(QUrl("http://discord.conceal.network/", QUrl::TolerantMode));
}

void OverviewFrame::twitterClicked()
{
  QDesktopServices::openUrl(QUrl("https://twitter.com/ConcealNetwork", QUrl::TolerantMode));
}

void OverviewFrame::telegramClicked()
{
  QDesktopServices::openUrl(QUrl("https://t.co/55klBHKGUR", QUrl::TolerantMode));
}

void OverviewFrame::githubClicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/ConcealNetwork", QUrl::TolerantMode));
}

void OverviewFrame::redditClicked()
{
  QDesktopServices::openUrl(QUrl("https://www.reddit.com/r/ConcealNetwork/", QUrl::TolerantMode));
}

void OverviewFrame::mediumClicked()
{
  QDesktopServices::openUrl(QUrl("https://medium.com/@ConcealNetwork", QUrl::TolerantMode));
}

void OverviewFrame::hotbitClicked()
{
  QDesktopServices::openUrl(QUrl("https://www.hotbit.io/exchange?symbol=CCX_BTC", QUrl::TolerantMode));
}

void OverviewFrame::stexClicked()
{
  QDesktopServices::openUrl(QUrl("https://app.stex.com/en/basic-trade/pair/BTC/CCX", QUrl::TolerantMode));
}

void OverviewFrame::tradeogreClicked()
{
  QDesktopServices::openUrl(QUrl("https://tradeogre.com/exchange/BTC-CCX", QUrl::TolerantMode));
}

void OverviewFrame::qtradeClicked()
{
  QDesktopServices::openUrl(QUrl("https://qtrade.io/market/CCX_BTC", QUrl::TolerantMode));
}

void OverviewFrame::helpClicked()
{
  QDesktopServices::openUrl(QUrl("https://conceal.network/wiki/doku.php?id=start", QUrl::TolerantMode));
}

bool OverviewFrame::checkWalletPassword()
{

  if (!Settings::instance().isEncrypted() && WalletAdapter::instance().checkWalletPassword(""))
    return true;

  PasswordDialog dlg(false, this);
  if (dlg.exec() == QDialog::Accepted)
  {
    QString password = dlg.getPassword();
    if (!WalletAdapter::instance().checkWalletPassword(password))
    {
      QMessageBox::critical(nullptr, tr("Incorrect password"), tr("Wrong password."), QMessageBox::Ok);
      return false;
    }
    else
    {
      return true;
    }
  }

  return false;
}

void OverviewFrame::lockWallet()
{
  if (QMessageBox::warning(&MainWindow::instance(), tr("Lock Wallet"),
                           tr("Would you like to lock your wallet? While your wallet is locked, it will continue to synchronize with the network. You will need to enter your wallet password to unlock it."),
                           QMessageBox::Cancel,
                           QMessageBox::Ok) != QMessageBox::Ok)
  {
    return;
  }
  m_ui->lockBox->show();
  m_ui->lockBox->raise();
}

void OverviewFrame::unlockWallet()
{
  if (!checkWalletPassword())
  {
    return;
  }
  m_ui->lockBox->hide();
}

void OverviewFrame::encryptWalletClicked()
{
  if (!Settings::instance().isEncrypted())
    return;
  Q_EMIT encryptWalletSignal();
}

void OverviewFrame::scrollToTransaction(const QModelIndex &_index)
{
  transactionClicked();
  QModelIndex sortedModelIndex = SortedTransactionsModel::instance().mapFromSource(_index);
  QModelIndex index = static_cast<QSortFilterProxyModel *>(m_ui->m_transactionsView->model())->mapFromSource(sortedModelIndex);
  m_ui->m_transactionsView->scrollTo(index);
  m_ui->m_transactionsView->setFocus();
  m_ui->m_transactionsView->setCurrentIndex(index);
}

} // namespace WalletGui

#include "OverviewFrame.moc"
