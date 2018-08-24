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
  PriceProvider(QObject *parent);
  ~PriceProvider();

  void getPrice();

private:
  QNetworkAccessManager m_networkManager;
  void readyRead();

Q_SIGNALS:
  void priceFoundSignal(const QString& _ccxusd, const QString& _ccxbtc, const QString& _btc, const QString& _diff, const QString& _hashrate, const QString& _reward, const QString& _deposits, const QString& _supply);
};

}
