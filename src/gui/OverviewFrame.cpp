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

  QWidget* createEditor(QWidget* _parent, const QStyleOptionViewItem& _option, const QModelIndex& _index) const Q_DECL_OVERRIDE {

    if (!_index.isValid()) {
      return nullptr;
    }
    return new TransactionFrame(_index, _parent);
  }

  QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const Q_DECL_OVERRIDE {
    return QSize(346, 64);
  }
};

OverviewFrame::OverviewFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame), m_priceProvider(new PriceProvider(this)), m_transactionModel(new RecentTransactionsModel) {

  m_ui->setupUi(this);

  /* load the new app-wide font */
  int id = QFontDatabase::addApplicationFont(":/fonts/Poppins-Regular.ttf");
  QFont font;
  font.setFamily("Poppins");
  font.setPointSize(12);

  /* connect signals */
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


  /* initialize basic ui elements */
  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel4->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel5->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_recentTransactionsView->setItemDelegate(new RecentTransactionsDelegate(this));
  m_ui->m_recentTransactionsView->setModel(m_transactionModel.data());

  /* disable the submenu */
  m_ui->m_subButton1->setText("");
  m_ui->m_subButton2->setText("");
  m_ui->m_subButton3->setText("");
  m_ui->m_subButton4->setText("");
  m_ui->m_subButton5->setText("");     
  m_ui->m_subButton1->setEnabled(false);
  m_ui->m_subButton2->setEnabled(false);
  m_ui->m_subButton3->setEnabled(false);
  m_ui->m_subButton4->setEnabled(false);
  m_ui->m_subButton5->setEnabled(false);
  int subMenu = 0;

  /* pull the chart from the website */
  QNetworkAccessManager *nam = new QNetworkAccessManager(this);
  connect(nam, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished);
  const QUrl url = QUrl::fromUserInput("http://explorer.conceal.network/q/maple/chart2.php");
  QNetworkRequest request(url);
  nam->get(request);

  showCurrentWallet();
  reset();

}

OverviewFrame::~OverviewFrame() {
}

void OverviewFrame::transactionsInserted(const QModelIndex& _parent, int _first, int _last) {

  for (quint32 i = _first; i <= _last; ++i) {
    QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
    m_priceProvider->getPrice(); 
  }
}

void OverviewFrame::updateWalletAddress(const QString& _address) {

  m_ui->m_myAddress->setText(_address);
  showCurrentWallet();
}


void OverviewFrame::showCurrentWallet() {

  /* show the name of the open wallet */
  QString walletFile = Settings::instance().getWalletFile();
  std::string wallet = walletFile.toStdString();

  /* Remove directory if present.
  Do this before extension removal incase directory has a period character. */
  const size_t last_slash_idx = wallet.find_last_of("\\/");
  if (std::string::npos != last_slash_idx) {
      wallet.erase(0, last_slash_idx + 1);
  }

  /*  remove extension if present */
  const size_t period_idx = wallet.rfind('.');
  if (std::string::npos != period_idx) {
      wallet.erase(period_idx);
  }

  /* back to QString and display */
  walletFile = QString::fromStdString(wallet);
  m_ui->m_currentWalletTitle->setText(walletFile);
}


void OverviewFrame::downloadFinished(QNetworkReply *reply) {

    /* download is done
    set the chart as the pixmap */
    QPixmap pm;
    pm.loadFromData(reply->readAll());
    m_ui->m_chart->setPixmap(pm);
}

void OverviewFrame::layoutChanged() {

  for (quint32 i = 0; i <= m_transactionModel->rowCount(); ++i) {
    QModelIndex recent_index = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recent_index);
    m_priceProvider->getPrice();     
  }
  showCurrentWallet();
}

void OverviewFrame::actualBalanceUpdated(quint64 _balance) {

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

void OverviewFrame::pendingBalanceUpdated(quint64 _balance) {

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

void OverviewFrame::actualDepositBalanceUpdated(quint64 _balance) {

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

void OverviewFrame::actualInvestmentBalanceUpdated(quint64 _balance) {

  m_ui->m_unlockedInvestmentsLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();  
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_totalInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingInvestmentBalance) + " CCX");
  m_priceProvider->getPrice(); 
}

void OverviewFrame::pendingDepositBalanceUpdated(quint64 _balance) {

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

void OverviewFrame::pendingInvestmentBalanceUpdated(quint64 _balance) {

  m_ui->m_lockedInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();  
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  m_ui->m_totalInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualInvestmentBalance) + " CCX");
  m_priceProvider->getPrice(); 
}

