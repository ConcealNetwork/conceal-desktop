// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "AddressProvider.h"
#include "BankingFrame.h"
#include "DepositDetailsDialog.h"
#include "DepositListModel.h"
#include "DepositModel.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "NodeAdapter.h"
#include "ui_bankingframe.h"
#include "Settings.h"

#include <QFontDatabase>
#include <QMessageBox>
#include <QPainter>
#include <QColor>

namespace WalletGui {

namespace 
{
  /* Convert weeks to the number of blocks */
  QString weeksToBlocks(int _weeks) 
  {
    QString resTempate("%1 %2");
    /* A maxmimum of 52 weeks */
    if (_weeks < 53) 
    {
      return resTempate.arg(_weeks * 5040).arg(QObject::tr("blocks"));
    }
    return QString();
  }

  /* Convert quarters to the number of blocks */
  QString quartersToBlocks(int _quarters) 
  {
    QString resTempate("%1 %2");
    /* A maximum of 20 quarters */
    if (_quarters < 21) 
    {
      return resTempate.arg(_quarters * 64800).arg(QObject::tr("blocks"));
    }
    return QString();
  }
}

BankingFrame::BankingFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::BankingFrame), m_depositModel(new DepositListModel) 
{
  m_ui->setupUi(this);
  m_ui->m_depositView->setModel(m_depositModel.data());
  m_ui->m_depositView->sortByColumn(5, Qt::SortOrder::DescendingOrder); //COLUMN_UNLOCK_HEIGHT, ascending
  m_ui->m_depositView->header()->moveSection(6, 0);
  m_ui->m_depositView->header()->moveSection(7, 1);  
  m_ui->m_depositView->header()->resizeSection(0, 75);
  m_ui->m_depositView->header()->resizeSection(1, 75);
  m_ui->m_depositView->header()->resizeSection(2, 100);
  m_ui->m_depositView->header()->resizeSection(3, 100);
  m_ui->m_depositView->header()->resizeSection(4, 150);  
  m_ui->m_depositView->header()->resizeSection(5, 175);
  m_ui->m_depositView->header()->resizeSection(6, 175);
  m_ui->m_depositView->header()->resizeSection(7, 125);
  m_ui->m_unlockedInvestmentLabel->setText("UNLOCKED INVESTMENTS " + CurrencyAdapter::instance().formatAmount(0) + " CCX");
  m_ui->m_unlockedDepositLabel->setText("UNLOCKED DEPOSITS " + CurrencyAdapter::instance().formatAmount(0) + " CCX");
 
  int id2 = QFontDatabase::addApplicationFont(":/fonts/Lucon.ttf");
  QFont font2;
  font2.setFamily("Lucida Console");
  font2.setPixelSize(12);
  m_ui->m_depositView->setFont(font2);

  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal, this, &BankingFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualInvestmentBalanceUpdatedSignal, this, &BankingFrame::actualInvestmentBalanceUpdated, Qt::QueuedConnection);
}

BankingFrame::~BankingFrame() {
}

/* Update the label when the deposit balance changes */
void BankingFrame::actualDepositBalanceUpdated(quint64 _balance) 
{
  m_ui->m_unlockedDepositLabel->setText("UNLOCKED DEPOSITS " + CurrencyAdapter::instance().formatAmount(_balance) + " CCX");
}

/* Update the label when the investment balance changes */
void BankingFrame::actualInvestmentBalanceUpdated(quint64 _balance) 
{
  quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
  m_ui->m_unlockedInvestmentLabel->setText("UNLOCKED INVESTMENTS " + CurrencyAdapter::instance().formatAmount(actualInvestmentBalance) + " CCX");
}


void BankingFrame::showDepositDetails(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }
  DepositDetailsDialog dlg(_index, &MainWindow::instance());
  dlg.exec();
} 

void BankingFrame::withdrawClicked() 
{
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



void BankingFrame::backClicked() {
  /* back to overview frame */
  Q_EMIT backSignal();
}



}
