// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "CurrencyAdapter.h"
#include "OverviewFrame.h"
#include "TransactionFrame.h"
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include "RecentTransactionsModel.h"
#include "WalletAdapter.h"
#include "PriceProvider.h"
#include "NodeAdapter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

OverviewFrame::OverviewFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame), m_priceProvider(new PriceProvider(this)), m_transactionModel(new RecentTransactionsModel) 
{

  m_ui->setupUi(this);

  int id = QFontDatabase::addApplicationFont(":/fonts/Oswald-Regular.ttf");
  int id2 = QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
  QFont font;
  font.setFamily("Oswald");
  font.setPointSize(18);

  QFont font2;
  font2.setFamily("Open Sans");
  font.setPointSize(9);

  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal,
    this, &OverviewFrame::actualBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingBalanceUpdatedSignal,
    this, &OverviewFrame::pendingBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal,
    this, &OverviewFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingDepositBalanceUpdatedSignal,
    this, &OverviewFrame::pendingDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &OverviewFrame::reset,
    Qt::QueuedConnection);
  connect(m_transactionModel.data(), &QAbstractItemModel::rowsInserted, this, &OverviewFrame::transactionsInserted);
  connect(m_transactionModel.data(), &QAbstractItemModel::layoutChanged, this, &OverviewFrame::layoutChanged);

  connect(&WalletAdapter::instance(), &WalletAdapter::updateWalletAddressSignal, this, &OverviewFrame::updateWalletAddress);
  connect(m_priceProvider, &PriceProvider::priceFoundSignal, this, &OverviewFrame::onPriceFound);  
  
  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel4->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel5->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_recentTransactionsView->setItemDelegate(new RecentTransactionsDelegate(this));
  m_ui->m_recentTransactionsView->setModel(m_transactionModel.data());



  reset();

}

OverviewFrame::~OverviewFrame() {
}

void OverviewFrame::transactionsInserted(const QModelIndex& _parent, int _first, int _last) {
  for (quint32 i = _first; i <= _last; ++i) {
    QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
  }
}

void OverviewFrame::updateWalletAddress(const QString& _address) {
  m_ui->m_myAddress->setText(_address);
}


void OverviewFrame::layoutChanged() {
  for (quint32 i = 0; i <= m_transactionModel->rowCount(); ++i) {
    QModelIndex recent_index = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recent_index);
  }
}

void OverviewFrame::actualBalanceUpdated(quint64 _balance) {
  m_ui->m_actualBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance;
  m_ui->m_totalPortfolioLabel->setText(CurrencyAdapter::instance().formatAmount(totalBalance) + " CCX");  
  m_ui->m_totalBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingBalance) + " CCX");
}

void OverviewFrame::pendingBalanceUpdated(quint64 _balance) {
  m_ui->m_pendingBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  m_ui->m_totalBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualBalance) + " CCX");
}

void OverviewFrame::actualDepositBalanceUpdated(quint64 _balance) {
  m_ui->m_unlockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance;
  m_ui->m_totalPortfolioLabel->setText(CurrencyAdapter::instance().formatAmount(totalBalance) + " CCX");   
  m_ui->m_totalDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingDepositBalance) + " CCX");
}

void OverviewFrame::pendingDepositBalanceUpdated(quint64 _balance) {
  m_ui->m_lockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  m_ui->m_totalDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualDepositBalance) + " CCX");
}

void OverviewFrame::onPriceFound(const QString& _ccxusd, const QString& _ccxbtc, const QString& _btc, const QString& _diff, const QString& _hashrate, const QString& _reward, const QString& _deposits, const QString& _supply) {
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  quint64 totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance;

  m_ui->m_ccxusd->setText("USD " + _ccxusd);

  float ccxusd = _ccxusd.toFloat();
  float total = ccxusd;

  m_ui->m_ccxbtc->setText(_ccxbtc + " satoshi");
  m_ui->m_btc->setText("USD " + _btc);
  m_ui->m_difficulty->setText(_diff + " mn");
  m_ui->m_deposits->setText(_deposits + " CCX");  
  m_ui->m_supply->setText(_supply + " CCX");    
  m_ui->m_hashrate->setText(_hashrate + " KH/s");
  m_ui->m_reward->setText(_reward + " CCX");
  m_ui->m_totalPortfolioLabelUSD->setText("USD " + QString::number(total, 'f', 2)); 
  m_ui->m_height->setText(QString::number(NodeAdapter::instance().getLastKnownBlockHeight()));
}

void OverviewFrame::sendClicked() {
  Q_EMIT sendSignal();
}

void OverviewFrame::depositClicked() {
  Q_EMIT depositSignal();
}

void OverviewFrame::backupClicked() {
  Q_EMIT backupSignal();
}

void OverviewFrame::rescanClicked() {
  Q_EMIT rescanSignal();
}

void OverviewFrame::reset() {
  actualBalanceUpdated(0);
  pendingBalanceUpdated(0);
  actualDepositBalanceUpdated(0);
  pendingDepositBalanceUpdated(0);
  m_priceProvider->getPrice(); 
  m_ui->m_height->setText(QString::number(NodeAdapter::instance().getLastKnownBlockHeight()));
}

}

#include "OverviewFrame.moc"
