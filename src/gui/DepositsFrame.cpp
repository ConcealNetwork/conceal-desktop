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

#include <QMessageBox>
#include <QPainter>
#include <QColor>

namespace WalletGui {

namespace {

  QString weeksToBlocks(int _weeks) {
    QString resTempate("%1 %2");
    if (_weeks < 53) {
      return resTempate.arg(_weeks * 5040).arg(QObject::tr("blocks"));
    }
    return QString();
  }

  QString quartersToBlocks(int _quarters) {
    QString resTempate("%1 %2");
    if (_quarters < 21) {
      return resTempate.arg(_quarters * 64800).arg(QObject::tr("blocks"));
    }
    return QString();
  }

}

/* ------------------------------------------------------------------------------------------- */

DepositsFrame::DepositsFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::DepositsFrame), m_depositModel(new DepositListModel) {

  m_ui->setupUi(this);
  m_ui->m_timeSpin->setMinimum(1);
  m_ui->m_timeSpin->setMaximum(52);
  m_ui->m_timeSpin->setSuffix(QString(" %1").arg(tr("Weeks")));
  m_ui->m_timeSpin2->setMinimum(1);
  m_ui->m_timeSpin2->setMaximum(20);
  m_ui->m_timeSpin2->setSuffix(QString(" %1").arg(tr("Quarters")));
  m_ui->m_amountSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_depositView->setModel(m_depositModel.data());
  m_ui->m_depositView->sortByColumn(5, Qt::SortOrder::AscendingOrder); //COLUMN_UNLOCK_HEIGHT, ascending
  m_ui->m_depositView->header()->resizeSection(0, 70);
  m_ui->m_depositView->header()->resizeSection(1, 110);
  m_ui->m_depositView->header()->resizeSection(2, 60);
  m_ui->m_depositView->header()->resizeSection(3, 40);
  m_ui->m_unlockedInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(0));
  m_ui->m_unlockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(0));
  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_feeLabel->setText(tr("%1 %2").arg(CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getMinimumFeeBanking())).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
  m_ui->m_feeLabel2->setText(tr("%1 %2").arg(CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getMinimumFeeBanking())).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
  
  m_ui->investmentsBox->hide();  
  
  /* pixmap for deposits */
  QPixmap pixmap(860,290);
  pixmap.fill(QColor("transparent"));

  QPainter painter(&pixmap);
  painter.setBrush(QBrush(QColor(48,48,48,255)));
  painter.setPen(QPen(QColor(48,48,48,255)));

  int a = 1;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;

  do {
    x = 11 + (a * 15);
    y = 166 - (a * 2);
    w = 10;
    h = 80 + (a * 2);    
    painter.drawRect(x, y, w, h);
    a = a + 1;
  } while( a < 53 );  
  
  m_ui->m_chartBack->setPixmap(pixmap);    
  m_ui->m_bar->setGeometry(47,164,10,82);
  m_ui->m_bar->raise();

  DepositsFrame::depositParamsChanged();
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal, this, &DepositsFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualInvestmentBalanceUpdatedSignal, this, &DepositsFrame::actualInvestmentBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &DepositsFrame::reset, Qt::QueuedConnection);
  reset();
}

/* ------------------------------------------------------------------------------------------- */

