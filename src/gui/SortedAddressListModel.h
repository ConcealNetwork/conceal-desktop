// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QSortFilterProxyModel>

namespace WalletGui {

class SortedAddressListModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  static SortedAddressListModel& instance();

protected:

private:
  SortedAddressListModel();
  ~SortedAddressListModel();
};

}
