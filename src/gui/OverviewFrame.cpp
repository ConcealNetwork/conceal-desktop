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
#include "DepositDetailsDialog.h"
#include "DepositListModel.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"
#include "VisibleMessagesModel.h"
#include "DepositModel.h"
#include "PriceProvider.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "QRLabel.h"
#include "SortedTransactionsModel.h"
#include "TransactionsFrame.h"
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

  dashboardClicked();

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

void OverviewFrame::addressBookClicked()
{
  Q_EMIT addressBookSignal();
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

void OverviewFrame::showTransactionDetails(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }

  TransactionDetailsDialog dlg(_index, this);
  dlg.exec();
}


} // namespace WalletGui

#include "OverviewFrame.moc"
