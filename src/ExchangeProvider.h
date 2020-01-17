// Copyright (c) 2011-2016 The Cryptonote developers
// Copyright (c) 2015-2016 XDN developers
// Copyright (c) 2016 Karbowanec developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

namespace WalletGui {

class ExchangeProvider : public QObject {
  Q_OBJECT

public:
  explicit ExchangeProvider(QObject *parent);
  ~ExchangeProvider();

  void getExchange(QString &_address);

private:
  QNetworkAccessManager m_networkManager;
  void readyRead();

Q_SIGNALS:
  void exchangeFoundSignal(QString &_exchange);
};

}