void OverviewFrame::onPriceFound(const QString& _ccxusd, const QString& _ccxbtc, const QString& _btc, const QString& _diff, const QString& _hashrate, const QString& _reward, const QString& _deposits, const QString& _supply) {

  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();  
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
  float ccxusd = _ccxusd.toFloat();
  float total = ccxusd * (float)totalBalance;
  m_ui->m_totalPortfolioLabelUSD->setText("TOTAL " + CurrencyAdapter::instance().formatAmount(totalBalance) + " CCX " + QString::number(total / 1000000, 'f', 2) + " USD"); 
  m_ui->m_ccxusd->setText(_ccxusd);  
  m_ui->m_btcusd->setText(_btc);

}

void OverviewFrame::sendClicked() {
  
  Q_EMIT sendSignal();
}

void OverviewFrame::depositClicked() {

  Q_EMIT depositSignal();
}

void OverviewFrame::transactionClicked() {

  Q_EMIT transactionSignal();
}

void OverviewFrame::addressBookClicked() {

  Q_EMIT addressBookSignal();
}

void OverviewFrame::aboutClicked() {

  if (subMenu != 4) {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");        
    m_ui->m_subButton1->setEnabled(true);
    m_ui->m_subButton2->setEnabled(true);
    m_ui->m_subButton3->setEnabled(false);
    m_ui->m_subButton4->setEnabled(false);
    m_ui->m_subButton5->setEnabled(false);    
    m_ui->m_subButton1->setText("About Conceal");
    m_ui->m_subButton2->setText("About QT");
    subMenu = 4;
  } else {
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
    subMenu = 0;
  }
}

void OverviewFrame::importClicked() {

  if (subMenu != 1) {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");        
    m_ui->m_subButton1->setEnabled(true);
    m_ui->m_subButton2->setEnabled(true);
    m_ui->m_subButton3->setEnabled(true);
    m_ui->m_subButton4->setEnabled(false);
    m_ui->m_subButton5->setEnabled(false);    
    m_ui->m_subButton1->setText("Import Seed");
    m_ui->m_subButton3->setText("Import Secret Keys");
    m_ui->m_subButton2->setText("Import GUI Key");
    subMenu = 1;
  } else {
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
    subMenu = 0;
  }
}

void OverviewFrame::settingsClicked() {

  if (subMenu != 2) {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");  
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

      if (!Settings::instance().isMinimizeToTrayEnabled()) {
        m_ui->m_subButton4->setText("Minimize to Tray Off");    
      } else {
        m_ui->m_subButton4->setText("Minimize to Tray On");    
      } 

      if (!Settings::instance().isCloseToTrayEnabled()) {
        m_ui->m_subButton5->setText("Close to Tray Off");       
      } else {
        m_ui->m_subButton5->setText("Close to Tray On");       
      }
    #endif
    subMenu = 2;
  } else  {
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
    subMenu = 0;
  }
}

void OverviewFrame::walletClicked() {

  if (subMenu != 3) {
    m_ui->m_subButton1->setText("");
    m_ui->m_subButton2->setText("");
    m_ui->m_subButton3->setText("");
    m_ui->m_subButton4->setText("");
    m_ui->m_subButton5->setText("");  
    m_ui->m_subButton1->setEnabled(true);
    m_ui->m_subButton2->setEnabled(true);
    m_ui->m_subButton3->setEnabled(true);
    m_ui->m_subButton4->setEnabled(true);    
    m_ui->m_subButton5->setEnabled(true);
    m_ui->m_subButton1->setText("Open Wallet");
    m_ui->m_subButton2->setText("Create Wallet");
    m_ui->m_subButton3->setText("Backup Wallet");    
    m_ui->m_subButton5->setText("Import Wallet");    

    if (!Settings::instance().isEncrypted()) {
      m_ui->m_subButton4->setText("Encrypt Wallet");
    } else {
      m_ui->m_subButton4->setText("Change Password");
    }

    subMenu = 3;
    } 
    
    else {
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
    subMenu = 0;
  }
}

