// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

class QAbstractButton;

namespace Ui {
class WelcomeFrame;
}

namespace WalletGui {

class WelcomeFrame : public QFrame {
  Q_OBJECT

public:
  WelcomeFrame(QWidget* _parent);
  ~WelcomeFrame();

  Q_SLOT void createWallet();
  Q_SLOT void openWallet();

private:
  QScopedPointer<Ui::WelcomeFrame> m_ui;

Q_SIGNALS:
  void createWalletClickedSignal();
  void openWalletClickedSignal(); 
};

}
