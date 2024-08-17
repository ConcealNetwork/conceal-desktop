// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QDateTime>
#include <QFont>
#include <QMetaEnum>
#include <QPixmap>
#include <QPainter>
#include <QTextStream>

#include "CurrencyAdapter.h"
#include "Message.h"
#include "NodeAdapter.h"
#include "TransactionsModel.h"
#include "WalletAdapter.h"
#include <Common/StringTools.h>
namespace WalletGui
{

  const int TRANSACTIONS_MODEL_COLUMN_COUNT =
      TransactionsModel::staticMetaObject.enumerator(TransactionsModel::staticMetaObject.indexOfEnumerator("Columns")).keyCount();

  namespace
  {

    QPixmap getTransactionIcon(TransactionsModel::TransactionType _transactionType)
    {
      switch (_transactionType)
      {
      case TransactionsModel::TransactionType::MINED:
        return QPixmap(":icons/tx-mined");
      case TransactionsModel::TransactionType::INPUT:
        return QPixmap(":icons/tx-input");
      case TransactionsModel::TransactionType::OUTPUT:
        return QPixmap(":icons/tx-output");
      case TransactionsModel::TransactionType::INOUT:
        return QPixmap(":icons/tx-inout");
      case TransactionsModel::TransactionType::DEPOSIT:
        return QPixmap(":icons/tx-deposit");
      case TransactionsModel::TransactionType::DEPOSIT_UNLOCK:
        return QPixmap(":icons/tx-withdraw");
      default:
        break;
      }

      return QPixmap();
    }

  } // namespace

  TransactionsModel &TransactionsModel::instance()
  {
    static TransactionsModel inst;
    return inst;
  }

  TransactionsModel::TransactionsModel() : QAbstractItemModel()
  {
    connect(&WalletAdapter::instance(), &WalletAdapter::reloadWalletTransactionsSignal, this, &TransactionsModel::reloadWalletTransactions,
            Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionCreatedSignal, this,
            static_cast<void (TransactionsModel::*)(cn::TransactionId)>(&TransactionsModel::appendTransaction), Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionUpdatedSignal, this, &TransactionsModel::updateWalletTransaction,
            Qt::QueuedConnection);
    connect(&NodeAdapter::instance(), &NodeAdapter::lastKnownBlockHeightUpdatedSignal, this, &TransactionsModel::lastKnownHeightUpdated,
            Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &TransactionsModel::reset,
            Qt::QueuedConnection);
  }

  TransactionsModel::~TransactionsModel()
  {
  }

  Qt::ItemFlags TransactionsModel::flags(const QModelIndex &_index) const
  {
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable;
    if (_index.column() == COLUMN_HASH)
    {
      flags |= Qt::ItemIsEditable;
    }

    return flags;
  }

  int TransactionsModel::columnCount(const QModelIndex &_parent) const
  {
    return TRANSACTIONS_MODEL_COLUMN_COUNT;
  }

  int TransactionsModel::rowCount(const QModelIndex &_parent) const
  {
    return m_transfers.size();
  }

  QVariant TransactionsModel::headerData(int _section, Qt::Orientation _orientation, int _role) const
  {
    if (_orientation != Qt::Horizontal)
    {
      return QVariant();
    }

    switch (_role)
    {
    case Qt::DisplayRole:
      switch (_section)
      {
      case COLUMN_STATE:
        return QVariant();
      case COLUMN_DATE:
        return tr("Date");
      case COLUMN_TYPE:
        return tr("Type");
      case COLUMN_ADDRESS:
        return tr("Address");
      case COLUMN_AMOUNT:
        return tr("Amount");
      case COLUMN_FEE:
        return tr("Fee");
      case COLUMN_HEIGHT:
        return tr("Height");
      case COLUMN_PAYMENT_ID:
        return tr("Payment ID");
      case COLUMN_MESSAGE:
        return tr("Message");
      case COLUMN_HASH:
        return tr("Transaction Hash");
      default:
        break;
      }

    case Qt::TextAlignmentRole:
      if (_section == COLUMN_AMOUNT)
      {
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
      }

      return QVariant();

    case ROLE_COLUMN:
      return _section;
    }

    return QVariant();
  }

