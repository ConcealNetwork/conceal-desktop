// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "SortedAddressListModel.h"
#include "AddressBookModel.h"

namespace WalletGui {

SortedAddressListModel& SortedAddressListModel::instance() {
  static SortedAddressListModel inst;
  return inst;
}

SortedAddressListModel::SortedAddressListModel() : QSortFilterProxyModel() {
  setSourceModel(&AddressBookModel::instance());
  setDynamicSortFilter(true);
  sort(AddressBookModel::COLUMN_LABEL, Qt::DescendingOrder);
}

SortedAddressListModel::~SortedAddressListModel() {
}


}
