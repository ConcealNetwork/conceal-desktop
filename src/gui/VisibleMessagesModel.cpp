// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "VisibleMessagesModel.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"

namespace WalletGui {

VisibleMessagesModel::VisibleMessagesModel() : QSortFilterProxyModel() {
  setSourceModel(&SortedMessagesModel::instance());
}

VisibleMessagesModel::~VisibleMessagesModel() {
}

bool VisibleMessagesModel::filterAcceptsColumn(int _sourceColumn, const QModelIndex& _sourceParent) const {
  return _sourceColumn == MessagesModel::COLUMN_DATE || _sourceColumn == MessagesModel::COLUMN_TYPE ||
    _sourceColumn == MessagesModel::COLUMN_HEIGHT || _sourceColumn == MessagesModel::COLUMN_MESSAGE;
}

}