  QVariant TransactionsModel::data(const QModelIndex &_index, int _role) const
  {
    if (!_index.isValid())
    {
      return QVariant();
    }

    cn::WalletTransaction transaction;
    cn::WalletTransfer transfer;
    cn::Deposit deposit;
    size_t transactionIndex = m_transfers.value(_index.row()).first;
    size_t transferIndex = m_transfers.value(_index.row()).second;
    if (!WalletAdapter::instance().getTransaction(transactionIndex, transaction) ||
        (m_transfers.value(_index.row()).second != cn::WALLET_INVALID_TRANSFER_ID &&
         !WalletAdapter::instance().getTransfer(transactionIndex, transferIndex, transfer)))
    {
      return QVariant();
    }
    cn::DepositId depositId = transaction.firstDepositId;
    if (depositId != cn::WALLET_INVALID_DEPOSIT_ID)
    {
      if (!WalletAdapter::instance().getDeposit(depositId, deposit))
      {
        return QVariant();
      }
    }

    switch (_role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return getDisplayRole(_index);

    case Qt::DecorationRole:
      return getDecorationRole(_index);

    case Qt::TextAlignmentRole:
      return getAlignmentRole(_index);

    default:
      return getUserRole(_index, _role, transactionIndex, transaction, transferIndex, transfer, depositId, deposit);
    }

    return QVariant();
  }

  QModelIndex TransactionsModel::index(int _row, int _column, const QModelIndex &_parent) const
  {
    if (_parent.isValid())
    {
      return QModelIndex();
    }

    return createIndex(_row, _column, _row);
  }

  QModelIndex TransactionsModel::parent(const QModelIndex &_index) const
  {
    return QModelIndex();
  }

