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

namespace WalletGui {

  class RecentTransactionsDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    RecentTransactionsDelegate(QObject* _parent) : QStyledItemDelegate(_parent) {
    }
    ~RecentTransactionsDelegate() {
    }

  QWidget* createEditor(QWidget* _parent, const QStyleOptionViewItem& _option, const QModelIndex& _index) const Q_DECL_OVERRIDE 
  {
    if (!_index.isValid()) {
      return nullptr;
    }
    return new TransactionFrame(_index, _parent);
  }

  QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const Q_DECL_OVERRIDE 
  {
    return QSize(346, 64);
  }
};

OverviewFrame::OverviewFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame), m_priceProvider(new PriceProvider(this)), m_transactionModel(new RecentTransactionsModel) 
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
  m_ui->m_currentWalletTitle->setText("SYNCHRONIZATION IS IN PROGRESS");

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

  walletSynced = false;

  /* Hide the second chart */
  int currentChart = 2;
  m_ui->m_chart->show();
  m_ui->m_chart_2->hide();

  /* Pull the chart */
  QNetworkAccessManager *nam = new QNetworkAccessManager(this);
  connect(nam, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished);
  const QUrl url = QUrl::fromUserInput("http://explorer.conceal.network/services/charts/price.png?vsCurrency=usd&days=7&priceDecimals=2&xPoints=12&width=1241&height=281");
  QNetworkRequest request(url);
  nam->get(request);

  /* Pull the alternate chart */
  QNetworkAccessManager *nam2 = new QNetworkAccessManager(this);
  connect(nam2, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished2);
  const QUrl url2 = QUrl::fromUserInput("http://explorer.conceal.network/services/charts/price.png?vsCurrency=btc&days=7&priceDecimals=6&priceSymbol=btc&xPoints=12&width=1241&height=281");
  QNetworkRequest request2(url2);
  nam2->get(request2);

  reset();
}

OverviewFrame::~OverviewFrame() {
}

