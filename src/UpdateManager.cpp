// Copyright (c) 2016 The Karbowanec developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "UpdateManager.h"

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include "Settings.h"
#include "gui/MainWindow.h"
#include "gui/Notification.h"

using namespace WalletGui;

Updater::Updater(QObject* parent) : QObject(parent)
{
  manager = new QNetworkAccessManager(this);
  connect(manager, &QNetworkAccessManager::finished, this, &Updater::replyFinished);
}
// http://stackoverflow.com/questions/2941491/compare-versions-as-strings/2941895#2941895
class Version
{
  // An internal utility structure just used to make the std::copy in the constructor easy to write.
  struct VersionDigit
  {
    int value;
    operator int() const { return value; }
  };
  friend std::istream& operator>>(std::istream& str, Version::VersionDigit& digit);

public:
  explicit Version(std::string const& versionStr)
  {
    // To Make processing easier in VersionDigit prepend a '.'
    std::stringstream versionStream(std::string(".") + versionStr);

    // Copy all parts of the version number into the version Info vector.
    std::copy(std::istream_iterator<VersionDigit>(versionStream),
              std::istream_iterator<VersionDigit>(), std::back_inserter(versionInfo));
  }

  // Test if two version numbers are the same.
  bool operator<(Version const& rhs) const
  {
    return std::lexicographical_compare(versionInfo.begin(), versionInfo.end(),
                                        rhs.versionInfo.begin(), rhs.versionInfo.end());
  }

private:
  std::vector<int> versionInfo;
};

// Read a single digit from the version.
std::istream& operator>>(std::istream& str, Version::VersionDigit& digit)
{
  str.get();
  str >> digit.value;
  return str;
}

void Updater::checkForUpdate()
{
  const QUrl url = QUrl::fromUserInput(
      "https://raw.githubusercontent.com/ConcealNetwork/conceal-desktop/master/version.txt");
  QNetworkRequest request(url);
  manager->get(request);
}

void Updater::replyFinished(QNetworkReply* reply)
{
  if (reply->error())
  {
    QString error = QString(tr("Error: %1")).arg(reply->errorString());
    Notification* notification = new Notification(&MainWindow::instance());
    notification->notify("Unable to check for update\n" + error);
  }
  else
  {
    Version ourVersion(Settings::instance().getVersion().split("-")[0].toStdString());

    QString result = reply->readAll().data();

    Version remoteVersion(result.toStdString());

    if (ourVersion < remoteVersion)
    {
      if (QMessageBox::warning(
              &MainWindow::instance(), QObject::tr("Conceal Desktop Update"),
              QObject::tr("There is an update available.\nWould you like to go to "
                          "the download page?"),
              QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
      {
        QString link = "https://github.com/ConcealNetwork/conceal-desktop/releases";
        QDesktopServices::openUrl(QUrl(link));
      }
    }
  }
  reply->deleteLater();
}
