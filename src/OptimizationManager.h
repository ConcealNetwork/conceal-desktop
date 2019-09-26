// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <QObject>

#include "CryptoNoteWrapper.h"
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"

namespace WalletGui
{

class OptimizationManager : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(OptimizationManager)

public:
  OptimizationManager(QObject *_parent);
  ~OptimizationManager();

  void checkOptimization();

  Q_SLOT void walletOpened();
  Q_SLOT void walletClosed();
  Q_SLOT void synchronizationProgressUpdated();
  Q_SLOT void synchronizationCompleted();

protected:
  virtual void timerEvent(QTimerEvent *_event);

private:
  int m_checkTimerId;
  int m_optimizationTimerId;
  quint64 m_currentOptimizationInterval;
  bool m_isSynchronized;

  void delay();
  void optimize();
  void ensureStarted();
  void ensureStopped();

};

} // namespace WalletGui
