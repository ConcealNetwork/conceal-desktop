// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QTime>
#include <QTimer>
#include <QThread>
#include <QTimerEvent>
#include <QMessageBox>

#include "OptimizationManager.h"
#include "WalletAdapter.h"
#include "gui/WalletEvents.h"
#include "NodeAdapter.h"
#include "Settings.h"

namespace WalletGui
{

namespace
{

const int CHECK_TIMER_INTERVAL = 1000;
}

OptimizationManager::OptimizationManager(QObject *_parent) : QObject(_parent),
                                                             m_checkTimerId(-1),
                                                             m_optimizationTimerId(-1),
                                                             m_currentOptimizationInterval(0),
                                                             m_isSynchronized(false)
{
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &OptimizationManager::walletOpened);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &OptimizationManager::walletClosed);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationProgressUpdatedSignal, this, &OptimizationManager::synchronizationProgressUpdated, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &OptimizationManager::synchronizationCompleted, Qt::QueuedConnection);
}

OptimizationManager::~OptimizationManager()
{
}

void OptimizationManager::walletOpened()
{
  Q_ASSERT(m_checkTimerId == -1);
  m_checkTimerId = startTimer(CHECK_TIMER_INTERVAL);
}

void OptimizationManager::walletClosed()
{
  m_isSynchronized = false;
  if (m_checkTimerId != -1)
  {
    killTimer(m_checkTimerId);
    m_checkTimerId = -1;
  }

  if (m_optimizationTimerId != -1)
  {
    killTimer(m_optimizationTimerId);
    m_optimizationTimerId = -1;
  }
}

void OptimizationManager::synchronizationProgressUpdated()
{
  m_isSynchronized = false;
}

void OptimizationManager::synchronizationCompleted()
{
  m_isSynchronized = true;
}

void OptimizationManager::timerEvent(QTimerEvent *_event)
{
  if (_event->timerId() == m_checkTimerId)
  {
    checkOptimization();
  }
  else if (_event->timerId() == m_optimizationTimerId)
  {
    optimize();
  }

  QObject::timerEvent(_event);
}

void OptimizationManager::checkOptimization()
{
  if ((Settings::instance().getAutoOptimizationStatus() == "enabled") && WalletAdapter::instance().isOpen())
  {
    ensureStarted();
  }
}

void OptimizationManager::optimize()
{
  if (Settings::instance().isTrackingMode())
  {
    /* Tracking wallet. Do nothing */
  }
  else
  {
 quint64 numUnlockedOutputs;
  if (WalletAdapter::instance().isOpen() && m_isSynchronized)
  {
    while (WalletAdapter::instance().getNumUnlockedOutputs() > 100)
    {
      numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
      if (numUnlockedOutputs == 0)
        break;
      WalletAdapter::instance().optimizeWallet();
      delay();
    }
  }
  }  
 
}

void OptimizationManager::delay()
{
  QThread::sleep(2);
}

void OptimizationManager::ensureStarted()
{
  if (m_optimizationTimerId != -1)
  {
    if (m_currentOptimizationInterval == Settings::instance().getOptimizationInterval())
    {
      return;
    }
  }
  m_currentOptimizationInterval = Settings::instance().getOptimizationInterval();
  m_optimizationTimerId = startTimer(m_currentOptimizationInterval);
  optimize();
}

void OptimizationManager::ensureStopped()
{
  if (m_optimizationTimerId == -1)
  {
    return;
  }
  killTimer(m_optimizationTimerId);
  m_currentOptimizationInterval = 0;
  m_optimizationTimerId = -1;
}
} // namespace WalletGui
