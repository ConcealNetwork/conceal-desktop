// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "AddressListModel.h"
#include "AddressBookModel.h"
#include "SortedAddressListModel.h"
#include "qdatetime.h"

namespace WalletGui {

AddressListModel::AddressListModel() : QSortFilterProxyModel() {
  setSourceModel(&SortedAddressListModel::instance());
}

AddressListModel::~AddressListModel() {
}

bool AddressListModel::filterAcceptsColumn(int _sourceColumn, const QModelIndex& _sourceParent) const {
  return _sourceColumn == AddressBookModel::COLUMN_LABEL || _sourceColumn == AddressBookModel::COLUMN_ADDRESS || _sourceColumn == AddressBookModel::COLUMN_PAYMENTID;
}

}
