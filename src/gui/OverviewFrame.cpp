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
#include "PriceProvider.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "QRLabel.h"
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
    return QSize(346, 64);
  }
};

OverviewFrame::OverviewFrame(QWidget *_parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame), m_priceProvider(new PriceProvider(this)), m_transactionModel(new RecentTransactionsModel)
{
  m_ui->setupUi(this);
  /* Load the new app-wide font */
  int id = QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
  QFont font;
  font.setFamily("Lato");
  font.setPointSize(13);

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
  connect(&MessagesModel::instance(), &MessagesModel::poolEarningsSignal, this, &OverviewFrame::poolUpdate);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &OverviewFrame::walletSynchronized, Qt::QueuedConnection);

  /* Initialize basic ui elements */
  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel4->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel5->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_recentTransactionsView->setItemDelegate(new RecentTransactionsDelegate(this));
  m_ui->m_recentTransactionsView->setModel(m_transactionModel.data());

  m_ui->m_newTransferButton->setStyleSheet("color: #444; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;");
  m_ui->m_newMessageButton->setStyleSheet("color: #444; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;");
  m_ui->m_newDepositButton->setStyleSheet("color: #444; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;");

  /* Disable and hide the submenu */
  m_ui->m_subButton1->setText("");
  m_ui->m_subButton2->setText("");
  m_ui->m_subButton3->setText("");
  m_ui->m_subButton4->setText("");
  m_ui->m_subButton5->setText("");
  m_ui->m_subButton6->setText("");
  m_ui->m_subButton1->setEnabled(false);
  m_ui->m_subButton2->setEnabled(false);
  m_ui->m_subButton3->setEnabled(false);
  m_ui->m_subButton4->setEnabled(false);
  m_ui->m_subButton5->setEnabled(false);
  m_ui->m_subButton6->setEnabled(false);
  int subMenu = 0;
  showCurrentWallet();
  walletSynced = false;

  /* Hide the second chart */
  int currentChart = 2;
  m_ui->m_chart->show();
  m_ui->m_chart_2->hide();

  /* Pull the chart */
  QNetworkAccessManager *nam = new QNetworkAccessManager(this);
  connect(nam, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished);
  QString link = "http://walletapi.conceal.network/services/charts/price.png?vsCurrency=" + Settings::instance().getCurrentCurrency() + "&days=7&priceDecimals=2&xPoints=12&width=711&height=241";
  const QUrl url = QUrl::fromUserInput(link);
  QNetworkRequest request(url);
  nam->get(request);

  /* Pull the alternate chart */
  QNetworkAccessManager *nam2 = new QNetworkAccessManager(this);
  connect(nam2, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished2);
  const QUrl url2 = QUrl::fromUserInput("http://walletapi.conceal.network/services/charts/price.png?vsCurrency=btc&days=7&priceDecimals=6&priceSymbol=btc&xPoints=12&width=711&height=241");
  QNetworkRequest request2(url2);
  nam2->get(request2);

  reset();
}

OverviewFrame::~OverviewFrame()
{
}

