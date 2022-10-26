// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2022 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

namespace WalletGui {

class AddressProvider : public QObject {
  Q_OBJECT

public:
  explicit AddressProvider(QObject *parent);
  ~AddressProvider();

  void getAddress();

private:
  QNetworkAccessManager m_networkManager;
  void readyRead();

Q_SIGNALS:
  void addressFoundSignal(const QString& _address);
};

}
