// Copyright (c) 2011-2016 The Cryptonote developers
// Copyright (c) 2015-2016 XDN developers
// Copyright (c) 2016 Karbowanec developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>
#include "Settings.h"
#include "ExchangeProvider.h"

namespace WalletGui
{

ExchangeProvider::ExchangeProvider(QObject *parent) : QObject(parent), m_networkManager()
{
}

ExchangeProvider::~ExchangeProvider()
{
}

void ExchangeProvider::getExchange(QString &_address)
{
  QString _urlString = "https://explorer.conceal.network/services/exchanges/list?address=" + _address;
  QUrl url = QUrl::fromUserInput(_urlString);
  if (!url.isValid())
  {
    return;
  }

  QNetworkRequest request(url);
  QNetworkReply *reply = m_networkManager.get(request);
  connect(reply, &QNetworkReply::readyRead, this, &ExchangeProvider::readyRead);
  connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void ExchangeProvider::readyRead()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull())
  {
    return;
  }

  QJsonArray arr = doc.array();
  if (arr.count() == 1)
    {
      for (auto value : arr)
      {
        auto object = value.toObject();
        QString name = object.value("name").toString();
        Q_EMIT exchangeFoundSignal(name);
      }
    }
}

} // namespace WalletGui
