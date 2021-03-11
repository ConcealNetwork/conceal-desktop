// Copyright (c) 2016 The Karbowanec developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UPDATE_H
#define UPDATE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrl>

class Updater : public QObject
{
  Q_OBJECT
public:
  explicit Updater(QObject* parent = 0);

  ~Updater() { delete manager; }

  void checkForUpdate();

public slots:
  void replyFinished(QNetworkReply* reply);

private:
  QNetworkAccessManager* manager;
};

#endif  // UPDATE_H