void OverviewFrame::walletSynchronized(int _error, const QString& _error_text) 
{
  /* Lets enable buttons now that wallet synchronization is complete */  
  m_ui->m_newTransferButton->setStyleSheet("QPushButton#m_newTransferButton {color: #ddd; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;} QPushButton#m_newTransferButton:hover {color: orange; background-color: #212529; border: 0px solid #343a40; font-family: Lato;font-size: 13px;}"); 
  m_ui->m_newDepositButton->setStyleSheet("QPushButton#m_newDepositButton {color: #ddd; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;} QPushButton#m_newDepositButton:hover {color: orange; background-color: #212529; border: 0px solid #343a40; font-family: Lato;font-size: 13px;}"); 
  m_ui->m_newMessageButton->setStyleSheet("QPushButton#m_newMessageButton {color: #ddd; background-color: #212529; border: 0px solid #343a40;font-family: Lato;font-size: 13px;} QPushButton#m_newMessageButton:hover {color: orange; background-color: #212529; border: 0px solid #343a40; font-family: Lato;font-size: 13px;}"); 
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


void OverviewFrame::transactionsInserted(const QModelIndex& _parent, int _first, int _last) 
{
  for (quint32 i = _first; i <= _last; ++i) 
  {
    QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
    m_priceProvider->getPrice(); 
  }
}

void OverviewFrame::updateWalletAddress(const QString& _address) 
{
  m_ui->m_copyAddressButton->setText(_address);
  //m_ui->m_copyAddressButton->setStyleSheet("Text-align:right");
}

void OverviewFrame::showCurrentWallet() 
{
  /* Show the name of the opened wallet */
  QString walletFile = Settings::instance().getWalletName();
  m_ui->m_currentWalletTitle->setText("Wallet: " + walletFile.toUpper());
}

void OverviewFrame::downloadFinished(QNetworkReply *reply) {
    /* Download is done
       set the chart as the pixmap */
    QPixmap pm;
    pm.loadFromData(reply->readAll());
    m_ui->m_chart->setPixmap(pm);
}

void OverviewFrame::downloadFinished2(QNetworkReply *reply2) {
    /* Download is done
       set the chart as the pixmap */
    QPixmap pm2;
    pm2.loadFromData(reply2->readAll());
    m_ui->m_chart_2->setPixmap(pm2);    
}

void OverviewFrame::layoutChanged() {
  for (quint32 i = 0; i <= m_transactionModel->rowCount(); ++i) {
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

void OverviewFrame::onPriceFound(const QString& _btcccx,const QString& _usdccx, const QString& _usdbtc, const QString& _usdmarketcap, const QString& _usdvolume) 
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
  m_ui->m_ccxusd->setText("CCX $" + _usdccx);  
  m_ui->m_btcusd->setText("BTC $" + _usdbtc);
  //m_ui->m_marketCap->setText("$" + _usdmarketcap);
  //m_ui->m_volume->setText("$" + _usdvolume);
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
    Q_EMIT depositSignal();
  } 
  else 
  {
    syncMessage();
  }
}

void OverviewFrame::transactionClicked() 
{
  Q_EMIT transactionSignal();
}

void OverviewFrame::addressBookClicked() 
{
  Q_EMIT addressBookSignal();
}

void OverviewFrame::aboutClicked() 
{
  if (subMenu != 4) {
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
    m_ui->m_subButton1->setText("About Conceal");
    m_ui->m_subButton2->setText("About QT");
    m_ui->m_subButton3->setText("Disclaimer");
    m_ui->m_subButton4->setText("Links");    
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
    m_ui->m_subButton4->setEnabled(false);
    m_ui->m_subButton5->setEnabled(false);    
    m_ui->m_subButton1->setText("Import Seed");
    m_ui->m_subButton3->setText("Import Secret Keys");
    m_ui->m_subButton2->setText("Import GUI Key");
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

void OverviewFrame::settingsClicked() 
{
  if (subMenu != 2) 
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
    m_ui->m_subButton1->setText("Optimize Wallet");
    m_ui->m_subButton2->setText("Connection Settings");
    m_ui->m_subButton3->setText("Rescan Wallet");    

    #ifdef Q_OS_WIN        
      m_ui->m_subButton4->setText("Minimize to Tray");    
      m_ui->m_subButton5->setText("Close to Tray");       

      if (!Settings::instance().isMinimizeToTrayEnabled()) 
      {
        m_ui->m_subButton4->setText("Minimize to Tray Off");    
      } 
      else 
      {
        m_ui->m_subButton4->setText("Minimize to Tray On");    
      } 

      if (!Settings::instance().isCloseToTrayEnabled()) 
      {
        m_ui->m_subButton5->setText("Close to Tray Off");       
      } 
      else 
      {
        m_ui->m_subButton5->setText("Close to Tray On");       
      }
    #endif
    subMenu = 2;
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

void OverviewFrame::walletClicked() {
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
    m_ui->m_subButton1->setText("Open Wallet");
    m_ui->m_subButton2->setText("Create Wallet");
    m_ui->m_subButton3->setText("Backup Wallet");    
    m_ui->m_subButton5->setText("Import Wallet");    
    m_ui->m_subButton6->setText("Close Wallet");  

    if (!Settings::instance().isEncrypted()) 
    {
      m_ui->m_subButton4->setText("Encrypt Wallet");
    } 
    else 
    {
      m_ui->m_subButton4->setText("Change Password");
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
  if (subMenu == 3) 
  {
    Q_EMIT encryptWalletSignal();
  }
  if (subMenu == 4) 
  {
    Q_EMIT linksSignal();
  } 

#ifdef Q_OS_WIN
  if (subMenu == 2) 
  {
    if (!Settings::instance().isMinimizeToTrayEnabled()) 
    {
      Settings::instance().setMinimizeToTrayEnabled(true);
      m_ui->m_subButton4->setText("Minimize to Tray On");    
    } 
    else 
    {
      Settings::instance().setMinimizeToTrayEnabled(false);    
      m_ui->m_subButton4->setText("Minimize to Tray Off");    
    }
  }  
#endif  
}

void OverviewFrame::subButton5Clicked() 
{
#ifdef Q_OS_WIN
  if (subMenu == 2) 
  {
    if (!Settings::instance().isCloseToTrayEnabled()) 
    {
      Settings::instance().setCloseToTrayEnabled(true);
      m_ui->m_subButton5->setText("Close to Tray On");       
    } 
    else 
    {
      Settings::instance().setCloseToTrayEnabled(false);   
      m_ui->m_subButton5->setText("Close to Tray Off");       
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
}

void OverviewFrame::qrCodeClicked() 
{
  Q_EMIT qrSignal(m_ui->m_copyAddressButton->text());
}

void OverviewFrame::messageClicked() 
{
  Q_EMIT messageSignal();
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
    Q_EMIT newTransferSignal();
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
    Q_EMIT newMessageSignal();
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

void OverviewFrame::setStatusBarText(const QString& _text) 
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

}

#include "OverviewFrame.moc"
