// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QFileDialog>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>
#include "MainWindow.h"
#include "SortedTransactionsModel.h"
#include "TransactionsFrame.h"
#include "TransactionDetailsDialog.h"
#include "TransactionsListModel.h"
#include "TransactionsModel.h"

#include "ui_transactionsframe.h"

namespace WalletGui {

TransactionsFrame::TransactionsFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::TransactionsFrame),
  m_transactionsModel(new TransactionsListModel) {
  m_ui->setupUi(this);
  m_ui->m_transactionsView->setModel(m_transactionsModel.data());
  m_ui->m_transactionsView->header()->setSectionResizeMode(TransactionsModel::COLUMN_STATE, QHeaderView::Fixed);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_STATE, 15);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_DATE, 140);
  m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_HASH, 300);
  m_ui->m_transactionsView->header()->moveSection( 3, 5 );

  int id2 = QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
  QFont font2;
  font2.setFamily("Lato");
  font2.setPixelSize(15);
  m_ui->m_transactionsView->setFont(font2);

}

TransactionsFrame::~TransactionsFrame() {
}

void TransactionsFrame::scrollToTransaction(const QModelIndex& _index) {
  QModelIndex sortedModelIndex = SortedTransactionsModel::instance().mapFromSource(_index);
  QModelIndex index = static_cast<QSortFilterProxyModel*>(m_ui->m_transactionsView->model())->mapFromSource(sortedModelIndex);
  m_ui->m_transactionsView->scrollTo(index);
  m_ui->m_transactionsView->setFocus();
  m_ui->m_transactionsView->setCurrentIndex(index);
}

void TransactionsFrame::exportToCsv() {
  QString file = QFileDialog::getSaveFileName(&MainWindow::instance(), tr("Select CSV file"), QDir::homePath(), "CSV (*.csv)");
  if (!file.isEmpty()) {
    QByteArray csv = TransactionsModel::instance().toCsv();
    QFile f(file);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      f.write(csv);
      f.close();
    }
  }
}

void TransactionsFrame::showTransactionDetails(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }

  TransactionDetailsDialog dlg(_index, &MainWindow::instance());
  dlg.exec();
}

void TransactionsFrame::backClicked() {
  Q_EMIT backSignal();
}

}