  QByteArray TransactionsModel::toCsv() const
  {
    QByteArray res;
    res.append("\"State\",\"Date\",\"Amount\",\"Fee\",\"Hash\",\"Height\",\"Address\",\"Payment ID\"\n");
    for (quint32 row = 0; row < rowCount(); ++row)
    {
      QModelIndex ind = index(row, COLUMN_STATE);
      quint64 numberOfConfirmations = ind.data(ROLE_NUMBER_OF_CONFIRMATIONS).value<quint64>();
      QString text = (numberOfConfirmations == 0 ? tr("unconfirmed") : tr("confirmations"));
      res.append("\"").append(QString("%1 / %2").arg(numberOfConfirmations).arg(text).toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_DATE).data().toString().toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_AMOUNT).data().toString().toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_FEE).data().toString().toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_HASH).data().toString().toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_HEIGHT).data().toString().toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_ADDRESS).data().toString().toUtf8()).append("\",");
      res.append("\"").append(ind.sibling(row, COLUMN_PAYMENT_ID).data().toString().toUtf8()).append("\"\n");
    }

    return res;
  }

  QVariant TransactionsModel::getDisplayRole(const QModelIndex &_index) const
  {
    switch (_index.column())
    {
    case COLUMN_DATE:
    {
      QDateTime date = _index.data(ROLE_DATE).toDateTime();
      return (date.isNull() || !date.isValid() ? "-" : date.toString("yyyy-MM-dd HH:mm"));
    }

    case COLUMN_HASH:
      return _index.data(ROLE_HASH).toByteArray().toHex().toUpper();

    /* process and pass the secret key */
    case COLUMN_SECRETKEY:
    {

      /* get the type of transaction */
      TransactionType transactionType = static_cast<TransactionType>(_index.data(ROLE_TYPE).value<quint8>());

      /* we dont need the key if it's incoming, in-out, or a mined block */
      if (transactionType == TransactionType::INPUT || transactionType == TransactionType::MINED ||
          transactionType == TransactionType::INOUT)
      {
        return "not applicable";
      }
      else
      {
        /* existing outbound transactions not in the same sessions should be empty */
        if (_index.data(ROLE_SECRETKEY) == "0000000000000000000000000000000000000000000000000000000000000000")
        {
          return "expired";
        }
        else
        {
          /* return the proper transaction secret key */
          return _index.data(ROLE_SECRETKEY);
        }
      }
    }

    case COLUMN_ADDRESS:
    {
      TransactionType transactionType = static_cast<TransactionType>(_index.data(ROLE_TYPE).value<quint8>());
      QString transactionAddress = _index.data(ROLE_ADDRESS).toString();
      if (transactionType == TransactionType::INPUT || transactionType == TransactionType::MINED ||
          transactionType == TransactionType::INOUT)
      {
        return QString(tr("me (%1)").arg(WalletAdapter::instance().getAddress()));
      }
      else if (transactionAddress.isEmpty())
      {
        return tr("(n/a)");
      }

      return transactionAddress;
    }

    case COLUMN_AMOUNT:
    {
      qint64 amount = _index.data(ROLE_AMOUNT).value<qint64>();
      QString amountStr = CurrencyAdapter::instance().formatAmount(qAbs(amount)) + " CCX";
      return (amount < 0 ? "-" + amountStr : amountStr);
    }

    case COLUMN_CONFIRMATIONS:
    {
      quint64 transactionHeight = _index.data(ROLE_HEIGHT).value<quint64>();
      if (transactionHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT)
      {
        return tr("Unconfirmed");
      }

      quint64 confirmations = NodeAdapter::instance().getLastKnownBlockHeight() - transactionHeight + 1;

      if (confirmations >= 10)
      {
        return tr("Confirmed");
      }
    }

    case COLUMN_PAYMENT_ID:
      return _index.data(ROLE_PAYMENT_ID);

    case COLUMN_FEE:
    {
      qint64 fee = _index.data(ROLE_FEE).value<qint64>();
      return CurrencyAdapter::instance().formatAmount(fee);
    }

    case COLUMN_HEIGHT:
    {
      quint64 transactionHeight = _index.data(ROLE_HEIGHT).value<quint64>();
      if (transactionHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT)
      {
        return QVariant();
      }

      return QString::number(transactionHeight);
    }

    case COLUMN_MESSAGE:
    {
      QString messageString = _index.data(ROLE_MESSAGE).toString();
      QTextStream messageStream(&messageString);
      return messageStream.readLine();
    }

    default:
      break;
    }

    return QVariant();
  }

  QVariant TransactionsModel::getDecorationRole(const QModelIndex &_index) const
  {
    if (_index.column() == COLUMN_STATE)
    {
      quint64 numberOfConfirmations = _index.data(ROLE_NUMBER_OF_CONFIRMATIONS).value<quint64>();
      switch (numberOfConfirmations)
      {
      case 0:
        return QPixmap(":icons/clock1");
      case 1:
      case 2:
        return QPixmap(":icons/clock1");
      case 3:
      case 4:
        return QPixmap(":icons/clock2");
      case 5:
      case 6:
        return QPixmap(":icons/clock3");
      case 7:
      case 8:
        return QPixmap(":icons/clock4");
      case 9:
        return QPixmap(":icons/clock5");
      default:
        QPixmap icon = _index.data(ROLE_ICON).value<QPixmap>();
        return icon;
      }
    }
    else if (_index.column() == COLUMN_ADDRESS)
    {
      return _index.data(ROLE_ICON).value<QPixmap>().scaled(20, 20, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    return QVariant();
  }

  QVariant TransactionsModel::getAlignmentRole(const QModelIndex &_index) const
  {
    return headerData(_index.column(), Qt::Horizontal, Qt::TextAlignmentRole);
  }

  QVariant TransactionsModel::getUserRole(const QModelIndex &_index, int _role, cn::TransactionId _transactionId,
                                          const cn::WalletTransaction &_transaction, cn::TransferId _transferId, const cn::WalletTransfer &_transfer,
                                          cn::DepositId _depositId, const cn::Deposit &_deposit) const

  {

    switch (_role)
    {

    case ROLE_STATE:
      return static_cast<quint8>(_transaction.state);

    case ROLE_DATE:
      return (_transaction.timestamp > 0 ? QDateTime::fromTime_t(_transaction.timestamp) : QDateTime());

    case ROLE_TYPE: {
      QString transactionAddress = _index.data(ROLE_ADDRESS).toString();
      if (_transaction.isBase) {
        return static_cast<quint8>(TransactionType::MINED);
      } else if (_transaction.firstDepositId != cn::WALLET_INVALID_DEPOSIT_ID) {
        return static_cast<quint8>(TransactionType::DEPOSIT);
      } else if (transactionAddress == WalletAdapter::instance().getAddress()) {
        if (_transaction.fee == cn::parameters::MINIMUM_FEE) {
          return static_cast<quint8>(TransactionType::DEPOSIT_UNLOCK);
        } else if (_transaction.totalAmount < 0 && _transaction.blockHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
          return static_cast<quint8>(TransactionType::OUTPUT);
        } else if (_transaction.totalAmount == -1000) {
          return static_cast<quint8>(TransactionType::INOUT);
        } else if (_transaction.totalAmount < 0) {
          return static_cast<quint8>(TransactionType::OUTPUT);
        }
      } else if (_transaction.totalAmount == -1000) {
        return static_cast<quint8>(TransactionType::INOUT);
      } else if (_transaction.totalAmount < 0) {
        return static_cast<quint8>(TransactionType::OUTPUT);
      }
      return static_cast<quint8>(TransactionType::INPUT);
    }

    case ROLE_TXTYPE:
    {
      QString transactionAddress = _index.data(ROLE_ADDRESS).toString();

      auto transactionState = static_cast<cn::WalletTransactionState>(_index.data(ROLE_STATE).value<quint8>());
      if (transactionState == cn::WalletTransactionState::FAILED)
      {
        return "Failed";
      }

      if (_transaction.isBase)
      {
        return "New Block";
      }
      else if (_transaction.firstDepositId != cn::WALLET_INVALID_DEPOSIT_ID)
      {
        return "New Deposit";
      }
      else if (!transactionAddress.compare(WalletAdapter::instance().getAddress()))
      {
        return "Optimization";
      }
      else if (_transaction.totalAmount < 0)
      {
        return "Sent CCX";
      }

      return "Received CCX";
    }

    case ROLE_HASH:
      return QByteArray(reinterpret_cast<const char *>(&_transaction.hash), sizeof(_transaction.hash));

    case ROLE_SECRETKEY:
    {
      crypto::Hash txHash = _transaction.hash;
      crypto::SecretKey txkey = WalletAdapter::instance().getTxKey(txHash);
      if (txkey != cn::NULL_SECRET_KEY)
      {
        return QString::fromStdString(common::podToHex(txkey));
      } else {
        return tr("not found");
      }
    }

    case ROLE_ADDRESS:
      return QString::fromStdString(_transfer.address);

    case ROLE_AMOUNT:
    {
      auto transactionType = static_cast<TransactionType>(_index.data(ROLE_TYPE).value<quint8>());
      if (transactionType == TransactionType::INPUT || transactionType == TransactionType::MINED)
      {
        return static_cast<qint64>(_transaction.totalAmount);
      }
      else if (transactionType == TransactionType::OUTPUT || transactionType == TransactionType::INOUT)
      {
        return static_cast<qint64>(_transaction.totalAmount);
      }
      else if (transactionType == TransactionType::DEPOSIT)
      {
        return static_cast<qint64>(_deposit.amount);
      } else if (transactionType == TransactionType::DEPOSIT_UNLOCK){
        return static_cast<qint64>(_transaction.totalAmount);
      }

      return QVariant();
    }

    case ROLE_PAYMENT_ID:
      return NodeAdapter::instance().extractPaymentId(_transaction.extra);

    case ROLE_ICON:
    {
      auto transactionType = static_cast<TransactionType>(_index.data(ROLE_TYPE).value<quint8>());
      return getTransactionIcon(transactionType);
    }

    case ROLE_TRANSACTION_ID:
      return QVariant::fromValue(_transactionId);

    case ROLE_HEIGHT:
      return static_cast<quint64>(_transaction.blockHeight);

    case ROLE_FEE:
      return static_cast<quint64>(_transaction.fee);

    case ROLE_NUMBER_OF_CONFIRMATIONS:
      return (_transaction.blockHeight == cn::WALLET_UNCONFIRMED_TRANSACTION_HEIGHT ? 0 : NodeAdapter::instance().getLastKnownBlockHeight() - _transaction.blockHeight + 1);

    case ROLE_COLUMN:
      return headerData(_index.column(), Qt::Horizontal, ROLE_COLUMN);

    case ROLE_ROW:
      return _index.row();

    case ROLE_MESSAGE:
    {
      if (_transaction.messages.empty())
      {
        return QVariant();
      }

      QString messageString = Message(QString::fromStdString(_transaction.messages[0])).getMessage();
      QTextStream messageStream(&messageString);
      return messageStream.readLine();
    }

    case ROLE_MESSAGES:
    {
      QStringList messageList;
      messageList.reserve(_transaction.messages.size());
      Q_FOREACH (const auto &message, _transaction.messages)
      {
        messageList << QString::fromStdString(message);
      }

      return messageList;
    }

    case ROLE_DEPOSIT_ID:
      return static_cast<quintptr>(_transaction.firstDepositId);

    case ROLE_DEPOSIT_COUNT:
      return static_cast<quintptr>(_transaction.depositCount);
    }

    return QVariant();
  }

  void TransactionsModel::reloadWalletTransactions()
  {
    beginResetModel();
    m_transfers.clear();
    m_transactionRow.clear();
    endResetModel();

    quint32 row_count = 0;
    for (cn::TransactionId transactionId = 0; transactionId < WalletAdapter::instance().getTransactionCount(); ++transactionId)
    {
      appendTransaction(transactionId, row_count);
    }

    if (row_count > 0)
    {
      beginInsertRows(QModelIndex(), 0, row_count - 1);
      endInsertRows();
    }
  }

  void TransactionsModel::appendTransaction(cn::TransactionId _transactionId, quint32 &_insertedRowCount)
  {
    cn::WalletTransaction transaction;
    const auto &wallet = WalletAdapter::instance();
    if (!wallet.getTransaction(_transactionId, transaction)) {
      return;
    }

    quint64 transferCount = wallet.getTransferCount(_transactionId);
    if (transferCount) {
      m_transactionRow[_transactionId] = qMakePair(m_transfers.size(), transferCount);
        m_transfers.append(TransactionTransferId(_transactionId, 0));
        ++_insertedRowCount;
    } else {
      m_transfers.append(TransactionTransferId(_transactionId, cn::WALLET_INVALID_TRANSFER_ID));
      m_transactionRow[_transactionId] = qMakePair(m_transfers.size() - 1, 1);
      ++_insertedRowCount;
    }
  }

  void TransactionsModel::appendTransaction(cn::TransactionId _transactionId)
  {
    if (m_transactionRow.contains(_transactionId))
    {
      return;
    }

    quint32 oldRowCount = rowCount();
    quint32 insertedRowCount = 0;
    for (quint64 transactionId = m_transactionRow.size(); transactionId <= _transactionId; ++transactionId)
    {
      appendTransaction(transactionId, insertedRowCount);
    }

    if (insertedRowCount > 0)
    {
      beginInsertRows(QModelIndex(), oldRowCount, oldRowCount + insertedRowCount - 1);
      endInsertRows();
    }
  }

  void TransactionsModel::updateWalletTransaction(cn::TransactionId _id)
  {
    quint32 firstRow = m_transactionRow.value(_id).first;
    quint32 lastRow = firstRow + m_transactionRow.value(_id).second - 1;
    Q_EMIT dataChanged(index(firstRow, COLUMN_DATE), index(lastRow, COLUMN_TYPE));
  }

  void TransactionsModel::lastKnownHeightUpdated(quint64 _height)
  {
    if (rowCount() > 0)
    {
      Q_EMIT dataChanged(index(0, COLUMN_STATE), index(rowCount() - 1, COLUMN_STATE));
    }
  }

  void TransactionsModel::reset()
  {
    beginResetModel();
    m_transfers.clear();
    m_transactionRow.clear();
    endResetModel();
  }

} // namespace WalletGui
