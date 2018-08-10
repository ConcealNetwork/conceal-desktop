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

  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_feeLabel->setText(tr("%1 %2").arg(CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getMinimumFee())).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));

  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal,
    this, &DepositsFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingDepositBalanceUpdatedSignal,
    this, &DepositsFrame::pendingDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &DepositsFrame::reset,
    Qt::QueuedConnection);

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
  WalletAdapter::instance().deposit(term, amount, CurrencyAdapter::instance().getMinimumFee(), 2);
}

/* ------------------------------------------------------------------------------------------- */

void DepositsFrame::depositParamsChanged() {
  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
  quint32 term = m_ui->m_timeSpin->value() * 21900;
  
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestLabel->setText(QString("+ %1 %2 (%3 %)").arg(CurrencyAdapter::instance().formatAmount(interest)).
    arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()).arg(QString::number(termRate * 100, 'f', 2)));
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

}