DepositsFrame::~DepositsFrame() {
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::actualDepositBalanceUpdated(quint64 _balance) {

  m_ui->m_unlockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::actualInvestmentBalanceUpdated(quint64 _balance) {

  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  m_ui->m_unlockedInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(actualInvestmentBalance));
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::reset() {

  actualDepositBalanceUpdated(0);
  actualInvestmentBalanceUpdated(0);
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::allButtonClicked() {

  double amount = (double)WalletAdapter::instance().getActualBalance() - (double)CurrencyAdapter::instance().getMinimumFeeBanking();
  int wholeAmount = (int)(amount/1000000);
  m_ui->m_amountSpin->setValue(wholeAmount);
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::depositClicked() {

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());

  /* deposits 2.0 launch */
  if (NodeAdapter::instance().getLastKnownBlockHeight() < 108000) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("New deposits only work after height 108,000"), QtCriticalMsg));
    return;
  }

  /* insufficient funds */
  if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFeeBanking() > WalletAdapter::instance().getActualBalance()) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("You don't have enough balance in your account!"), QtCriticalMsg));
    return;
  }

  /* a week is set as 5040 blocks */
  quint32 term = m_ui->m_timeSpin->value() * 5040;

  /* warn the user */
  if (QMessageBox::warning(&MainWindow::instance(), tr("Deposit Confirmation"),
    tr("Please note that once funds are locked in a deposit, you will not have access until maturity. Are you sure you want to proceed?"), 
    QMessageBox::Cancel, 
    QMessageBox::Ok) != QMessageBox::Ok) {
    return;
  }

  /* initiate the desposit */
  WalletAdapter::instance().deposit(term, amount, CurrencyAdapter::instance().getMinimumFeeBanking(), 4);
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::investmentClicked() {

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin2->cleanText());

  /* investments 1.0 launch */
  if (NodeAdapter::instance().getLastKnownBlockHeight() < 108000) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("New investments only work after height 108,000"), QtCriticalMsg));
    return;
  }

  /* insufficient funds */
  if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFeeBanking() > WalletAdapter::instance().getActualBalance()) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("You don't have enough balance in your account!"), QtCriticalMsg));
    return;
  }

  /* a quarter is set as 64800 blocks */
  quint32 term = m_ui->m_timeSpin2->value() * 64800;

  /* warn the user */
  if (QMessageBox::warning(&MainWindow::instance(), tr("Investment Confirmation"),
    tr("Please note that once funds are locked in an investment, you will not have access to those funds until maturity. Are you sure you want to proceed?"), 
    QMessageBox::Cancel, 
    QMessageBox::Ok) != QMessageBox::Ok) {
    return;
  }
 
  /* initiate the investment */
  WalletAdapter::instance().deposit(term, amount, CurrencyAdapter::instance().getMinimumFeeBanking(), 4);
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::depositParamsChanged() {

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
  quint32 term = m_ui->m_timeSpin->value() * 5040;
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestLabel->setText(QString("+ %1 %2 (%3 %)").arg(CurrencyAdapter::instance().formatAmount(interest)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()).arg(QString::number(termRate * 100, 'f', 4)));

  /* draw bar */
  int x = 11 + (m_ui->m_timeSpin->value() * 15);
  int y = 166 - (m_ui->m_timeSpin->value() * 2);
  int w = 10;
  int h = 80 + (m_ui->m_timeSpin->value() * 2);
  m_ui->m_bar->setGeometry(x,y,w,h);
  m_ui->m_bar->raise();
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::showDepositDetails(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }
  DepositDetailsDialog dlg(_index, &MainWindow::instance());
  dlg.exec();
} 

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::investmentParamsChanged() {

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin2->cleanText());
  quint32 term = m_ui->m_timeSpin2->value() * 64800;
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestLabel2->setText(QString("+ %1 %2 (%3 %)").arg(CurrencyAdapter::instance().formatAmount(interest)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()).arg(QString::number(termRate * 100, 'f', 4)));

  /* draw bar */
  int x = -10 + (m_ui->m_timeSpin2->value() * 39);
  int y = 166 - (m_ui->m_timeSpin2->value() * 2);
  int w = 36;
  int h = 80 + (m_ui->m_timeSpin2->value() * 2);
  m_ui->m_bar->setGeometry(x,y,w,h);
  m_ui->m_bar->raise();
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::timeChanged(int _value) {

  m_ui->m_nativeTimeLabel->setText(weeksToBlocks(m_ui->m_timeSpin->value()));
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::timeChanged2(int _value) {

  m_ui->m_nativeTimeLabel2->setText(quartersToBlocks(m_ui->m_timeSpin2->value()));
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::withdrawClicked() {

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

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::backClicked() {
  /* back to overview frame */
  Q_EMIT backSignal();
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::investmentsClicked() {
  /* switch to investment graph and box */
  m_ui->investmentsBox->show();
  m_ui->depositsBox->hide();
  m_ui->m_depositSelectButton->setStyleSheet("color: #777; background-color: #212529;font-size: 18px;"); 
  m_ui->m_investmentSelectButton->setStyleSheet("color: orange; background-color: #212529;font-size: 18px;"); 
  m_ui->label_10->setText("INTEREST % PER QUARTER");
  m_ui->label_12->setText("Quarters");  

  /* pixmap for investments */
  QPixmap pixmap2(860,290);
  pixmap2.fill(QColor("transparent"));
  QPainter painter2(&pixmap2);
  painter2.setBrush(QBrush(QColor(48,48,48,255)));
  painter2.setPen(QPen(QColor(48,48,48,255))); 
  int a = 1;
  int x = -19;
  int y = 256;
  int w = 36;
  int h = 0;

  do {
    x = -10 + (a * 39);
    y = 166 - (a * 2);
    w = 36;
    h = 80 + (a * 2);    
    painter2.drawRect(x, y, w, h);
    a = a + 1;
  } while( a < 21 );  

  m_ui->m_chartBack->setPixmap(pixmap2);   
  DepositsFrame::investmentParamsChanged();
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::depositsClicked() {
  /* switch to deposit graph and box */
  m_ui->investmentsBox->hide();
  m_ui->depositsBox->show();
  m_ui->m_depositSelectButton->setStyleSheet("color: orange; background-color: #212529; font-size: 18px;");
  m_ui->m_investmentSelectButton->setStyleSheet("color: #777; background-color: #212529; font-size: 18px;"); 
  m_ui->label_10->setText("INTEREST % PER WEEK");
  m_ui->label_12->setText("Weeks");  

  /* pixmap for deposits */
  QPixmap pixmap(860,290);
  pixmap.fill(QColor("transparent"));
  QPainter painter(&pixmap);
  painter.setBrush(QBrush(QColor(48,48,48,255)));
  painter.setPen(QPen(QColor(48,48,48,255)));
  int a = 1;
  int x = -19;
  int y = 256;
  int w = 36;
  int h = 0;

  /* draw bars */
  do {
    x = 11 + (a * 15);
    y = 166 - (a * 2);
    w = 10;
    h = 80 + (a * 2);    
    painter.drawRect(x, y, w, h);
    a = a + 1;
  } while( a < 53 );  
  
  m_ui->m_chartBack->setPixmap(pixmap);    
  m_ui->m_bar->setGeometry(47,164,10,82);
  m_ui->m_bar->raise();
  DepositsFrame::depositParamsChanged();
}

}
