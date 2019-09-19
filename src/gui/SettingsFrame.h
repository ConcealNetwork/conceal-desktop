// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui {
  class SettingsFrame;
}

namespace WalletGui {

class SettingsFrame : public QFrame {
  Q_OBJECT

public:
  SettingsFrame(QWidget* _parent);
  ~SettingsFrame(); 

  void setMessage(QString optimizationMessage);
  void optimizeClicked();
  void delay();
  void backClicked();
  void rescanClicked();

private:
  QScopedPointer<Ui::SettingsFrame> m_ui;

Q_SIGNALS:
  void backSignal();
  void rescanSignal();
};

}
