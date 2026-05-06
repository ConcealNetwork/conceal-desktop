// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2020 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QDateTime>
#include <QDebug>
#include <QMetaEnum>
#include <QSize>
#include <QDateTime>
#include <QFont>
#include <QPixmap>
#include <QTextStream>

#include <Common/StringTools.h>

#include "DepositModel.h"
#include "TransactionsModel.h"
#include "CurrencyAdapter.h"
#include "NodeAdapter.h"
#include "WalletAdapter.h"
#include "Settings.h"

Q_DECLARE_METATYPE(cn::TransactionId)

namespace WalletGui {

Q_DECL_CONSTEXPR quint32 YEAR_SECONDS = 365 * 24 * 60 * 60;

namespace {

  QDateTime getExpectedTimeForHeight(quint64 _height) {
    quint64 lastLocalBlockHeight = NodeAdapter::instance().getLastLocalBlockHeight();
    QDateTime localLocalBlockTimestamp = NodeAdapter::instance().getLastLocalBlockTimestamp();

    return localLocalBlockTimestamp.addSecs((_height - lastLocalBlockHeight) * CurrencyAdapter::instance().getDifficultyTarget());
  }
}

enum class MessageType : quint8 {INPUT, OUTPUT};

const int DEPOSIT_MODEL_COLUMN_COUNT =
  DepositModel::staticMetaObject.enumerator(DepositModel::staticMetaObject.indexOfEnumerator("Columns")).keyCount();

DepositModel& DepositModel::instance() {
  static DepositModel inst;
  return inst;
}

DepositModel::DepositModel() : QAbstractItemModel(), m_depositCount(0) {
  connect(&WalletAdapter::instance(), &WalletAdapter::reloadWalletTransactionsSignal, this, &DepositModel::reloadWalletDeposits, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionCreatedSignal, this, static_cast<void(DepositModel::*)(cn::TransactionId)>(&DepositModel::transactionCreated), Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionUpdatedSignal, this, &DepositModel::transactionUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &DepositModel::reset, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletDepositsUpdatedSignal, this, &DepositModel::depositsUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletDepositPendingSignal, this, &DepositModel::walletDepositPendingSubmitted, Qt::QueuedConnection);
}

DepositModel::~DepositModel() {
}

bool DepositModel::isPendingRow(int _row) const {
  return _row >= static_cast<int>(m_depositCount);
}

int DepositModel::pendingIndex(int _row) const {
  return _row - static_cast<int>(m_depositCount);
}

Qt::ItemFlags DepositModel::flags(const QModelIndex& _index) const {
  if (!_index.isValid()) {
    return Qt::NoItemFlags;
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}

int DepositModel::columnCount(const QModelIndex& _parent) const {
  return DEPOSIT_MODEL_COLUMN_COUNT;
}

int DepositModel::rowCount(const QModelIndex& _parent) const {
  return static_cast<int>(m_depositCount) + m_pendingDeposits.size();
}

QVariant DepositModel::headerData(int _section, Qt::Orientation _orientation, int _role) const {
  if(_orientation != Qt::Horizontal) {
    return QVariant();
  }

  switch(_role) {
  case Qt::DisplayRole:
    switch(_section) {
    case COLUMN_STATE:
      return tr("Status");
    case COLUMN_AMOUNT:
      return tr("Amount");
    case COLUMN_INTEREST:
      return tr("Interest");
    case COLUMN_SUM:
      return tr("Sum");
    case COLUMN_TERM_RATE:
      return tr("Rate");
    case COLUMN_TERM:
      return tr("Term");
    case COLUMN_UNLOCK_HEIGHT:
      return tr("Unlock height");
    case COLUMN_UNLOCK_TIME:
      return tr("Unlock time");
    case COLUMN_CREATRING_TRANSACTION_HASH:
      return tr("Creating transaction");
    case COLUMN_CREATING_HEIGHT:
      return tr("Creating height");
    case COLUMN_CREATING_TIME:
      return tr("Creating time");
    case COLUMN_SPENDING_TRANSACTION_HASH:
      return tr("Spending transaction");
    case COLUMN_SPENDING_HEIGHT:
      return tr("Spending height");
    case COLUMN_SPENDING_TIME:
      return tr("Spending time");
    case COLUMN_TYPE:
      return tr("Type");      
    default:
      break;
    }

  case ROLE_COLUMN:
    return _section;
  }

  return QVariant();
}

QVariant DepositModel::data(const QModelIndex& _index, int _role) const {
  if(!_index.isValid()) {
    return QVariant();
  }

  switch(_role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    return getDisplayRole(_index);

  case Qt::SizeHintRole:
    return QSize(300, 35);

  case Qt::DecorationRole:
    return getDecorationRole(_index);

  case Qt::TextAlignmentRole:
    return getAlignmentRole(_index);

  default:
    return getUserRole(_index, _role);
  }

  return QVariant();
}

QModelIndex DepositModel::index(int _row, int _column, const QModelIndex& _parent) const {
  if(_parent.isValid()) {
    return QModelIndex();
  }

  return createIndex(_row, _column, _row);
}

QModelIndex DepositModel::parent(const QModelIndex& _index) const {
  return QModelIndex();
}

qreal DepositModel::calculateRate(quint64 _amount, quint64 _interest) {
  return (static_cast<qreal>(_interest)) / _amount;
}

QVariant DepositModel::getDisplayRole(const QModelIndex& _index) const {
  if (isPendingRow(_index.row())) {
    const PendingDepositRow& pr = m_pendingDeposits[pendingIndex(_index.row())];
    const quint64 pendingInterest = CurrencyAdapter::instance().calculateInterest(
        pr.amount, pr.term, NodeAdapter::instance().getLastKnownBlockHeight());
    switch (_index.column()) {
    case COLUMN_STATE:
      return tr("Pending");
    case COLUMN_AMOUNT:
      return CurrencyAdapter::instance().formatAmount(pr.amount);
    case COLUMN_INTEREST:
      return CurrencyAdapter::instance().formatAmount(pendingInterest);
    case COLUMN_SUM:
      return CurrencyAdapter::instance().formatAmount(pr.amount + pendingInterest);
    case COLUMN_TERM_RATE: {
      if (pr.amount == 0) {
        return QStringLiteral("-");
      }
      const qreal termRate = calculateRate(pr.amount, pendingInterest);
      return QString("%1 %").arg(QString::number(termRate * 100, 'f', 6));
    }
    case COLUMN_TERM:
      return pr.term > 0 ? QVariant(pr.term) : QStringLiteral("-");
    case COLUMN_UNLOCK_HEIGHT:
    case COLUMN_UNLOCK_TIME:
      return QStringLiteral("-");
    case COLUMN_TYPE:
      return tr("Deposit");
    case COLUMN_CREATRING_TRANSACTION_HASH:
      return pr.transactionHash;
    case COLUMN_CREATING_HEIGHT: {
      if (pr.transactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
        return QStringLiteral("-");
      }
      return TransactionsModel::instance().index(pr.transactionId, TransactionsModel::COLUMN_HEIGHT).data();
    }
    case COLUMN_CREATING_TIME: {
      if (pr.transactionId != cn::WALLET_INVALID_TRANSACTION_ID) {
        const QVariant d =
            TransactionsModel::instance().index(pr.transactionId, TransactionsModel::COLUMN_DATE).data();
        if (d.isValid() && !d.toString().isEmpty()) {
          return d;
        }
      }
      return QDateTime::fromSecsSinceEpoch(0).toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    }
    case COLUMN_SPENDING_TRANSACTION_HASH:
    case COLUMN_SPENDING_HEIGHT:
    case COLUMN_SPENDING_TIME:
      return QStringLiteral("-");
    default:
      return QStringLiteral("-");
    }
  }

  switch(_index.column()) {
  case COLUMN_STATE: {
    DepositState depositState = static_cast<DepositState>(_index.data(ROLE_STATE).toInt());
    switch (depositState) {
    case STATE_LOCKED:
      return tr("Locked");
    case STATE_UNLOCKED:
      return tr("Unlocked");
    case STATE_SPENT:
      return tr("Spent");
    case STATE_PENDING_CONFIRMATION:
      return tr("Pending");
    }
  }

  case COLUMN_AMOUNT:
    return CurrencyAdapter::instance().formatAmount(_index.data(ROLE_DEPOSIT_AMOUNT).value<quint64>());
  case COLUMN_INTEREST:
    return CurrencyAdapter::instance().formatAmount(_index.data(ROLE_DEPOSIT_INTEREST).value<quint64>());
  case COLUMN_SUM:
    return CurrencyAdapter::instance().formatAmount(_index.data(ROLE_DEPOSIT_AMOUNT).value<quint64>() + _index.data(ROLE_DEPOSIT_INTEREST).value<quint64>());
  case COLUMN_TERM_RATE: {
    quint64 amount = _index.data(ROLE_DEPOSIT_AMOUNT).value<quint64>();
    quint64 interest = _index.data(ROLE_DEPOSIT_INTEREST).value<quint64>();
    quint32 term = _index.data(ROLE_DEPOSIT_TERM).value<quint32>();
    qreal termRate = calculateRate(amount, interest);
    return QString("%1 %").arg(QString::number(termRate * 100, 'f', 6));
  }
  case COLUMN_TERM:
    return _index.data(ROLE_DEPOSIT_TERM);
  case COLUMN_UNLOCK_HEIGHT: {
    quint64 unlockHeight = _index.data(ROLE_UNLOCK_HEIGHT).value<quint64>();
    if (unlockHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
      return "-";
    }

    return unlockHeight > 0 ? unlockHeight - 1 : 0;
  }

  case COLUMN_TYPE: {
    quint32 term = _index.data(ROLE_DEPOSIT_TERM).value<quint32>();
    if (term % 64800 == 0) {
      return tr("Investment");
    }
    if (term % CurrencyAdapter::instance().getCurrency().depositMinTermV3() == 0) {
      return tr("Deposit");
    }
    if (term % 5040 == 0) {
      return tr("Deposit");
    }
  }

  case COLUMN_UNLOCK_TIME: {
    DepositState depositState = static_cast<DepositState>(_index.data(ROLE_STATE).toInt());
    if (depositState == STATE_LOCKED) {
      quint64 unlockHeight = _index.data(ROLE_UNLOCK_HEIGHT).value<quint64>();
      if (unlockHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
        return "-";
      }

      return getExpectedTimeForHeight(unlockHeight).toString("yyyy-MM-dd HH:mm");
    } else {
      return QDateTime();
    }
  }

  case COLUMN_CREATRING_TRANSACTION_HASH: {
    cn::TransactionId creatingTransactionId = _index.data(ROLE_CREATING_TRANSACTION_ID).value<cn::TransactionId>();
    return TransactionsModel::instance().index(creatingTransactionId, TransactionsModel::COLUMN_HASH).data();
  }

  case COLUMN_CREATING_HEIGHT: {
    cn::TransactionId creatingTransactionId = _index.data(ROLE_CREATING_TRANSACTION_ID).value<cn::TransactionId>();
    if (creatingTransactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
      return "-";
    }

    return TransactionsModel::instance().index(creatingTransactionId, TransactionsModel::COLUMN_HEIGHT).data();
  }

  case COLUMN_CREATING_TIME: {
    cn::TransactionId creatingTransactionId = _index.data(ROLE_CREATING_TRANSACTION_ID).value<cn::TransactionId>();
    if (creatingTransactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
      return "-";
    }

    return TransactionsModel::instance().index(creatingTransactionId, TransactionsModel::COLUMN_DATE).data();
  }

  case COLUMN_SPENDING_TRANSACTION_HASH: {
    cn::TransactionId spendingTransactionId = _index.data(ROLE_SPENDING_TRANSACTION_ID).value<cn::TransactionId>();
    if (spendingTransactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
      return "-";
    }

    return TransactionsModel::instance().index(spendingTransactionId, TransactionsModel::COLUMN_HASH).data();
  }

  case COLUMN_SPENDING_HEIGHT: {
    cn::TransactionId spendingTransactionId = _index.data(ROLE_SPENDING_TRANSACTION_ID).value<cn::TransactionId>();
    if (spendingTransactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
      return "-";
    }

    return TransactionsModel::instance().index(spendingTransactionId, TransactionsModel::COLUMN_HEIGHT).data();
  }

  case COLUMN_SPENDING_TIME: {
    cn::TransactionId spendingTransactionId = _index.data(ROLE_SPENDING_TRANSACTION_ID).value<cn::TransactionId>();
    if (spendingTransactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
      return "-";
    }

    return TransactionsModel::instance().index(spendingTransactionId, TransactionsModel::COLUMN_DATE).data();
  }

  default:
    break;
  }

  return QVariant();
}

QVariant DepositModel::getDecorationRole(const QModelIndex& _index) const {
  return QVariant();
}

QVariant DepositModel::getAlignmentRole(const QModelIndex& _index) const {
  return headerData(_index.column(), Qt::Horizontal, Qt::TextAlignmentRole);
}

QVariant DepositModel::getUserRole(const QModelIndex& _index, int _role) const {
  if (isPendingRow(_index.row())) {
    const PendingDepositRow& pr = m_pendingDeposits.at(pendingIndex(_index.row()));
    switch (_role) {
    case ROLE_STATE:
      return static_cast<int>(STATE_PENDING_CONFIRMATION);
    case ROLE_CREATING_TRANSACTION_ID:
      return pr.transactionId == cn::WALLET_INVALID_TRANSACTION_ID
                 ? QVariant()
                 : QVariant(static_cast<quintptr>(pr.transactionId));
    case ROLE_SPENDING_TRANSACTION_ID:
      return QVariant(static_cast<quintptr>(cn::WALLET_INVALID_TRANSACTION_ID));
    case ROLE_UNLOCK_HEIGHT:
      return static_cast<quint64>(cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT);
    case ROLE_ROW:
      return _index.row();
    case ROLE_COLUMN:
      return headerData(_index.column(), Qt::Horizontal, ROLE_COLUMN);
    case ROLE_IS_PENDING_DEPOSIT:
      return true;
    case ROLE_DEPOSIT_TERM:
      return pr.term;
    case ROLE_DEPOSIT_AMOUNT:
      return pr.amount;
    case ROLE_DEPOSIT_INTEREST:
      return CurrencyAdapter::instance().calculateInterest(pr.amount, pr.term, NodeAdapter::instance().getLastKnownBlockHeight());
    default:
      return QVariant();
    }
  }

  cn::Deposit deposit;

  if(!WalletAdapter::instance().getDeposit(_index.row(), deposit)) {
    return QVariant();
  }

  switch(_role) {
  case ROLE_DEPOSIT_TERM:
    return deposit.term;

  case ROLE_DEPOSIT_AMOUNT:
    return static_cast<quint64>(deposit.amount);

  case ROLE_DEPOSIT_INTEREST:
    return CurrencyAdapter::instance().calculateInterest(deposit.amount, deposit.term, NodeAdapter::instance().getLastKnownBlockHeight());
  

  case ROLE_STATE:
    if (deposit.locked) {
      return static_cast<int>(STATE_LOCKED);
    } else if (deposit.spendingTransactionId == cn::WALLET_INVALID_TRANSACTION_ID) {
      return static_cast<int>(STATE_UNLOCKED);
    } else {
      return static_cast<int>(STATE_SPENT);
    }

    return QVariant();

  case ROLE_CREATING_TRANSACTION_ID:
    return static_cast<quintptr>(deposit.creatingTransactionId);

  case ROLE_SPENDING_TRANSACTION_ID:
    return static_cast<quintptr>(deposit.spendingTransactionId);

  case ROLE_UNLOCK_HEIGHT: {
    cn::TransactionId creatingTransactionId = _index.data(ROLE_CREATING_TRANSACTION_ID).value<cn::TransactionId>();
    quint64 creatingHeight = TransactionsModel::instance().index(creatingTransactionId, 0).
      data(TransactionsModel::ROLE_HEIGHT).value<quint64>();
    if (creatingHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
      return static_cast<const quint64>(cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT);
    }

    return creatingHeight + _index.data(ROLE_DEPOSIT_TERM).value<quint32>();
  }

  case ROLE_ROW:
    return _index.row();

  case ROLE_COLUMN:
    return headerData(_index.column(), Qt::Horizontal, ROLE_COLUMN);

  case ROLE_IS_PENDING_DEPOSIT:
    return false;

  }

  return QVariant();
}

void DepositModel::reloadWalletDeposits() {
  reset();

  if (WalletAdapter::instance().getDepositCount() == 0) {
    return;
  }

  beginInsertRows(QModelIndex(), 0, WalletAdapter::instance().getDepositCount() - 1);
  m_depositCount = WalletAdapter::instance().getDepositCount();
  endInsertRows();
}

void DepositModel::appendDeposit(cn::DepositId _depositId) {
  if (_depositId < m_depositCount) {
    return;
  }

  cn::Deposit deposit;

  if(!WalletAdapter::instance().getDeposit(_depositId, deposit)) {
    return;
  }

  if (!deposit.locked) {
    if (deposit.spendingTransactionId != cn::WALLET_INVALID_TRANSACTION_ID) {
   return;
    }
  }  



  beginInsertRows(QModelIndex(), m_depositCount, _depositId);
  m_depositCount = _depositId + 1;
  endInsertRows();
}

void DepositModel::transactionCreated(cn::TransactionId _transactionId) {
  Q_UNUSED(_transactionId);
  syncDepositRowsFromWallet();
}

void DepositModel::reset() {
  beginResetModel();
  m_depositCount = 0;
  m_pendingDeposits.clear();
  endResetModel();
}

void DepositModel::depositsUpdated(const QVector<cn::DepositId>& _depositIds) {
  syncDepositRowsFromWallet();
  Q_FOREACH (const auto& depositId, _depositIds) {
    if (depositId < m_depositCount) {
      Q_EMIT dataChanged(index(static_cast<int>(depositId), 0), index(static_cast<int>(depositId), columnCount() - 1));
    }
  }
}

void DepositModel::syncDepositRowsFromWallet() {
  const quint64 walletCount = WalletAdapter::instance().getDepositCount();
  if (m_depositCount >= walletCount) {
    return;
  }
  appendDeposit(static_cast<cn::DepositId>(walletCount - 1));
}

void DepositModel::walletDepositPendingSubmitted(cn::TransactionId _transactionId, const QString& _hashHex, quint64 _amount,
                                                 quint32 _term) {
  PendingDepositRow row;
  row.transactionId = _transactionId;
  row.transactionHash = _hashHex.toUpper();
  row.amount = _amount;
  row.term = _term;
  const int newRow = static_cast<int>(m_depositCount) + m_pendingDeposits.size();
  beginInsertRows(QModelIndex(), newRow, newRow);
  m_pendingDeposits.append(row);
  endInsertRows();
}

void DepositModel::transactionUpdated(cn::TransactionId _transactionId) {
  cn::WalletTransaction wt;
  if (WalletAdapter::instance().getTransaction(_transactionId, wt)) {
    const QString hashUpper = QString::fromStdString(common::podToHex(wt.hash)).toUpper();
    if (wt.blockHeight != cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
      for (int i = m_pendingDeposits.size() - 1; i >= 0; --i) {
        const PendingDepositRow& pr = m_pendingDeposits[i];
        const bool match = (pr.transactionHash == hashUpper) ||
          (pr.transactionId != cn::WALLET_INVALID_TRANSACTION_ID && pr.transactionId == _transactionId);
        if (match) {
          const int row = static_cast<int>(m_depositCount) + i;
          beginRemoveRows(QModelIndex(), row, row);
          m_pendingDeposits.removeAt(i);
          endRemoveRows();
          break;
        }
      }
    }
  }

  syncDepositRowsFromWallet();

  QModelIndex transactionIndex = TransactionsModel::instance().index(_transactionId, 0);
  if (!transactionIndex.isValid()) {
    return;
  }

  const quintptr firstDepositId = transactionIndex.data(TransactionsModel::ROLE_DEPOSIT_ID).value<quintptr>();
  const quintptr depositCountTx = transactionIndex.data(TransactionsModel::ROLE_DEPOSIT_COUNT).value<quintptr>();
  if (depositCountTx == 0) {
    return;
  }

  Q_EMIT dataChanged(index(static_cast<int>(firstDepositId), 0),
                     index(static_cast<int>(firstDepositId + depositCountTx - 1), columnCount() - 1));
}

}
