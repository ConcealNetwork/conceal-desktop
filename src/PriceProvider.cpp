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
  QUrl url = "https://maplechange.com/api/v2/tickers/ccxbtc.json";

  QNetworkRequest request(url);
  QNetworkReply* reply = m_networkManager.get(request);
  connect(reply, &QNetworkReply::readyRead, this, &PriceProvider::readyRead);
  connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void PriceProvider::readyRead() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  QString data = (QString)reply->readAll();
  Q_EMIT priceFoundSignal(data, data);

  QJsonObject obj;

  QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
  if(doc.isObject())
  {
      obj = doc.object();        
      Q_EMIT priceFoundSignal("object", "object");
  }
 /* if (doc.isNull()) {
    return;


 Q_EMIT priceFoundSignal(data, data);

  if (jsonArray.isEmpty()) {
             
    return;
  }

  QJsonObject obj = jsonArray.first().toObject();
  QString name = obj.value("last").toString();
  QString address = obj.value("high").toString();

  if (!name.isEmpty()) {
    Q_EMIT priceFoundSignal(name, address);
  }*/
}

}
