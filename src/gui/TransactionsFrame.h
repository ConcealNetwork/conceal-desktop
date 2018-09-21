// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>
#include <QStyledItemDelegate>

namespace Ui {
class TransactionsFrame;
}

namespace WalletGui {

class TransactionsListModel;

class TransactionsFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(TransactionsFrame)

public:
  TransactionsFrame(QWidget* _parent);
  ~TransactionsFrame();

  void scrollToTransaction(const QModelIndex& _index);

private:
  QScopedPointer<Ui::TransactionsFrame> m_ui;
  QScopedPointer<TransactionsListModel> m_transactionsModel;

  Q_SLOT void exportToCsv();
  Q_SLOT void showTransactionDetails(const QModelIndex& _index);
  Q_SLOT void backClicked();

Q_SIGNALS:
  void backSignal();



};

}