void OverviewFrame::subButton1Clicked() {

  if (subMenu == 2) {
    Q_EMIT optimizeSignal();
  }

  if (subMenu == 1) {
    Q_EMIT importSeedSignal();  
  }
  
  if (subMenu == 3) {
    Q_EMIT openWalletSignal();
  }

  if (subMenu == 4) {
    Q_EMIT aboutSignal();
  }  

}

void OverviewFrame::subButton2Clicked() {

  if (subMenu == 2) {
    Q_EMIT connectionSettingsSignal();
  }
  if (subMenu == 1) {
    Q_EMIT importGUIKeySignal();  
  }  
  if (subMenu == 3) {
    Q_EMIT newWalletSignal();
  }
  if (subMenu == 4) {
    Q_EMIT aboutQTSignal();
  }

}

void OverviewFrame::subButton3Clicked() {

  if (subMenu == 2) {
    Q_EMIT rescanSignal();
  }
  if (subMenu == 1) {
    Q_EMIT importSecretKeysSignal();  
  }  
  if (subMenu == 3) {
    Q_EMIT backupSignal();
  }  
}

void OverviewFrame::subButton4Clicked() {

  if (subMenu == 3) {
    Q_EMIT encryptWalletSignal();
  }

#ifdef Q_OS_WIN
  if (subMenu == 2) {
    if (!Settings::instance().isMinimizeToTrayEnabled()) {
      Settings::instance().setMinimizeToTrayEnabled(true);
      m_ui->m_subButton4->setText("Minimize to Tray On");    
    } else {
      Settings::instance().setMinimizeToTrayEnabled(false);    
      m_ui->m_subButton4->setText("Minimize to Tray Off");    
    }
  }  
#endif  
}

void OverviewFrame::subButton5Clicked() 
{
#ifdef Q_OS_WIN
  if (subMenu == 2) {
    if (!Settings::instance().isCloseToTrayEnabled()) {
      Settings::instance().setCloseToTrayEnabled(true);
      m_ui->m_subButton5->setText("Close to Tray On");       
    } else {
      Settings::instance().setCloseToTrayEnabled(false);   
      m_ui->m_subButton5->setText("Close to Tray Off");       
    }
  }
#endif
  if (subMenu == 3) {
    OverviewFrame::importClicked();
  }
}

void OverviewFrame::qrCodeClicked() {

  Q_EMIT qrSignal(m_ui->m_myAddress->text());
}

void OverviewFrame::miningClicked() {

  Q_EMIT miningSignal();
}

void OverviewFrame::messageClicked() {

  Q_EMIT messageSignal();
}

void OverviewFrame::newWalletClicked() {

  Q_EMIT newWalletSignal();
}

void OverviewFrame::newTransferClicked() {

  Q_EMIT newTransferSignal();
}

void OverviewFrame::newMessageClicked() {

  Q_EMIT newMessageSignal();
}

void OverviewFrame::reset() {

  actualBalanceUpdated(0);
  pendingBalanceUpdated(0);
  actualDepositBalanceUpdated(0);
  pendingDepositBalanceUpdated(0);
  actualInvestmentBalanceUpdated(0);
  pendingInvestmentBalanceUpdated(0);
  m_ui->m_actualBalanceLabel_3->setText(CurrencyAdapter::instance().formatAmount(0));
  m_priceProvider->getPrice(); 
  Q_EMIT resetWalletSignal();
}

void OverviewFrame::setStatusBarText(const QString& _text) {
  m_ui->m_statusBox->setText(_text);
} 

void OverviewFrame::poolUpdate(quint64 _dayPoolAmount, quint64 _totalPoolAmount) {

  m_ui->m_actualBalanceLabel_3->setText(CurrencyAdapter::instance().formatAmount(_dayPoolAmount));
  m_ui->m_pendingBalanceLabel_3->setText(CurrencyAdapter::instance().formatAmount(_totalPoolAmount));
}

void OverviewFrame::copyClicked() {

    QApplication::clipboard()->setText(m_ui->m_myAddress->text());
    QMessageBox::information(this, tr("Wallet"), "Address copied to clipboard");
}

}

#include "OverviewFrame.moc"
