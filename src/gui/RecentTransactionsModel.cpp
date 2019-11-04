// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "RecentTransactionsModel.h"
#include "SortedTransactionsModel.h"
#include <QColor>

namespace WalletGui {

RecentTransactionsModel::RecentTransactionsModel() : QSortFilterProxyModel() {
  setSourceModel(&SortedTransactionsModel::instance());
  setDynamicSortFilter(true);
  connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &RecentTransactionsModel::invalidateFilter);
}

RecentTransactionsModel::~RecentTransactionsModel() {
}

int RecentTransactionsModel::columnCount(const QModelIndex& _parent) const {
  return 1;
}

QVariant RecentTransactionsModel::data(const QModelIndex& _index, int _role) const {
  if(_role == Qt::DecorationRole) {
    return QVariant();
  }
  if(_role == Qt::BackgroundRole) {
    return QColor(40, 45, 49);
  }
  return QSortFilterProxyModel::data(_index, _role);
}

bool RecentTransactionsModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const {
  return _sourceRow < 3;
}


}
