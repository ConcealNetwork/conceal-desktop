// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QFile>
#include <QFileSystemWatcher>
#include <QTextStream>
#include <QTimerEvent>

#include "LogFileWatcher.h"

namespace WalletGui {

LogFileWatcher::LogFileWatcher(const QString& _fileName, QObject* _parent) : QObject(_parent),
  m_logFile(new QFile(_fileName, this)) {
  if (m_logFile->open(QFile::ReadOnly | QFile::Text)) {
    m_logFile->seek(m_logFile->size());
    m_fileCheckTimer = startTimer(300);
  }
}

LogFileWatcher::~LogFileWatcher() {
}

void LogFileWatcher::timerEvent(QTimerEvent* _event) {
  if (_event->timerId() == m_fileCheckTimer) {
    if (!m_logFile->atEnd()) {
      fileChanged();
    }
  }

  QObject::timerEvent(_event);
}

void LogFileWatcher::fileChanged() {
  QTextStream stream(m_logFile);
  while(!stream.atEnd()) {
    QString line = stream.readLine();
    Q_EMIT newLogStringSignal(line);
  }
}

}
