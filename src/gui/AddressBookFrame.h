// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui {
class AddressBookFrame;
}

namespace WalletGui {

class AddressListModel;

class AddressBookFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(AddressBookFrame)

public:
  AddressBookFrame(QWidget* _parent);
  ~AddressBookFrame();

private:
  QScopedPointer<Ui::AddressBookFrame> m_ui;
  QScopedArrayPointer<AddressListModel> m_addressBookModel;

  Q_SLOT void addClicked();
  Q_SLOT void copyClicked();
  Q_SLOT void deleteClicked();
  Q_SLOT void currentAddressChanged(const QModelIndex& _index);
  Q_SLOT void addressDoubleClicked(const QModelIndex& _index);
  Q_SLOT void backClicked();



Q_SIGNALS:
  void payToSignal(const QModelIndex& _index);
  void backSignal();  
};

}
