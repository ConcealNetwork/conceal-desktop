// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "DepositsFrame.h"
#include "DepositDetailsDialog.h"
#include "DepositListModel.h"
#include "DepositModel.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "NodeAdapter.h"
#include "ui_depositsframe.h"

#include <QPainter>
#include <QColor>

namespace WalletGui {

namespace {

QString monthsToBlocks(int _months) {
  QString resTempate("%1 %2");
  if (_months < 13) {
    return resTempate.arg(_months * 21900).arg(QObject::tr("blocks"));
  }
  return QString();
}

}

DepositsFrame::DepositsFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::DepositsFrame), m_depositModel(new DepositListModel) {
  m_ui->setupUi(this);
  m_ui->m_timeSpin->setMinimum(1);
  m_ui->m_timeSpin->setMaximum(12);
  m_ui->m_timeSpin->setSuffix(QString(" %1").arg(tr("Months")));
  m_ui->m_amountSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_amountSpin->setMinimum(CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getDepositMinAmount()).toDouble());
  m_ui->m_amountSpin->setDecimals(CurrencyAdapter::instance().getNumberOfDecimalPlaces());
  m_ui->m_depositView->setModel(m_depositModel.data());
  m_ui->m_depositView->sortByColumn(5, Qt::SortOrder::AscendingOrder); //COLUMN_UNLOCK_HEIGHT, ascending

  m_ui->m_depositView->header()->resizeSection(0, 70);
  m_ui->m_depositView->header()->resizeSection(1, 110);
  m_ui->m_depositView->header()->resizeSection(2, 60);
  m_ui->m_depositView->header()->resizeSection(3, 40);

  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_feeLabel->setText(tr("%1 %2").arg(CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getMinimumFee())).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
  
  m_ui->investmentsBox->hide();  

  QPixmap pixmap(860,290);
  pixmap.fill(QColor("transparent"));

  QPainter painter(&pixmap);
  painter.setBrush(QBrush(QColor(48,48,48,255)));
  painter.setPen(QPen(QColor(48,48,48,255)));
  
  int a = 1;
  int x = 22;
  int y = 256;
  int w = 35;
  int h = 0;

  do {
    x = 32 + (a * 60);
    y = 236 - (a * 5 * 3);
    w = 38;
    h = 0 + (a * 5 * 3);    
    painter.drawRect(x, y, w, h);
    a = a + 1;
  } while( a < 13 );  
  
  m_ui->m_chartBack->setPixmap(pixmap);    
  m_ui->m_bar->setGeometry(92,221,35,15);
  m_ui->m_bar->raise();

  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal, this, &DepositsFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingDepositBalanceUpdatedSignal, this, &DepositsFrame::pendingDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &DepositsFrame::reset, Qt::QueuedConnection);
  reset();
}



DepositsFrame::~DepositsFrame() {
}

void DepositsFrame::actualDepositBalanceUpdated(quint64 _balance) {
  m_ui->m_unlockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
  m_ui->m_totalDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingDepositBalance));
}

void DepositsFrame::pendingDepositBalanceUpdated(quint64 _balance) {
  m_ui->m_lockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
  m_ui->m_totalDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualDepositBalance));
}

void DepositsFrame::reset() {
  actualDepositBalanceUpdated(0);
  pendingDepositBalanceUpdated(0);
}

void DepositsFrame::allButtonClicked() {
  double amount = (double)WalletAdapter::instance().getActualBalance() - (double)CurrencyAdapter::instance().getMinimumFee();
  m_ui->m_amountSpin->setValue(amount / 1000000);
}

/* --------------------------------- CREATE A NEW DEPOSIT ------------------------------------- */

void DepositsFrame::depositClicked() 
{

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());

  if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFee() > WalletAdapter::instance().getActualBalance()) 
  {

    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("You don't have enough balance in your account!"), QtCriticalMsg));
    return;
  }

  quint32 term = m_ui->m_timeSpin->value() * 21900;

  /* initiate the desposit */
  WalletAdapter::instance().deposit(term, amount, CurrencyAdapter::instance().getMinimumFee(), 9);
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::depositParamsChanged() {
  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
  quint32 term = m_ui->m_timeSpin->value() * 21900;
  
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestLabel->setText(QString("+ %1 %2 (%3 %)").arg(CurrencyAdapter::instance().formatAmount(interest)).
    arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()).arg(QString::number(termRate * 100, 'f', 2)));

  /* draw deposit graphs */
  int x = 32 + (m_ui->m_timeSpin->value() * 60);
  int y = 236 - (m_ui->m_timeSpin->value() * 5 * 3);
  int w = 38;
  int h = 0 + (m_ui->m_timeSpin->value() * 5 * 3);
  m_ui->m_bar->setGeometry(x,y,w,h);

}

void DepositsFrame::showDepositDetails(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }

  DepositDetailsDialog dlg(_index, &MainWindow::instance());
  dlg.exec();
}



void DepositsFrame::timeChanged(int _value) {
  m_ui->m_nativeTimeLabel->setText(monthsToBlocks(m_ui->m_timeSpin->value()));
}

void DepositsFrame::withdrawClicked() {
  QModelIndexList unlockedDepositIndexList = DepositModel::instance().match(DepositModel::instance().index(0, 0),
    DepositModel::ROLE_STATE, DepositModel::STATE_UNLOCKED, -1);
  if (unlockedDepositIndexList.isEmpty()) {
    return;
  }

  QVector<CryptoNote::DepositId> depositIds;
  Q_FOREACH (const QModelIndex& index, unlockedDepositIndexList) {
    depositIds.append(index.row());
  }

  WalletAdapter::instance().withdrawUnlockedDeposits(depositIds, CurrencyAdapter::instance().getMinimumFee());
}

void DepositsFrame::backClicked() {
  Q_EMIT backSignal();
}

void DepositsFrame::investmentsClicked() {
  /*
  m_ui->investmentsBox->show();
  m_ui->depositsBox->hide();
  m_ui->m_depositSelectButton->setStyleSheet("color: #777; background-color: #222;"); 
  m_ui->m_investmentSelectButton->setStyleSheet("color: orange; background-color: #222;"); */
}

void DepositsFrame::depositsClicked() {
  /*
  m_ui->investmentsBox->hide();
  m_ui->depositsBox->show();
  m_ui->m_depositSelectButton->setStyleSheet("color: orange; background-color: #222;");
  m_ui->m_investmentSelectButton->setStyleSheet("color: #777; background-color: #222;"); */
}


}
