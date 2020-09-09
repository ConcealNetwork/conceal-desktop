// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

namespace WalletGui {

class PriceProvider : public QObject {
  Q_OBJECT

public:
  explicit PriceProvider(QObject *parent);
  ~PriceProvider();

  void getPrice();

private:
  QNetworkAccessManager m_networkManager;
  void readyRead();

Q_SIGNALS:
  void priceFoundSignal(QJsonObject &result);
};

}
