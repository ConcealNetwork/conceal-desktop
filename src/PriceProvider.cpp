// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>

#include "PriceProvider.h"

namespace WalletGui {

PriceProvider::PriceProvider(QObject *parent) : QObject(parent), m_networkManager() {
}

PriceProvider::~PriceProvider() {
}

void PriceProvider::getPrice() {
  QUrl url = QUrl::fromUserInput(QString("https://explorer.conceal.network/q/maple/ccx-usd.php"));

  QNetworkRequest request(url);
  QNetworkReply* reply = m_networkManager.get(request);
  connect(reply, &QNetworkReply::readyRead, this, &PriceProvider::readyRead);
  connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void PriceProvider::readyRead() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  QString data = (QString)reply->readAll();
  QStringList pairs = data.split(" ");
  QString ccxusd = pairs[0];
  QString ccxbtc = pairs[1];
  QString btc = pairs[2];
  QString diff = pairs[3];
  QString hashrate = pairs[4];
  QString reward = pairs[5];

  Q_EMIT priceFoundSignal(ccxusd, ccxbtc, btc, diff, hashrate, reward);

}

}
