// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QObject>

class QFile;
class QFileSystemWatcher;

namespace WalletGui {

class LogFileWatcher : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(LogFileWatcher)

public:
  LogFileWatcher(const QString& _filePath, QObject* _parent);
  ~LogFileWatcher();

protected:
  void timerEvent(QTimerEvent* _event) override;

private:
  int m_fileCheckTimer;
  QFile* m_logFile;

  void fileChanged();

Q_SIGNALS:
  void newLogStringSignal(const QString& _string);
};

}
