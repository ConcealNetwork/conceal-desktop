// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2026 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <IWallet.h>

namespace WalletGui {

class DepositModel : public QAbstractItemModel {
  Q_OBJECT
  Q_ENUMS(Columns)
  Q_ENUMS(Roles)
  Q_ENUMS(DepositState)

public:
  enum Columns{
    COLUMN_STATE = 0, COLUMN_AMOUNT, COLUMN_INTEREST, COLUMN_SUM, COLUMN_TERM_RATE, COLUMN_TERM, COLUMN_UNLOCK_HEIGHT,
    COLUMN_UNLOCK_TIME, COLUMN_CREATRING_TRANSACTION_HASH, COLUMN_CREATING_HEIGHT, COLUMN_CREATING_TIME,
    COLUMN_SPENDING_TRANSACTION_HASH, COLUMN_SPENDING_HEIGHT, COLUMN_SPENDING_TIME, COLUMN_TYPE
  };

  enum Roles {
    ROLE_DEPOSIT_TERM = Qt::UserRole, ROLE_DEPOSIT_AMOUNT, ROLE_DEPOSIT_INTEREST, ROLE_STATE,
    ROLE_CREATING_TRANSACTION_ID, ROLE_SPENDING_TRANSACTION_ID, ROLE_UNLOCK_HEIGHT, ROLE_ROW, ROLE_COLUMN,
    ROLE_IS_PENDING_DEPOSIT
  };

  enum DepositState {
    STATE_LOCKED, STATE_UNLOCKED, STATE_SPENT, STATE_PENDING_CONFIRMATION
  };

  static DepositModel& instance();

  Qt::ItemFlags flags(const QModelIndex& _index) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& _parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int rowCount(const QModelIndex& _parent = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant headerData(int _section, Qt::Orientation _orientation, int _role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QModelIndex index(int _row, int _column, const QModelIndex& _parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QModelIndex	parent(const QModelIndex& _index) const Q_DECL_OVERRIDE;
  static qreal calculateRate(quint64 _amount, //amount invested (in coin minimum increments)
                             quint64 _interest); //interest earned (in coin minimum increments)

private:
  struct PendingDepositRow {
    cn::TransactionId transactionId;
    QString transactionHash;
    quint64 amount = 0;
    quint32 term = 0;
  };

  quint32 m_depositCount;
  QList<PendingDepositRow> m_pendingDeposits;
  QHash<cn::TransactionId, QList<cn::DepositId> > m_spendingTransactions;

  DepositModel();
  ~DepositModel();

  bool isPendingRow(int _row) const;
  int pendingIndex(int _row) const;
  QVariant getDisplayRole(const QModelIndex& _index) const;
  QVariant getDecorationRole(const QModelIndex& _index) const;
  QVariant getAlignmentRole(const QModelIndex& _index) const;
  QVariant getUserRole(const QModelIndex& _index, int _role) const;

  void reloadWalletDeposits();
  void appendDeposit(cn::DepositId _depositId);
  void syncDepositRowsFromWallet();
  void transactionCreated(cn::TransactionId _transactionId);
  void reset();
  void depositsUpdated(const QVector<cn::DepositId>& _depositIds);
  void transactionUpdated(cn::TransactionId _transactionId);
  void walletDepositPendingSubmitted(cn::TransactionId _transactionId, const QString& _hashHex, quint64 _amount,
                                    quint32 _term);
};

}
