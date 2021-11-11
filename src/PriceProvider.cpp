// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "PriceProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>

#include "Settings.h"

namespace WalletGui
{
  PriceProvider::PriceProvider(QObject *parent) : QObject(parent), m_networkManager() { }

  PriceProvider::~PriceProvider() { }

  void PriceProvider::getPrice()
  {
    QUrl url = QUrl::fromUserInput(
        QString("https://explorer.conceal.network/services/market/info?vsCurrencies=%1")
            .arg(Settings::instance().getCurrentCurrency()));

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::readyRead, this, &PriceProvider::readyRead);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
  }

  void PriceProvider::readyRead()
  {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull())
    {
      return;
    }

    QJsonObject jsonObject = doc.object();
    QJsonObject result;
    result = jsonObject["conceal"].toObject();
    Q_EMIT priceFoundSignal(result);
  }

}  // namespace WalletGui
