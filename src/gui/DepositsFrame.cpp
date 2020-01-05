// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "AddressProvider.h"
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
#include "Settings.h"

#include <QMessageBox>
#include <QPainter>
#include <QColor>

namespace WalletGui
{

namespace
{
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

} // namespace

DepositsFrame::DepositsFrame(QWidget *_parent) : QFrame(_parent), m_ui(new Ui::DepositsFrame), m_depositModel(new DepositListModel)
{
  m_ui->setupUi(this);
  m_ui->m_timeSpin->setMinimum(1);
  m_ui->m_timeSpin->setMaximum(12);
  m_ui->m_timeSpin->setSuffix(QString(" %1").arg(tr("Month(s)")));
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
  m_ui->m_feeLabel->setText(tr("%1 %2").arg(CurrencyAdapter::instance().formatAmount(1000)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));

  QString fee_address = Settings::instance().getCurrentFeeAddress();
  QString connection = Settings::instance().getConnection();
  if (connection.compare("autoremote") == 0)
  {
    m_ui->m_feeLabel->setText(tr("%1 + 0.01 (Node Fee) %2").arg(CurrencyAdapter::instance().formatAmount(1000)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
  }
  else if (connection.compare("remote") == 0)
  {
    if (!fee_address.isEmpty())
    {
      m_ui->m_feeLabel->setText(tr("%1 + 0.01 (Node Fee) %2").arg(CurrencyAdapter::instance().formatAmount(1000)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
    }
  }

  DepositsFrame::depositParamsChanged();
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal, this, &DepositsFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualInvestmentBalanceUpdatedSignal, this, &DepositsFrame::actualInvestmentBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &DepositsFrame::reset, Qt::QueuedConnection);
  reset();
}

DepositsFrame::~DepositsFrame()
{
}

/* Update the label when the deposit balance changes */
void DepositsFrame::actualDepositBalanceUpdated(quint64 _balance)
{
  m_ui->m_unlockedDepositLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
}

/* Update the label when the investment balance changes */
void DepositsFrame::actualInvestmentBalanceUpdated(quint64 _balance)
{
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  m_ui->m_unlockedInvestmentLabel->setText(CurrencyAdapter::instance().formatAmount(actualInvestmentBalance));
}

/* Reset totals */
void DepositsFrame::reset()
{
  actualDepositBalanceUpdated(0);
  actualInvestmentBalanceUpdated(0);
}

/* Select all funds button */
void DepositsFrame::allButtonClicked()
{
  double amount = (double)WalletAdapter::instance().getActualBalance() - (double)CurrencyAdapter::instance().getMinimumFeeBanking();
  int wholeAmount = (int)(amount / 1000000);
  m_ui->m_amountSpin->setValue(wholeAmount);
}

/* New deposit */
void DepositsFrame::depositClicked()
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
    DepositsFrame::remote_node_fee_address = Settings::instance().getCurrentFeeAddress();
    if (!DepositsFrame::remote_node_fee_address.isEmpty())
    {
      QVector<CryptoNote::TransactionMessage> walletMessages;
      CryptoNote::WalletLegacyTransfer walletTransfer;
      walletTransfer.address = DepositsFrame::remote_node_fee_address.toStdString();
      walletTransfer.amount = 10000;
      walletTransfers.push_back(walletTransfer);

      if (WalletAdapter::instance().isOpen())
      {
        WalletAdapter::instance().sendTransaction(walletTransfers, 1000, "", 4, walletMessages);
      }
    }
  }
}

void DepositsFrame::depositParamsChanged()
{

  quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
  quint32 term = m_ui->m_timeSpin->value() * 21900;
  quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
  qreal termRate = DepositModel::calculateRate(amount, interest);
  m_ui->m_interestLabel->setText(QString("+ %1 %2 (%3 %)").arg(CurrencyAdapter::instance().formatAmount(interest)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()).arg(QString::number(termRate * 100, 'f', 4)));
}

void DepositsFrame::showDepositDetails(const QModelIndex &_index)
{
  if (!_index.isValid())
  {
    return;
  }
  DepositDetailsDialog dlg(_index, &MainWindow::instance());
  dlg.exec();
}

void DepositsFrame::timeChanged(int _value)
{
  m_ui->m_nativeTimeLabel->setText(monthsToBlocks(m_ui->m_timeSpin->value()));
}

void DepositsFrame::withdrawClicked()
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
}

void DepositsFrame::backClicked()
{
  /* back to overview frame */
  Q_EMIT backSignal();
}

} // namespace WalletGui
