// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui
{
class BankingFrame2;
}

namespace WalletGui
{

class BankingFrame2 : public QFrame
{
  Q_OBJECT

public:
  BankingFrame2(QWidget *_parent);
  ~BankingFrame2();

private:
  QString m_proof;
  QScopedPointer<Ui::BankingFrame2> m_ui;

  void setMessage(QString optimizationMessage);
  void synchronizationCompleted();
  void delay();

  Q_SLOT void optimizeClicked();
  Q_SLOT void autoOptimizeClicked();  
  Q_SLOT void bpClicked();    
  Q_SLOT void saveLanguageClicked();
  Q_SLOT void saveCurrencyClicked();  
  Q_SLOT void saveConnectionClicked();
  Q_SLOT void closeToTrayClicked();
  Q_SLOT void minToTrayClicked();
  Q_SLOT void backClicked();
  Q_SLOT void rescanClicked();

Q_SIGNALS:
  void backSignal();
  void rescanSignal();
};

} // namespace WalletGui