void OverviewFrame::walletSynchronized(int _error, const QString &_error_text)
{

  if (Settings::instance().isTrackingMode())
  {
    /* Do nothing. The buttons remain dark */
  }
  else
  {
    /* Lets enable buttons now that wallet synchronization is complete */
    m_ui->m_newTransferButton->setStyleSheet("QPushButton#m_newTransferButton {color: #ddd; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;} QPushButton#m_newTransferButton:hover {color: orange; background-color: #212529; border: 0px solid #343a40; font-family: Lato;font-size: 13px;}");
    m_ui->m_newDepositButton->setStyleSheet("QPushButton#m_newDepositButton {color: #ddd; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;} QPushButton#m_newDepositButton:hover {color: orange; background-color: #212529; border: 0px solid #343a40; font-family: Lato;font-size: 13px;}");
    m_ui->m_newMessageButton->setStyleSheet("QPushButton#m_newMessageButton {color: #ddd; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;} QPushButton#m_newMessageButton:hover {color: orange; background-color: #212529; border: 0px solid #343a40; font-family: Lato;font-size: 13px;}");
  }

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
  m_ui->m_copyAddressButton->setText(_address);
  m_ui->m_copyAddressButton->setStyleSheet("Text-align:right");
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

void OverviewFrame::onPriceFound(const QString &_btcccx, const QString &_usdccx, const QString &_usdbtc, const QString &_usdmarketcap, const QString &_usdvolume, const QString &_eurccx, const QString &_eurbtc, const QString &_eurmarketcap, const QString &_eurvolume)
{
  QString currentCurrency = Settings::instance().getCurrentCurrency();
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;

  float total = 0;
  if (currentCurrency == "EUR")
  {
    float ccxeur = _eurccx.toFloat();
    total = ccxeur * (float)totalBalance;
    m_ui->m_ccxusd->setText("€" + _eurccx);
    m_ui->m_btcusd->setText("€" + _eurbtc);
    m_ui->m_marketCap->setText("€" + _eurmarketcap);
    m_ui->m_volume->setText("€" + _eurvolume);
  }
  else
  {
    float ccxusd = _usdccx.toFloat();
    total = ccxusd * (float)totalBalance;
    m_ui->m_ccxusd->setText("$" + _usdccx);
    m_ui->m_btcusd->setText("$" + _usdbtc);
    m_ui->m_marketCap->setText("$" + _usdmarketcap);
    m_ui->m_volume->setText("$" + _usdvolume);
  }

  m_ui->m_totalPortfolioLabelUSD->setText("TOTAL " + CurrencyAdapter::instance().formatAmount(totalBalance) + " CCX | " + CurrencyAdapter::instance().formatCurrencyAmount(total / 10000) + " " + Settings::instance().getCurrentCurrency());
}

void OverviewFrame::aboutClicked()
{
  if (subMenu != 4)
  {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");
    m_ui->m_subButton6->setText("");
    m_ui->m_subButton1->setEnabled(true);
    m_ui->m_subButton2->setEnabled(true);
    m_ui->m_subButton3->setEnabled(true);
    m_ui->m_subButton4->setEnabled(true);
    m_ui->m_subButton5->setEnabled(false);
    m_ui->m_subButton1->setText(tr("About Conceal"));
    m_ui->m_subButton2->setText(tr("About QT"));
    m_ui->m_subButton3->setText(tr("Disclaimer"));
    m_ui->m_subButton4->setText(tr("Links"));
    subMenu = 4;
  }
  else
  {
    m_ui->m_subButton1->setEnabled(false);
    m_ui->m_subButton2->setEnabled(false);
    m_ui->m_subButton3->setEnabled(false);
    m_ui->m_subButton4->setEnabled(false);
    m_ui->m_subButton5->setEnabled(false);
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");
    m_ui->m_subButton6->setText("");
    subMenu = 0;
  }
}

void OverviewFrame::importClicked()
{
  if (subMenu != 1)
  {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");
    m_ui->m_subButton6->setText("");
    m_ui->m_subButton1->setEnabled(true);
    m_ui->m_subButton2->setEnabled(true);
    m_ui->m_subButton3->setEnabled(true);
    m_ui->m_subButton4->setEnabled(true);
    m_ui->m_subButton5->setEnabled(false);
    m_ui->m_subButton1->setText(tr("Import Seed"));
    m_ui->m_subButton3->setText(tr("Import Secret Keys"));
    m_ui->m_subButton2->setText(tr("Import GUI Key"));
    m_ui->m_subButton4->setText(tr("Import Tracking Key"));
    subMenu = 1;
  }
  else
  {
    m_ui->m_subButton1->setEnabled(false);
    m_ui->m_subButton2->setEnabled(false);
    m_ui->m_subButton3->setEnabled(false);
    m_ui->m_subButton4->setEnabled(false);
    m_ui->m_subButton5->setEnabled(false);
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");
    m_ui->m_subButton6->setText("");
    subMenu = 0;
  }
}

void OverviewFrame::walletClicked()
{
  if (subMenu != 3)
  {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");
    m_ui->m_subButton6->setText("");
    m_ui->m_subButton1->setEnabled(true);
    m_ui->m_subButton2->setEnabled(true);
    m_ui->m_subButton3->setEnabled(true);
    m_ui->m_subButton4->setEnabled(true);
    m_ui->m_subButton5->setEnabled(true);
    m_ui->m_subButton6->setEnabled(true);
    m_ui->m_subButton1->setText(tr("Open Wallet"));
    m_ui->m_subButton2->setText(tr("Create Wallet"));
    m_ui->m_subButton3->setText(tr("Backup Wallet"));
    m_ui->m_subButton5->setText(tr("Import Wallet"));
    m_ui->m_subButton6->setText(tr("Close Wallet"));

    if (!Settings::instance().isEncrypted())
    {
      m_ui->m_subButton4->setText(tr("Encrypt Wallet"));
    }
    else
    {
      m_ui->m_subButton4->setText(tr("Change Password"));
    }

    subMenu = 3;
  }
  else
  {
    m_ui->m_subButton1->setEnabled(false);
    m_ui->m_subButton2->setEnabled(false);
    m_ui->m_subButton3->setEnabled(false);
    m_ui->m_subButton4->setEnabled(false);
    m_ui->m_subButton5->setEnabled(false);
    m_ui->m_subButton6->setEnabled(false);
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");
    m_ui->m_subButton6->setText("");
    subMenu = 0;
  }
}

void OverviewFrame::settingsClicked()
{
  Q_EMIT settingsSignal();
}

void OverviewFrame::sendClicked()
{
  if (walletSynced == true)
  {
    if (Settings::instance().isTrackingMode())
    {
      trackingMessage();
    }
    else
    {
      Q_EMIT sendSignal();
    }
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
    if (Settings::instance().isTrackingMode())
    {
      trackingMessage();
    }
    else
    {
      Q_EMIT depositSignal();
    }
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::transactionClicked()
{
  if (walletSynced == true)
  {
    Q_EMIT transactionSignal();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::addressBookClicked()
{
  Q_EMIT addressBookSignal();
}

void OverviewFrame::subButton1Clicked()
{
  if (subMenu == 2)
  {
    if (walletSynced == true)
    {
      Q_EMIT optimizeSignal();
    }
    else
    {
      syncMessage();
    }
  }
  if (subMenu == 1)
  {
    Q_EMIT importSeedSignal();
  }
  if (subMenu == 3)
  {
    Q_EMIT openWalletSignal();
  }
  if (subMenu == 4)
  {
    Q_EMIT aboutSignal();
  }
}

void OverviewFrame::subButton2Clicked()
{
  if (subMenu == 2)
  {
    Q_EMIT connectionSettingsSignal();
  }
  if (subMenu == 1)
  {
    Q_EMIT importGUIKeySignal();
  }
  if (subMenu == 3)
  {
    Q_EMIT newWalletSignal();
  }
  if (subMenu == 4)
  {
    Q_EMIT aboutQTSignal();
  }
}

void OverviewFrame::subButton3Clicked()
{
  if (subMenu == 2)
  {
    Q_EMIT rescanSignal();
  }
  if (subMenu == 1)
  {
    Q_EMIT importSecretKeysSignal();
  }
  if (subMenu == 3)
  {
    Q_EMIT backupSignal();
  }
  if (subMenu == 4)
  {
    Q_EMIT disclaimerSignal();
  }
}

void OverviewFrame::subButton4Clicked()
{
  if (subMenu == 1)
  {
    Q_EMIT importTrackingKeySignal();
  }
  if (subMenu == 3)
  {
    Q_EMIT encryptWalletSignal();
  }
  if (subMenu == 4)
  {
    Q_EMIT linksSignal();
  }
  if (subMenu == 2)
  {
    Q_EMIT languageSettingsSignal();
  }
}

void OverviewFrame::subButton5Clicked()
{
#ifdef Q_OS_WIN
  if (subMenu == 2)
  {
    if (!Settings::instance().isCloseToTrayEnabled())
    {
      Settings::instance().setCloseToTrayEnabled(true);
      m_ui->m_subButton5->setText(tr("Close to Tray On"));
    }
    else
    {
      Settings::instance().setCloseToTrayEnabled(false);
      m_ui->m_subButton5->setText(tr("Close to Tray Off"));
    }
  }
#endif
  if (subMenu == 3)
  {
    OverviewFrame::importClicked();
  }
}

void OverviewFrame::subButton6Clicked()
{
  if (subMenu == 3)
  {
    OverviewFrame::closeWalletClicked();
  }

#ifdef Q_OS_WIN
  if (subMenu == 2)
  {
    if (!Settings::instance().isMinimizeToTrayEnabled())
    {
      Settings::instance().setMinimizeToTrayEnabled(true);
      m_ui->m_subButton6->setText(tr("Minimize to Tray On"));
    }
    else
    {
      Settings::instance().setMinimizeToTrayEnabled(false);
      m_ui->m_subButton6->setText(tr("Minimize to Tray Off"));
    }
  }
#endif
}

void OverviewFrame::qrCodeClicked()
{
  Q_EMIT qrSignal(m_ui->m_copyAddressButton->text());
}

void OverviewFrame::messageClicked()
{
  if (walletSynced == true)
  {
    Q_EMIT messageSignal();
  }
  else
  {
    syncMessage();
  }
}

void OverviewFrame::newWalletClicked()
{
  Q_EMIT newWalletSignal();
}

void OverviewFrame::closeWalletClicked()
{
  Q_EMIT closeWalletSignal();
}

void OverviewFrame::newMessageClicked()
{
  if (walletSynced == true)
  {
    if (Settings::instance().isTrackingMode())
    {
      trackingMessage();
    }
    else
    {
      Q_EMIT newMessageSignal();
    }
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

void OverviewFrame::poolUpdate(quint64 _dayPoolAmount, quint64 _totalPoolAmount)
{
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

void OverviewFrame::trackingMessage()
{
  QMessageBox::information(this, tr("Tracking Wallet"), "This is a tracking wallet. This action is not available.");
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

<<<<<<< HEAD
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
    m_ui->m_newTransferButton->hide();
    m_ui->m_newMessageButton->hide();
  }
  else
  {
    m_ui->m_addressMessageEdit->setText(_address);
    m_ui->m_myConcealWalletTitle->setText("SEND MESSAGE");
    m_ui->newMessageBox->raise();
    m_ui->m_newTransferButton->hide();
    m_ui->m_newMessageButton->hide();
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
  m_ui->m_newTransferButton->hide();
  m_ui->m_newMessageButton->hide();
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

void OverviewFrame::recalculateMessageLength()
{
  QString messageText = m_ui->m_messageTextEdit->toPlainText();
  quint32 messageSize = messageText.length();
  if (messageSize > 0)
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
  if (messageSize > 0)
  {
    --messageSize;
  }

  if (messageSize > 260)
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
  if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFeeBanking() > WalletAdapter::instance().getActualBalance())
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
  m_ui->m_interestRateLabel->setText(QString("%3 %)").arg(QString::number(termRate * 100, 'f', 4)));
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
    while (WalletAdapter::instance().getNumUnlockedOutputs() > 100)
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

void OverviewFrame::bitcointalkClicked()
{
  QDesktopServices::openUrl(QUrl("https://bitcointalk.org/index.php?topic=5086106", QUrl::TolerantMode));
}

void OverviewFrame::mediumClicked()
{
  QDesktopServices::openUrl(QUrl("https://medium.com/@ConcealNetwork", QUrl::TolerantMode));
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

=======
>>>>>>> parent of 01fbc44... Merge pull request #83 from ConcealNetwork/600
} // namespace WalletGui

#include "OverviewFrame.moc"
